import gdb
import re
import os
import sys

_printer_dir = os.path.dirname(os.path.abspath(__file__))
if _printer_dir not in sys.path:
    sys.path.insert(0, _printer_dir)

sys.modules.pop('cat_gdb', None)
from cat_gdb import cat_pretty_printers, cat_type  # noqa: E402


def register_user_command(command_class):
    try:
        command_class()
    except gdb.error as error:
        if 'already exists' not in str(error).lower():
            raise


def register_libcat_commands():
    cat_allocator.register_commands(register_user_command)


@cat_type('monostate_type')
class MonostatePrinter:
    "Print a `cat::monostate`"

    def __init__(self, val: gdb.Value):
        return

    def to_string(self):
        return 'monostate'


@cat_type('basic_int')
class BasicIntPrinter:
    "Print a `cat::basic_int`"

    def __init__(self, val: gdb.Value):
        self.raw = val['raw']

        typed_policy = val.type.template_argument(1)
        stripped_policy = str(typed_policy)[24:]
        match stripped_policy:
            case 'undefined':
                self.policy = 'undefined'
            case 'wrap':
                self.policy = 'wrap'
            case 'saturate':
                self.policy = 'sat'
            # case 'trap':
            #     self.policy = 'trap'
            case _:
                self.policy = 'WTF'

        return

    def to_string(self):
        if self.policy == 'undefined':
            return str(self.raw)
        return str(self.raw) + ' (' + self.policy + ')'


@cat_type('basic_float')
class BasicFloatPrinter:
    "Print a `cat::basic_float`"

    def __init__(self, val: gdb.Value):
        self.raw = val['raw']
        return

    def to_string(self):
        return str(self.raw)


@cat_type('basic_idx')
class IndexPrinter:
    "Print a `cat::idx`"

    def __init__(self, val: gdb.Value):
        self.raw = val['raw']
        typed_policy = val.type.template_argument(0)
        stripped_policy = str(typed_policy)[24:]
        match stripped_policy:
            case 'undefined':
                self.policy = 'undefined'
            case 'wrap':
                self.policy = 'wrap'
            case 'saturate':
                self.policy = 'sat'
            # case 'trap':
            #     self.policy = 'trap'
            case _:
                self.policy = 'WTF'
        return

    def to_string(self):
        if self.policy == 'undefined':
            return str(self.raw)
        return str(self.raw) + ' (' + self.policy + ')'


@cat_type('byte')
class BytePrinter:
    "Print a `cat::byte`"

    def __init__(self, val: gdb.Value):
        self.value = val['value']['raw']
        return

    def to_string(self):
        return str(hex(self.value))


@cat_type('span')
class SpanPrinter:
    "Print a `cat::span`"

    def __init__(self, val: gdb.Value):
        self.m_p_data = val['m_p_data']
        self.m_size: int = val['m_size']['raw']
        return

    def to_string(self):
        p_data = self.m_p_data.cast(
            self.m_p_data.type.target().strip_typedefs().pointer()
        )

        array: str = ',\n'.join(
            ['  [' + str(x) + '] = ' + str(p_data[x]) for x in range(self.m_size)]
        )

        return (
            '{' + str(p_data) + ', ' + str(self.m_size) + '}' + ' [\n' + array + '\n]'
        )


@cat_type('array')
class ArrayPrinter:
    "Print a `cat::array`"

    def __init__(self, val: gdb.Value):
        self.m_data: gdb.Value = val['m_data']
        self.size: int = int(
            str(re.search('[0-9]+}>$', str(val.type.strip_typedefs())).group(0))[:-2]
        )
        return

    def to_string(self):
        return '[' + str(self.m_data)[1:-1] + '] (size: ' + str(self.size) + ')'


@cat_type('basic_str_span')
class StrSpanPrinter:
    "Print a `cat::(z)str_span` or `cat::(z)str_view`"

    def __init__(self, val: gdb.Value):
        self.m_p_data = val['m_p_data']
        self.m_size = val['m_size']['raw']
        self.is_null_terminated = str(val.type.template_argument(1)) == 'true'
        return

    def to_string(self):
        p_str = self.m_p_data.cast(
            self.m_p_data.type.target().strip_typedefs().pointer()
        )

        # Print null bytes as `\0`.
        string = p_str.string(length=self.m_size)
        string = ''.join(['\\0' if i == '\0' else i for i in string])

        zstr_suffix = ', null-terminated' if self.is_null_terminated else ''
        return (
            '"'
            + string
            + '" ('
            + str(self.m_size)
            + ' chars'
            + zstr_suffix
            + ', '
            + str(hex(self.m_p_data))
            + ')'
        )


