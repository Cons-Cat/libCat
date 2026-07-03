import gdb

cat_pretty_printers = gdb.printing.RegexpCollectionPrettyPrinter('libCat')


def append_pretty_printer(namespace: str, type_name: str, printer):
    cat_pretty_printers.add_printer(
        namespace + '::' + type_name,
        '^' + namespace + '::' + type_name + '(<.*>)?$',
        printer,
    )


def cat_type(type_name: str, namespace: str = 'cat'):
    def _register_printer(printer):
        append_pretty_printer(namespace, type_name, printer)
        return printer

    return _register_printer
