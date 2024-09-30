import gdb
import re


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

    
@cat_type('byte')
class BytePrinter:
    "Print a `cat::index`"

    def __init__(self, val: gdb.Value):
        self.value = val['value']
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
        p_data = self.m_p_data.cast(self.m_p_data.type
                                  .target()
                                  .strip_typedefs()
                                  .pointer())

        array: str = ',\n'.join(['  ['  + str(x) + '] = ' + str(p_data[x])
                                for x
                                in range(self.m_size)])
        
        return '{' + str(p_data) + ', ' + str(self.m_size) + '}' + ' [\n' \
            + array \
            + '\n]'

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


@cat_type('bit_value')
class BitValuePrinter:
    "Print a `cat::bit_value`"

    def __init__(self, val: gdb.Value):
        self.m_value: gdb.Value = val['m_value']

    def to_string(self):
        return "1" if bool(self.m_value) else "0"
    

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
            type: str = val.type.strip_typedefs().tag
            # Extract all numbers in the type signature.
            type_numbers = [int(s) for s in re.findall(r'\b\d+\b', type)]
            # The last number is the bitset's size.
            bits_count: int = type_numbers[-1]
            self.leading_skipped_bits: int = \
                self.array_len * self.element_size_bits - bits_count

        return

    def _fmt_int(self, bytes_index: int):
        # Create a bitstring of `element_size_bits` length from the element
        # at `bytes_index.
        bitstring: str = format(int(self.m_data[bytes_index]['raw']),
                                '0' + str(self.element_size_bits) + 'b')

        # Insert ' separators between every 8 bits.
        separated_bytes: str = \
            '\''.join([bitstring[::-1][i:i+8][::-1]
                       for i
                       in range(0, len(bitstring), 8)][::-1])
        
        return separated_bytes

    def to_string(self):
        bitstrings = [self._fmt_int(x)
                      for x
                      in range(self.array_len)]
        
        # Truncate leading skipped bits from the last bytes in the array.
        if self.leading_skipped_bits > 0:
            bitstrings[-1] = bitstrings[-1][:-self.leading_skipped_bits]

        return '[' + str(', '.join(bitstrings)) + '] (' \
            + str(self.array_len * self.element_size_bits - self.leading_skipped_bits) \
            + ' bits)'


# At the end of the script, register all `cat_pretty_printers` simultaneously.
gdb.printing.register_pretty_printer(None, cat_pretty_printers, replace=True)
