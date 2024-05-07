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

    def __init__(self, val):
        return

    def to_string(self):
        return 'monostate'


# At the end of the script, register all `cat_pretty_printers` simultaneously.
gdb.printing.register_pretty_printer(None, cat_pretty_printers, replace=True)
