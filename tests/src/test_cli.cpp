#include <cat/allocator_linear>
#include <cat/cli>
#include <cat/string>

#include "../unit_tests.hpp"

namespace {

char const* const g_argv[] = {"program", nullptr};

using empty_cli = decltype(cat::cli(1, g_argv));
using file_cli = decltype(cat::cli(1, g_argv).add_flag<"f,file">());

static_assert(cat::is_same<empty_cli, cat::cli<>>);
static_assert(file_cli::flag_count == 1u);

// Lazy parsing only holds `argv`.
static_assert(sizeof(file_cli) == 2u * sizeof(void*));

// Types that allocate must be given an allocator.
template <typename T>
concept can_parse_flag =
   requires(file_cli const& cli) { cli.parse<T>("file"); };

static_assert(can_parse_flag<int4>);
static_assert(can_parse_flag<cat::str_inplace<8u>>);

}  // namespace

$test(cli_parse_values) {
   char const* argv[] = {
      "program", "-f",  "in.txt", "--number=4", "--size",
      "12",      "-o8", "--",     "--count=1",  nullptr,
   };
   int const argc = 9;

   auto cli = cat::cli(argc, argv)
                 .add_flag<"f,file">()
                 .add_flag<"number">()
                 .add_flag<"s,size">()
                 .add_flag<"o,output">()
                 .add_flag<"num">()
                 .add_flag<"count">();

   cat::verify(cli.parse<int4>("number").value() == 4);
   cat::verify(cli.parse<int4>("size").value() == 12);
   cat::verify(cli.parse<int4>("s").value() == 12);
   cat::verify(cli.parse<int4>("o").value() == 8);
   cat::verify(cli.parse<cat::str_inplace<16u>>("file").value() == "in.txt");

   // `--number=4` is not `--num`.
   cat::verify(cli.parse<int4>("num").is_empty());
   // `--count=1` follows `--`.
   cat::verify(cli.parse<int4>("count").is_empty());
   // `in.txt` does not scan as an integer.
   cat::verify(cli.parse<int4>("f").is_empty());
}

$test(cli_skips_values_of_other_flags) {
   // `--number` is the value of `--file`, not a flag.
   char const* argv[] = {"program", "--file", "--number", "-n", "3", nullptr};
   int const argc = 5;

   auto cli = cat::cli(argc, argv).add_flag<"f,file">().add_flag<"n,number">();

   cat::verify(cli.parse<cat::str_inplace<16u>>("file").value() == "--number");
   cat::verify(cli.parse<int4>("number").value() == 3);
}

$test(cli_repeated_and_missing_values) {
   char const* argv[] = {"program", "-n1",     "--number", "2",
                         "--size=", "--empty", nullptr};
   int const argc = 6;

   auto cli = cat::cli(argc, argv)
                 .add_flag<"n,number">()
                 .add_flag<"size">()
                 .add_flag<"empty">()
                 .add_flag<"absent">();

   // The last occurrence wins.
   cat::verify(cli.parse<int4>("n").value() == 2);
   // `--size=` has an empty value, which is still a value.
   cat::verify(cli.parse<cat::str_inplace<4u>>("size").value() == "");
   cat::verify(cli.parse<int4>("size").is_empty());
   // `--empty` is the last argument, so it has no value.
   cat::verify(cli.parse<cat::str_inplace<4u>>("empty").is_empty());
   cat::verify(cli.parse<int4>("absent").is_empty());
}

$test(cli_positional_keeps_earlier_values) {
   char const* argv[] = {"program", "-n", "3", "file.txt", "-m", "4", nullptr};
   auto cli = cat::cli(6, argv).add_flag<"n">().add_flag<"m">();
   cat::verify(cli.parse<int4>("n").verify() == 3);
   // Flags after a positional argument are not parsed yet.
   cat::verify(cli.parse<int4>("m").is_empty());
}

$test(cli_skips_program_name) {
   char const* argv[] = {"--number=1", nullptr};
   auto cli = cat::cli(1, argv).add_flag<"number">();
   cat::verify(cli.parse<int4>("number").is_empty());

   // `cli` is usable in constant evaluation.
   static constexpr char const* constant_argv[] = {
      "program", "--number", "5", nullptr
   };
   static_assert(
      cat::cli(3, constant_argv)
         .add_flag<"number">()
         .parse<int4>("number")
         .value()
      == 5
   );
}

