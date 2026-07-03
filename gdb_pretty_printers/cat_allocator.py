import gdb
import re

from cat_gdb import cat_type

_ALLOCATOR_PRINTERS = {}

_LEAF_TAGS = frozenset(
    {
        'linear_allocator',
        'pool_allocator',
        'resizable_allocator',
        'page_allocator',
        'null_allocator',
    }
)


def _format_ptr(value):
    if value is None:
        return '0x0'
    try:
        void_ptr = value.cast(gdb.lookup_type('void').pointer())
        return str(void_ptr)
    except gdb.error:
        return str(value)


def _str_view(value):
    try:
        p_data = value['m_p_data']
        size = int(value['m_size']['raw'])
        if size is None or size == 0:
            return ''
        char_ptr = p_data.cast(gdb.lookup_type('char').pointer())
        return char_ptr.string(length=size)
    except gdb.error:
        return ''


def _type_tag(val):
    name = val.type.strip_typedefs().name or str(val.type)
    match = re.search(r'cat::(\w+)', name)
    return match.group(1) if match else name


def _template_idx_arg(val, index):
    try:
        return int(val.type.template_argument(index)['raw'])
    except (gdb.error, RuntimeError):
        return None


def _has_field(val, name):
    try:
        val[name]
        return True
    except gdb.error:
        return False


def _count_free_list(head, limit):
    count = 0
    seen = set()
    p_walk = head
    while int(p_walk) != 0 and count < limit:
        addr = int(p_walk)
        if addr in seen:
            break
        seen.add(addr)
        count += 1
        try:
            p_walk = p_walk['p_next']
        except gdb.error:
            break
    return count


_SKIP_ANON_PREFIXES = (
    '[stack',
    '[heap]',
    '[vdso]',
    '[vvar]',
    '[vsyscall]',
)

_MAX_ANON_REGION_KB = 65536


def _smaps_pathname(header):
    parts = header.split()
    return parts[5] if len(parts) >= 6 else ''


def _is_file_backed_map(header):
    tag = _smaps_pathname(header)
    return tag.startswith('/')


def _is_private_anonymous_map(header):
    parts = header.split()
    if len(parts) < 5:
        return False
    perms = parts[1]
    if 'p' not in perms or 'w' not in perms:
        return False
    tag = _smaps_pathname(header)
    if tag.startswith('/'):
        return False
    if any(tag.startswith(prefix) for prefix in _SKIP_ANON_PREFIXES):
        return False
    lo, hi = (int(address, 16) for address in parts[0].split('-'))
    if hi <= lo:
        return False
    return True


def _region_span_valid(header, size_kb, max_region_kb):
    parts = header.split()
    if len(parts) < 2:
        return False
    lo, hi = (int(address, 16) for address in parts[0].split('-'))
    if hi <= lo:
        return False
    if size_kb > max_region_kb:
        return False
    if size_kb > _MAX_ANON_REGION_KB and not _is_file_backed_map(header):
        tag = _smaps_pathname(header)
        if any(tag.startswith(prefix) for prefix in _SKIP_ANON_PREFIXES):
            return False
        return False
    return True


def _flush_smaps_region(header, size_kb, rss_kb, max_region_kb, totals):
    if header is None or not _region_span_valid(header, size_kb, max_region_kb):
        return
    if _is_private_anonymous_map(header):
        totals['resident_kb'] += rss_kb
    totals['total_mapped_kb'] += size_kb
    if _is_file_backed_map(header):
        totals['file_mapped_kb'] += size_kb


def _anon_smaps_from_proc():
    try:
        pid = gdb.selected_inferior().pid
        with open(f'/proc/{pid}/smaps', encoding='ascii') as handle:
            lines = handle.readlines()
    except (OSError, gdb.error):
        return None, None

    max_region_kb = 1024 * 1024

    totals = {'resident_kb': 0, 'total_mapped_kb': 0, 'file_mapped_kb': 0}
    header = None
    size_kb = rss_kb = 0
    for line in lines:
        if line and not line.startswith((' ', '\t')) and '-' in line:
            _flush_smaps_region(header, size_kb, rss_kb, max_region_kb, totals)
            header = line.rstrip('\n')
            size_kb = rss_kb = 0
        elif line.startswith('Size:'):
            size_kb = int(line.split()[1])
        elif line.startswith('Rss:'):
            rss_kb = int(line.split()[1])
    _flush_smaps_region(header, size_kb, rss_kb, max_region_kb, totals)
    mapped_kb = totals['total_mapped_kb'] - totals['file_mapped_kb']
    return totals['resident_kb'] * 1024, mapped_kb * 1024


