# This `.gdbinit` is to be symlinked in all build directories.
# i.e. `ln build/examples/.gdbinit` and `build/tests/Debug/`
break exit
break cat::default_assert_handler
break test_fail

source /home/conscat/src/libcat/gdb_pretty_printers/cat_printers.py

# Skip stepping into uninteresting code.
# skip file meta
# skip file numerals
# skip file utility
# skip file notype
# skip file bit
# skip file compare
