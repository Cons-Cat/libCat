. = ALIGN(4);
.init_array :
{
__init_array_start = .;
KEEP (*(.init_array*))
__init_array_end = .;
}