@cat_type('basic_str_inplace')
class StrInplacePrinter:
    "Print a `cat::(z)str_inplace`"

    def __init__(self, val: gdb.Value):
        self.m_data = val['m_data']
        self.is_null_terminated = str(val.type.template_argument(2)) == 'true'

        try:
            self.m_size = val.type.template_argument(1)['raw']
        except:
            # If the template parameter can't be found by GDB, parse out the last
            # integer in the type signature instead.
            type_string = val.type.strip_typedefs().name
            regex = re.compile(r'[0-9]+', re.MULTILINE)
            self.m_size = int(regex.findall(type_string)[-1])

        return

    def to_string(self):
        p_str = self.m_data.cast(self.m_data.type.target().strip_typedefs().pointer())

        # Print null bytes as `\0`.
        string = p_str.string(length=self.m_size)
        string = ''.join(['\\0' if i == '\0' else i for i in string])

        zstr_suffix = ', null-terminated' if self.is_null_terminated else ''
        return '"' + string + '" (' + str(self.m_size) + ' chars' + zstr_suffix + ')'


@cat_type('bit_value')
class BitValuePrinter:
    "Print a `cat::bit_value`"

    def __init__(self, val: gdb.Value):
        self.m_value: gdb.Value = val['m_value']

    def to_string(self):
        return '1' if bool(self.m_value) else '0'


@cat_type('bitset')
class BitsetPrinter:
    "Print a `cat::bitset`"

    def __init__(self, val: gdb.Value):
        self.m_data: gdb.Value = val['m_data']['m_data']
        self.element_size_bits: int = self.m_data[0].type.sizeof * 8
        # Extract the length of this internal array.
        self.array_len: int = self.m_data.type.range()[1] + 1

        # The number of leading bits that are ignored by this container.
        try:
            self.leading_skipped_bits: int = int(val['leading_skipped_bits']['raw'])
        except:
            # If that's optimized out, parse the `ptype` for it.
            # This happens when compiling with Clang, even with `-gfull`.
            type: str = str(val.type.strip_typedefs())
            # Extract all numbers in the type signature.
            type_numbers = [int(s) for s in re.findall(r' \d+\b', type)]
            # The last number is the bitset's size.
            bits_count: int = type_numbers[-1]
            self.leading_skipped_bits: int = (
                self.array_len * self.element_size_bits - bits_count
            )

        return

    def _fmt_int(self, bytes_index: int):
        # Create a bitstring of `element_size_bits` length from the element at
        # `bytes_index`.
        return format(
            int(self.m_data[bytes_index]['raw']),
            '0' + str(self.element_size_bits) + 'b',
        )

    def _separate_bytes(self, bitstring: str):
        # Insert ' separators between every 8 bits.
        return "'".join([bitstring[i : i + 8] for i in range(0, len(bitstring), 8)])

    def to_string(self):
        bitstrings = [self._fmt_int(x) for x in range(self.array_len)]

        # Truncate right-padding bits from the last bytes in the array.
        if self.leading_skipped_bits > 0:
            bitstrings[-1] = bitstrings[-1][: -self.leading_skipped_bits]
        bitstrings = [
            self._separate_bytes(bitstring)
            for bitstring in bitstrings
            if len(bitstring) > 0
        ]

        bits_count = self.array_len * self.element_size_bits - self.leading_skipped_bits
        padding_suffix = ''
        if self.leading_skipped_bits > 0:
            padding_suffix = ', ' + str(self.leading_skipped_bits) + ' r-padding'
        return (
            '['
            + str(', '.join(bitstrings))
            + '] ('
            + str(bits_count)
            + ' bits'
            + padding_suffix
            + ')'
        )


sys.modules.pop('cat_allocator', None)
import cat_allocator  # noqa: E402

register_libcat_commands()


# At the end of the script, register all `cat_pretty_printers` simultaneously.
gdb.printing.register_pretty_printer(None, cat_pretty_printers, replace=True)