def _format_anon_memory(resident, mapped):
    if resident is None or mapped is None:
        return 'unknown process anon memory'
    return (
        f'{_format_byte_count(resident)} RSS / '
        f'{_format_byte_count(mapped)} mapped (process anon memory)'
    )


def _unwrap_allocator_ref(val):
    tag = _type_tag(val)
    if tag not in ('named_allocator', 'allocator_ref'):
        return val
    try:
        storage = val['m_storage']
        if storage.type.code == gdb.TYPE_CODE_PTR:
            if int(storage) == 0:
                return val
            return storage.dereference()
        return storage
    except gdb.error:
        return val


def _format_byte_count(value):
    ki = 1024
    if value < ki:
        return f'{value} bytes'
    scaled = float(value) / ki
    for suffix in ('KiB', 'MiB', 'GiB', 'TiB', 'PiB'):
        if scaled < ki or suffix == 'PiB':
            if scaled == int(scaled):
                return f'{int(scaled)} {suffix}'
            formatted = f'{scaled:.2f}'.rstrip('0').rstrip('.')
            return f'{formatted} {suffix}'
        scaled /= ki


def _format_usage(used, capacity, prefix='', suffix=''):
    if used is None or capacity is None:
        if prefix:
            return prefix.rstrip(': ') + suffix if suffix else prefix + 'unknown usage'
        return 'unknown usage' + suffix
    pct = (used * 100 // capacity) if capacity else 0
    body = f'{_format_byte_count(used)}/{_format_byte_count(capacity)} ({pct}%)'
    if prefix:
        body = prefix + body
    return body + suffix


def _bar(used, capacity, width=32):
    if capacity is None or capacity <= 0:
        return '[' + ('?' * width) + ']'
    filled = min(width, max(0, used * width // capacity))
    return '[' + ('=' * filled) + ('-' * (width - filled)) + ']'


def _printer_for(val):
    target = _AllocatorPrinter.resolve(val)
    tag = _type_tag(target)
    if tag in _LEAF_TAGS:
        return _ALLOCATOR_PRINTERS[tag], target
    if _has_field(target, 'm_nodes'):
        return _ALLOCATOR_PRINTERS['pool_allocator'], target
    if _has_field(target, 'm_p_arena_begin'):
        return _ALLOCATOR_PRINTERS['linear_allocator'], target
    return None, target


def _usage_for(val):
    printer_cls, target = _printer_for(val)
    if printer_cls:
        return printer_cls.usage(target)
    return None, None


class _AllocatorPrinter:
    tag = ''

    def __init_subclass__(cls, *, tag=None, **kwargs):
        super().__init_subclass__(**kwargs)
        if tag is not None:
            cls.tag = tag
            _ALLOCATOR_PRINTERS[tag] = cls

    def __init__(self, val, prefix='', suffix=''):
        self.val = val
        self.prefix = prefix
        self.suffix = suffix
        try:
            self.used, self.capacity = self.usage(val)
        except (gdb.error, RuntimeError):
            self.used, self.capacity = None, None

    @classmethod
    def resolve(cls, val):
        target = _unwrap_allocator_ref(val)
        for _ in range(4):
            tag = _type_tag(target)
            if tag in _LEAF_TAGS:
                return target
            if _has_field(target, 'm_p_arena_begin') or _has_field(target, 'm_nodes'):
                return target
            if tag != 'allocator_ref':
                break
            next_target = _unwrap_allocator_ref(target)
            if int(next_target.address) == int(target.address):
                break
            target = next_target
        return target

    @classmethod
    def usage(cls, val):
        return None, None

    @classmethod
    def dump(cls, val, out):
        used, capacity = cls.usage(val)
        out.write(f'{cls.tag} {_format_usage(used, capacity, suffix="")}\n')

    @classmethod
    def find_in(cls, val, address, out, indent=''):
        return False

    @classmethod
    def vis_details(cls, val, out):
        pass

    def to_string(self):
        return _format_usage(self.used, self.capacity, self.prefix, self.suffix)


@cat_type('linear_allocator')
class _LinearAllocatorPrinter(_AllocatorPrinter, tag='linear_allocator'):
    @classmethod
    def usage(cls, val):
        try:
            begin = int(val['m_p_arena_begin']['raw'])
            end = int(val['m_p_arena_end']['raw'])
            current = int(val['m_p_arena_current']['raw'])
        except gdb.error:
            return None, None
        if begin is None or end is None or current is None:
            return None, None
        return begin - current, begin - end

    @classmethod
    def dump(cls, val, out):
        used, capacity = cls.usage(val)
        begin = int(val['m_p_arena_begin']['raw'])
        end = int(val['m_p_arena_end']['raw'])
        current = int(val['m_p_arena_current']['raw'])
        out.write(f'linear_allocator {_format_usage(used, capacity, suffix="")}\n')
        out.write(
            f'  arena: {_format_ptr(val["m_p_arena_end"])}'
            f' .. {_format_ptr(val["m_p_arena_begin"])}\n'
        )
        out.write(f'  bump:  {_format_ptr(val["m_p_arena_current"])}\n')
        if begin is not None and end is not None and current is not None:
            out.write(f'  {_bar(used, capacity)}\n')
            out.write(
                f'  live:  {_format_ptr(val["m_p_arena_current"])}'
                f' .. {_format_ptr(val["m_p_arena_begin"])}\n'
            )
            out.write(
                f'  free:  {_format_ptr(val["m_p_arena_end"])}'
                f' .. {_format_ptr(val["m_p_arena_current"])}\n'
            )

    @classmethod
    def contains(cls, val, address):
        end = int(val['m_p_arena_end']['raw'])
        begin = int(val['m_p_arena_begin']['raw'])
        if end is None or begin is None:
            return False
        return end <= address < begin

    @classmethod
    def find_in(cls, val, address, out, indent=''):
        if not cls.contains(val, address):
            return False
        out.write(f'{indent}in linear arena\n')
        current = int(val['m_p_arena_current']['raw'])
        begin = int(val['m_p_arena_begin']['raw'])
        if current is not None and begin is not None and current <= address < begin:
            out.write(f'{indent}status: live bump region\n')
        else:
            out.write(f'{indent}status: free/reserved tail\n')
        return True

    @classmethod
    def vis_details(cls, val, out):
        out.write(f'  end {_format_ptr(val["m_p_arena_end"])}\n')
        out.write(f'  cur {_format_ptr(val["m_p_arena_current"])}\n')
        out.write(f'  top {_format_ptr(val["m_p_arena_begin"])}\n')


@cat_type('pool_allocator')
class _PoolAllocatorPrinter(_AllocatorPrinter, tag='pool_allocator'):
    @classmethod
    def node_bytes(cls, val):
        type_name = val.type.strip_typedefs().name or ''
        match = re.search(r',\s*(\d+)\}>\s*$', type_name)
        if match:
            return int(match.group(1))
        match = re.search(r'pool_allocator<\s*(\d+)\s*>', type_name)
        if match:
            return int(match.group(1))
        return _template_idx_arg(val, 0)

    @classmethod
    def usage(cls, val):
        try:
            nodes = val['m_nodes']
            node_count = int(nodes['m_size']['raw'])
            max_node_bytes = cls.node_bytes(val)
            if node_count is None or max_node_bytes is None:
                return None, None
            free_nodes = _count_free_list(val['m_p_head'], node_count + 1)
            used_nodes = node_count - free_nodes
            return used_nodes * max_node_bytes, node_count * max_node_bytes
        except gdb.error:
            return None, None

    @classmethod
    def dump(cls, val, out):
        used, capacity = cls.usage(val)
        max_node = cls.node_bytes(val)
        node_count = int(val['m_nodes']['m_size']['raw'])
        free_nodes = _count_free_list(val['m_p_head'], (node_count or 0) + 1)
        label = (
            f'pool_allocator<{max_node}>' if max_node is not None else 'pool_allocator'
        )
        out.write(f'{label} {_format_usage(used, capacity, suffix="")}\n')
        out.write(f'  nodes: {node_count} x {max_node} bytes\n')
        out.write(f'  free list: {free_nodes} nodes\n')
        out.write(f'  head: {_format_ptr(val["m_p_head"])}\n')
        index = 0
        p_walk = val['m_p_head']
        while int(p_walk) != 0 and index < (node_count or 0):
            out.write(f'    [{index}] {_format_ptr(p_walk)}\n')
            try:
                p_walk = p_walk['p_next']
            except gdb.error:
                break
            index += 1

    @classmethod
    def contains(cls, val, address):
        try:
            nodes = val['m_nodes']
            p_data = nodes['m_p_data']
            base = int(p_data.cast(gdb.lookup_type('void').pointer()))
            size = int(nodes['m_size']['raw'])
            max_node = cls.node_bytes(val)
            if size is None or max_node is None:
                return False
            end = base + size * max_node
            return base <= address < end
        except gdb.error:
            return False

    @classmethod
    def find_in(cls, val, address, out, indent=''):
        if not cls.contains(val, address):
            return False
        out.write(f'{indent}in pool node slab\n')
        return True

    @classmethod
    def vis_details(cls, val, out):
        out.write(f'  slab {_format_ptr(val["m_nodes"]["m_p_data"])}\n')
        out.write(f'  free {_format_ptr(val["m_p_head"])}\n')


@cat_type('page_allocator')
class _PageAllocatorPrinter(_AllocatorPrinter, tag='page_allocator'):
    @classmethod
    def usage(cls, val):
        return _anon_smaps_from_proc()

    def to_string(self):
        return _format_anon_memory(self.used, self.capacity)

    @classmethod
    def dump(cls, val, out):
        resident, mapped = cls.usage(val)
        out.write(f'page_allocator {_format_anon_memory(resident, mapped)}\n')
        out.write('  (process anon memory via /proc/self/smaps)\n')


@cat_type('null_allocator')
class _NullAllocatorPrinter(_AllocatorPrinter, tag='null_allocator'):
    @classmethod
    def usage(cls, val):
        return 0, 0


@cat_type('resizable_allocator')
class _ResizableAllocatorPrinter(_AllocatorPrinter, tag='resizable_allocator'):
    @classmethod
    def usage(cls, val):
        try:
            node_bytes = _template_idx_arg(val, 2)
            if node_bytes is None:
                return None, None
            used_total = 0
            cap_total = 0
            p_chunk = val['m_p_chunks']
            chunk_index = 0
            while int(p_chunk) != 0 and chunk_index < 256:
                inner_used, inner_cap = _usage_for(p_chunk['inner'])
                if inner_used is not None:
                    used_total += inner_used
                if inner_cap is not None:
                    cap_total += inner_cap
                try:
                    p_chunk = p_chunk['p_next']
                except gdb.error:
                    break
                chunk_index += 1
            free_slots = _count_free_list(val['m_p_free_head'], 1024)
            used = used_total - free_slots * node_bytes
            if used < 0:
                used = 0
            return used, cap_total
        except gdb.error:
            return None, None

    @classmethod
    def dump(cls, val, out):
        used, capacity = cls.usage(val)
        node_bytes = _template_idx_arg(val, 2)
        chunk_bytes = _template_idx_arg(val, 3)
        out.write(f'resizable_allocator {_format_usage(used, capacity, suffix="")}\n')
        out.write(f'  node_bytes: {node_bytes}, chunk_bytes: {chunk_bytes}\n')
        free_slots = _count_free_list(val['m_p_free_head'], 1024)
        out.write(
            f'  free list: {free_slots} slots at {_format_ptr(val["m_p_free_head"])}\n'
        )
        chunk_index = 0
        p_chunk = val['m_p_chunks']
        while int(p_chunk) != 0 and chunk_index < 256:
            chunk_used, chunk_capacity = _usage_for(p_chunk['inner'])
            chunk_total = int(p_chunk['chunk_bytes']['raw'])
            out.write(
                f'  chunk[{chunk_index}] {_format_ptr(p_chunk)}'
                f' total={chunk_total}'
                f' inner={chunk_used}/{chunk_capacity}\n'
            )
            try:
                p_chunk = p_chunk['p_next']
            except gdb.error:
                break
            chunk_index += 1

    @classmethod
    def find_in(cls, val, address, out, indent=''):
        p_chunk = val['m_p_chunks']
        chunk_index = 0
        while int(p_chunk) != 0 and chunk_index < 256:
            printer_cls, inner = _printer_for(p_chunk['inner'])
            if printer_cls and printer_cls.find_in(
                inner, address, out, indent + f'chunk[{chunk_index}] '
            ):
                out.write(f'{indent}  at {_format_ptr(p_chunk)}\n')
                return True
            try:
                p_chunk = p_chunk['p_next']
            except gdb.error:
                break
            chunk_index += 1
        p_free = val['m_p_free_head']
        index = 0
        while int(p_free) != 0 and index < 1024:
            if int(p_free) == address:
                out.write(f'{indent}free list slot[{index}]\n')
                return True
            try:
                p_free = p_free['p_next']
            except gdb.error:
                break
            index += 1
        return False


@cat_type('named_allocator')
class _NamedAllocatorPrinter(_AllocatorPrinter, tag='named_allocator'):
    def __init__(self, val):
        name = _str_view(val['m_name'])
        prefix = f'"{name}": ' if name else ''
        super().__init__(val, prefix=prefix)

    @classmethod
    def usage(cls, val):
        return _usage_for(val)

    @classmethod
    def dump(cls, val, out):
        name = _str_view(val['m_name'])
        out.write(f'named_allocator "{name}"\n')
        used, capacity = cls.usage(val)
        out.write(f'  {_format_usage(used, capacity, suffix="")}\n')


def dump_allocator(val, out=None):
    if out is None:
        out = gdb
    tag = _type_tag(val)
    printer_cls = _ALLOCATOR_PRINTERS.get(tag)
    if printer_cls:
        target = val if tag == 'named_allocator' else _AllocatorPrinter.resolve(val)
        printer_cls.dump(target, out)
        return
    printer_cls, target = _printer_for(val)
    if printer_cls:
        printer_cls.dump(target, out)
        return
    used, capacity = _usage_for(val)
    out.write(f'{tag} {_format_usage(used, capacity, suffix="")}\n')


def find_address(val, address, out=None):
    if out is None:
        out = gdb
    tag = _type_tag(val)
    out.write(f'searching {tag} for {address:#x}\n')
    printer_cls, target = _printer_for(val)
    if printer_cls and printer_cls.find_in(target, address, out, indent='  '):
        return True
    used, capacity = _usage_for(val)
    out.write(f'  not found (usage {used}/{capacity})\n')
    return False


def vis_allocator(val, out=None):
    if out is None:
        out = gdb
    printer_cls, target = _printer_for(val)
    tag = _type_tag(target)
    if tag == 'page_allocator':
        resident, mapped = _usage_for(val)
        out.write(f'{tag} {_format_anon_memory(resident, mapped)}\n')
        return
    used, capacity = _usage_for(val)
    out.write(
        f'{tag} {_bar(used or 0, capacity)} '
        f'{_format_usage(used, capacity, suffix="")}\n'
    )
    if printer_cls:
        printer_cls.vis_details(target, out)


class _CatAllocCommand(gdb.Command):
    """Inspect libCat allocators (dump, find, vis)."""

    def __init__(self):
        super().__init__('cat-alloc', gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        argv = gdb.string_to_argv(arg)
        if len(argv) < 2:
            gdb.write(
                'usage: cat-alloc dump EXPR\n'
                '       cat-alloc find EXPR ADDRESS\n'
                '       cat-alloc vis EXPR\n'
            )
            return
        subcommand = argv[0]
        expr = argv[1]
        val = gdb.parse_and_eval(expr)
        if subcommand == 'dump':
            dump_allocator(val)
        elif subcommand == 'vis':
            vis_allocator(val)
        elif subcommand == 'find':
            if len(argv) < 3:
                gdb.write('usage: cat-alloc find EXPR ADDRESS\n')
                return
            address = int(gdb.parse_and_eval(argv[2]))
            find_address(val, address)
        else:
            gdb.write(f'unknown cat-alloc subcommand: {subcommand}\n')


def register_commands(register):
    register(_CatAllocCommand)