namespace {

using versioned = decltype(cat::cli(1, g_argv)
                              .add_flag<"f,file">()
                              .add_version<cat::version{1, 2, "rc.1"}>());

static_assert(versioned::version_text == "1.2.rc.1");
static_assert(sizeof(versioned) == sizeof(file_cli));

// The version can be a braced list.
static_assert(
   decltype(cat::cli(1, g_argv).add_version<{1, 2, "rc.1"}>())::version_text
   == "1.2.rc.1"
);

// No flags can follow `.add_version()`.
template <typename T>
concept can_add_flag = requires(T cli) { cli.template add_flag<"n">(); };

static_assert(can_add_flag<file_cli>);
static_assert(!can_add_flag<versioned>);

// A registered flag keeps any name that the version flag would spell.
using verbose_cli = decltype(cat::cli(1, g_argv)
                                .add_flag<"v,verbose">()
                                .add_version<cat::version{1}>());

static_assert(verbose_cli::version_names.short_name == '\0');
static_assert(verbose_cli::version_names.long_name == "version");

using respelled_cli = decltype(cat::cli(1, g_argv)
                                  .add_flag<"version">()
                                  .add_version<cat::version{1}, "V,version">());

static_assert(respelled_cli::version_names.short_name == 'V');
static_assert(respelled_cli::version_names.long_name.is_empty());

template <
   cat::detail::cli_names const& names, cat::basic_str_literal... spellings>
constexpr auto
requests_version(auto const&... arguments) -> bool {
   char const* const argv[] = {arguments...};
   return cat::detail::cli_has_bool_flag<names, spellings...>(
      cat::span<char const* const>(argv, sizeof...(arguments))
   );
}

constexpr auto const& default_names = versioned::version_names;

static_assert(requests_version<default_names, "f,file">("--version"));
static_assert(requests_version<default_names, "f,file">("-f", "x", "-v"));
// `-v` is the value of `-f`.
static_assert(!requests_version<default_names, "f,file">("-f", "-v"));
static_assert(!requests_version<default_names, "f,file">("--", "-v"));
static_assert(!requests_version<default_names, "f,file">("--version=1"));
static_assert(!requests_version<default_names, "f,file">("-vv"));
static_assert(!requests_version<verbose_cli::version_names, "v,verbose">("-v"));

}  // namespace

$test(cli_version_without_flag) {
   char const* argv[] = {"program", "-n", "3", nullptr};
   auto cli = cat::cli(3, argv).add_flag<"n">().add_version<cat::version{1}>();
   cat::verify(cli.parse<int4>("n").value() == 3);
}

$test(cli_bool_flags) {
   char const* argv[] = {"program", "-ab",           "--enable", "-n",
                         "3",       "--quiet=false", "-xyz",     nullptr};
   int const argc = 7;

   auto cli = cat::cli(argc, argv)
                 .add_bool<"a">()
                 .add_bool<"b">()
                 .add_bool<"c">()
                 .add_bool<"e,enable">()
                 .add_bool<"q,quiet">()
                 .add_bool<"x">()
                 .add_flag<"n,number">();

   cat::verify(cli.parse<bool>("a").verify());
   cat::verify(cli.parse<bool>("b").verify());
   cat::verify(cli.parse<bool>("c").is_empty());
   // `--enable` does not take `-n` as its value.
   cat::verify(cli.parse<bool>("e").verify());
   cat::verify(cli.parse<int4>("number").verify() == 3);
   cat::verify(!cli.parse<bool>("quiet").verify());
   // `y` and `z` are not bool flags, so `-xyz` is not a cluster.
   cat::verify(cli.parse<bool>("x").is_empty());
}

$test(cli_bool_flag_after_value_flag) {
   // `-e` is the value of `-f`, and a bool flag does not step over `-f`.
   char const* argv[] = {"program", "-f", "-e", "-e", "-f", "x", nullptr};
   auto cli = cat::cli(6, argv).add_flag<"f">().add_bool<"e">();
   cat::verify(cli.parse<bool>("e").verify());
   cat::verify(cli.parse<cat::str_inplace<4u>>("f").verify() == "x");
}

$test(cli_bool_flag_takes_no_attached_value) {
   char const* argv[] = {"program", "-o1", nullptr};
   auto cli = cat::cli(2, argv).add_bool<"o">();
   cat::verify(cli.parse<bool>("o").is_empty());
}

namespace {

// How `=` is handled in every spelling.
constexpr char const* equals_in_value[] = {"program", "--file=a=b", nullptr};
static_assert(
   cat::cli(2, equals_in_value)
      .add_flag<"f,file">()
      .parse<cat::str_inplace<4u>>("file")
      .verify()
   == "a=b"
);

// A separate value is taken whole.
constexpr char const* separate_equals[] = {"program", "--file", "=x", nullptr};
static_assert(
   cat::cli(3, separate_equals)
      .add_flag<"f,file">()
      .parse<cat::str_inplace<4u>>("file")
      .verify()
   == "=x"
);

// A short flag takes everything after it, like `getopt()`.
constexpr char const* short_equals[] = {"program", "-f=x", nullptr};
static_assert(
   cat::cli(2, short_equals)
      .add_flag<"f,file">()
      .parse<cat::str_inplace<4u>>("f")
      .verify()
   == "=x"
);

// An empty bool value is not a bool.
constexpr char const* empty_bool[] = {"program", "--enable=", nullptr};
static_assert(cat::cli(2, empty_bool)
                 .add_bool<"e,enable">()
                 .parse<bool>("enable")
                 .is_empty());

// Every libCat bool parses a bool flag.
constexpr char const* wide_bools[] = {"program", "-e", "--quiet=false", nullptr};
static_assert(cat::cli(3, wide_bools)
                 .add_bool<"e,enable">()
                 .parse<bool2>("enable")
                 .verify());
static_assert(!cat::cli(3, wide_bools)
                 .add_bool<"q,quiet">()
                 .parse<bool4>("quiet")
                 .verify());

// A short bool flag never takes a value.
constexpr char const* short_bool_equals[] = {"program", "-e=false", nullptr};
static_assert(cat::cli(2, short_bool_equals)
                 .add_bool<"e,enable">()
                 .parse<bool>("enable")
                 .is_empty());

}  // namespace
