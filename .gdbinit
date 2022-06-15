# This `.gdbinit` is to be symlinked in all build directories.
# i.e. `ln build/examples/.gdbinit` and `build/tests/Debug/`
break exit

# Set a breakpoint at `exit()`, and define a `meow` command
# to enter a `meow()` symbol.
define meow
  starti
  tbreak meow
  c
end

