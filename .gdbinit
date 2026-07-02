# Symlinked next to build outputs by CMake (`cat_gdb_symlink`).
# Requires: set auto-load local-gdbinit on   (in ~/.gdbinit, once)
break exit
break cat::default_assert_handler
break test_fail

# Find a pretty printers GDB script and evaluate it automatically.
python
import os
import gdb

_root = os.path.dirname(os.path.realpath(os.path.join(os.getcwd(), '.gdbinit')))
_printers = os.path.join(_root, 'gdb_pretty_printers', 'cat_printers.py')
if os.path.isfile(_printers):
    gdb.execute(f'source {_printers}')
else:
    gdb.write(f'libCat: missing {_printers}\n')
end
