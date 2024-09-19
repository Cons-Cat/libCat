import gdb


class PrinterControl(gdb.printing.PrettyPrinter):
    def __init__(self, type_name, printer):
        super().__init__(type_name)
        self.printer = printer

    def __call__(self, val):
        return self.printer(val) if val.type.name == self.name else None


# `cat_pretty_printers` stores the complete list of libCat pretty printers.
cat_pretty_printers = gdb.printing.RegexpCollectionPrettyPrinter('libCat')


def append_pretty_printer(namespace: str, type_name: str, printer):
    cat_pretty_printers.add_printer(
        namespace + '::' + type_name,
        '^' + namespace + '::' + type_name + '(<.*>)?$',
        printer,
    )


def cat_type(type_name: str, namespace: str = 'cat'):
    # Decorator for pretty printers.
    def _register_printer(printer):
        append_pretty_printer(namespace, type_name, printer)

    return _register_printer


@cat_type('monostate_type')
class MonostatePrinter:
    "Print a `cat::monostate`"

    def __init__(self, val: gdb.Value):
        return

    def to_string(self):
        return 'monostate'


@cat_type('arithmetic')
class ArithmeticPrinter:
    "Print a `cat::arithmetic`"

    def __init__(self, val: gdb.Value):
        self.raw = val['raw']

        type = val.type.strip_typedefs().name
        untyped_policy = type[-2:-1]
        typed_policy = gdb.Value(int(untyped_policy)).cast(
            gdb.lookup_type('cat::overflow_policies')
        )
        stripped_policy = str(typed_policy)[24:]
        match stripped_policy:
            case 'undefined':
                self.policy = 'undefined'
            case 'wrap' | 'wrap_member':
                self.policy = 'wrap'
            case 'saturate' | 'sat_member':
                self.policy = 'sat'
            case 'trap' | 'trap_member':
                self.policy = 'trap'
            case _:
                self.policy = 'WTF'

        return

    def to_string(self):
        if self.policy == 'undefined':
            return str(self.raw)
        return str(self.raw) + ' (' + self.policy + ')'


@cat_type('index')
class IndexPrinter:
    "Print a `cat::index`"

    def __init__(self, val: gdb.Value):
        self.raw = val['raw']
        return

    def to_string(self):
        return str(self.raw)

    
@cat_type('str_span')
class StrSpanPrinter:
    "Print a `cat::str_span`"

    def __init__(self, val: gdb.Value):
        self.m_p_data = val['m_p_data']
        self.m_size = val['m_size']['raw']
        return

    def to_string(self):
        p_str = self.m_p_data.cast(self.m_p_data.type
                                  .target()
                                  .strip_typedefs()
                                  .pointer())
        return p_str.lazy_string(length = self.m_size)


@cat_type('bitset')
class BitsetPrinter:
    "Print a `cat::bitset`"

    def __init__(self, val: gdb.Value):
        self.m_data: gdb.Value = val['m_data']['m_data']
        # self.bitset_size: int = val['bits_count']
        self.element_size_bits: int = self.m_data[0].type.sizeof * 8
        # Extract the length of this internal array.
        self.size: int = self.m_data.type.range()[1] + 1
        return

    def _fmt_int(self, bytes_index: int):
        # Create a bitstring of `element_size_bits` length from the element
        # at `bytes_index.
        bitstring: str = format(int(self.m_data[bytes_index]['raw']),
                                '0' + str(self.element_size_bits) + 'b')

        separated_bytes: str = \
            '\''.join([bitstring[::-1][i:i+8][::-1]
                       for i
                       in range(0, len(bitstring), 8)][::-1])
        
        return separated_bytes

    def to_string(self):
        bitstrings = [self._fmt_int(x)
                      for x
                      in range(self.size)]
        return '[' + str(', '.join(bitstrings)) + '] (' + str(127) + ' bits)'


# At the end of the script, register all `cat_pretty_printers` simultaneously.
gdb.printing.register_pretty_printer(None, cat_pretty_printers, replace=True)
