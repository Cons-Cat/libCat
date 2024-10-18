# This `.gdbinit` is to be symlinked in all build directories.
# i.e. `ln build/examples/.gdbinit` and `build/tests/Debug/`
break exit
break cat::default_assert_handler
break test_fail

# TODO: Do not hard-code this path.
source /home/conscat/src/libcat/gdb_pretty_printers/cat_printers.py

# Skip stepping into uninteresting code.
# This is necessary because Clang cannot compile `artificial` methods
# if they are also `friend` or `static`.
skip file debug
skip file compare
skip file arithmetic
skip file arithmetic_interface
