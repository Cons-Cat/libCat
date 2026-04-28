set confirm off
set debuginfod enabled off
set pagination off
set print pretty off
set environment ASAN_OPTIONS detect_leaks=0

source gdb_pretty_printers/cat_printers.py

break cat_gdb_pretty_printer_breakpoint
run

python
import gdb
import re


green = "\033[32m"
red = "\033[31m"
bold = "\033[1m"
reset = "\033[0m"


def write_result(expr, actual, expected):
    color = green if expected else red
    gdb.write(f"  {bold}{color}{expr}{reset}:\n{actual}\n")


def expect_exact(expr, expected):
    actual = str(gdb.parse_and_eval(expr))
    write_result(expr, actual, actual == expected)
    if actual != expected:
        raise gdb.GdbError(
            f"{expr}: expected {expected!r}, got {actual!r}")


def expect_regex(expr, expected_pattern):
    actual = str(gdb.parse_and_eval(expr))
    expected = re.fullmatch(expected_pattern, actual) is not None
    write_result(expr, actual, expected)
    if not expected:
        raise gdb.GdbError(
            f"{expr}: expected pattern {expected_pattern!r}, got {actual!r}")


def pointer_pattern():
    return r"0x[0-9a-f]+(?: <[^>]+>)?"


def escaped_pointer_pattern():
    return r"0x[0-9a-f]+"


def char_sized_arithmetic_pattern(value, policy=""):
    suffix = "" if policy == "" else r" \(" + policy + r"\)"
    return str(value) + r"( '.+')?" + suffix


def group_ones(bits):
    bitstring = "1" * bits
    return "'".join(bitstring[index:index + 8]
                    for index in range(0, bits, 8))


def filled_bitset(bits, padding, groups=None):
    groups = [bits] if groups is None else groups
    bitstrings = ", ".join(group_ones(group) for group in groups)
    padding_suffix = "" if padding == 0 else f", {padding} r-padding"
    return f"[{bitstrings}] ({bits} bits{padding_suffix})"

print("Testing pretty printers:")

expect_exact("cat_gdb_monostate_value", "monostate")
expect_exact("cat_gdb_arithmetic_matrix_failures", "0")
expect_exact("cat_gdb_arithmetic_value", "42")
expect_exact("cat_gdb_arithmetic_wrap_value", "250 (wrap)")
expect_exact("cat_gdb_index_value", "7")
expect_regex("cat_gdb_int1_value", char_sized_arithmetic_pattern(65))
expect_regex("cat_gdb_wrap_int1_value",
             char_sized_arithmetic_pattern(65, "wrap"))
expect_regex("cat_gdb_sat_int1_value",
             char_sized_arithmetic_pattern(65, "sat"))
expect_regex("cat_gdb_uint1_value", char_sized_arithmetic_pattern(65))
expect_regex("cat_gdb_wrap_uint1_value",
             char_sized_arithmetic_pattern(65, "wrap"))
expect_regex("cat_gdb_sat_uint1_value",
             char_sized_arithmetic_pattern(65, "sat"))
expect_exact("cat_gdb_int2_value", "1200")
expect_exact("cat_gdb_wrap_int2_value", "1200 (wrap)")
expect_exact("cat_gdb_sat_int2_value", "1200 (sat)")
expect_exact("cat_gdb_uint2_value", "1200")
expect_exact("cat_gdb_wrap_uint2_value", "1200 (wrap)")
expect_exact("cat_gdb_sat_uint2_value", "1200 (sat)")
expect_exact("cat_gdb_int4_value", "120000")
expect_exact("cat_gdb_wrap_int4_value", "120000 (wrap)")
expect_exact("cat_gdb_sat_int4_value", "120000 (sat)")
expect_exact("cat_gdb_uint4_value", "120000")
expect_exact("cat_gdb_wrap_uint4_value", "120000 (wrap)")
expect_exact("cat_gdb_sat_uint4_value", "120000 (sat)")
expect_exact("cat_gdb_int8_value", "12000000000")
expect_exact("cat_gdb_wrap_int8_value", "12000000000 (wrap)")
expect_exact("cat_gdb_sat_int8_value", "12000000000 (sat)")
expect_exact("cat_gdb_uint8_value", "12000000000")
expect_exact("cat_gdb_wrap_uint8_value", "12000000000 (wrap)")
expect_exact("cat_gdb_sat_uint8_value", "12000000000 (sat)")
expect_exact("cat_gdb_iword_value", "12000000000")
expect_exact("cat_gdb_wrap_iword_value", "12000000000 (wrap)")
expect_exact("cat_gdb_sat_iword_value", "12000000000 (sat)")
expect_exact("cat_gdb_uword_value", "12000000000")
expect_exact("cat_gdb_wrap_uword_value", "12000000000 (wrap)")
expect_exact("cat_gdb_sat_uword_value", "12000000000 (sat)")
expect_exact("cat_gdb_idx_value", "12000000000")
expect_exact("cat_gdb_wrap_idx_value", "12000000000 (wrap)")
expect_exact("cat_gdb_sat_idx_value", "12000000000 (sat)")
expect_exact("cat_gdb_byte_value", "0xab")
expect_regex(
    "cat_gdb_span_value",
    r"\{" + pointer_pattern() + r", 3\} \[\n"
    + r"  \[0\] = 1,\n"
    + r"  \[1\] = 2,\n"
    + r"  \[2\] = 3\n"
    + r"\]")
expect_exact("cat_gdb_array_value", "[4, 5, 6] (size: 3)")
expect_regex(
    "cat_gdb_str_span_value",
    r'"hi\\0!" \(4 chars, ' + escaped_pointer_pattern() + r"\)")
expect_exact("cat_gdb_str_inplace_value", '"hello" (5 chars)')
expect_regex(
    "cat_gdb_zstr_span_value",
    r'"zip\\0" \(4 chars, null-terminated, '
    + escaped_pointer_pattern()
    + r"\)")
expect_regex(
    "cat_gdb_zstr_view_value",
    r'"view\\0" \(5 chars, null-terminated, '
    + escaped_pointer_pattern()
    + r"\)")
expect_exact("cat_gdb_zstr_inplace_value",
             '"hello\\0" (6 chars, null-terminated)')
expect_exact("cat_gdb_zstr_padded_inplace_value",
             '"hi\\0\\0\\0\\0" (6 chars, null-terminated)')
expect_exact("cat_gdb_bit_value", "1")
expect_exact("cat_gdb_bitset_value", "[01010101'01] (10 bits, 6 r-padding)")
expect_exact("cat_gdb_bitset6_value", filled_bitset(6, 2))
expect_exact("cat_gdb_bitset8_value", filled_bitset(8, 0))
expect_exact("cat_gdb_bitset16_value", filled_bitset(16, 0))
expect_exact("cat_gdb_bitset17_value", filled_bitset(17, 15))
expect_exact("cat_gdb_bitset24_value", filled_bitset(24, 8))
expect_exact("cat_gdb_bitset25_value", filled_bitset(25, 7))
expect_exact("cat_gdb_bitset32_value", filled_bitset(32, 0))
expect_exact("cat_gdb_bitset33_value", filled_bitset(33, 31))
expect_exact("cat_gdb_bitset40_value", filled_bitset(40, 24))
expect_exact("cat_gdb_bitset64_value", filled_bitset(64, 0))
expect_exact("cat_gdb_bitset65_value", filled_bitset(65, 63, [64, 1]))
expect_exact("cat_gdb_bitset128_value", filled_bitset(128, 0, [64, 64]))
end

quit
