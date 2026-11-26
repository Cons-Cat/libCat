#include <cat/array>
#include <cat/file>
#include <cat/file_path_inplace>

#include "../unit_tests.hpp"

namespace {

static_assert(cat::is_iterable<cat::file_path_inplace<8u>>);
static_assert(cat::is_stepanov_iterable<cat::file_path_inplace<8u>>);
static_assert(cat::is_path_segment<cat::file_path_inplace<8u>>);

constexpr auto constant_path =
   cat::make_file_path_inplace<16u>("/alpha/file.txt").verify();
static_assert(constant_path.filename() == "file.txt");
static_assert(constant_path.extension() == ".txt");

using native_view = cat::basic_file_path_view<cat::path_os::native>;
static_assert(
   cat::is_convertible<cat::file_path_inplace<8u> const&, native_view>
);
static_assert(
   cat::is_convertible<
      cat::basic_file_path_inplace<8u, cat::path_os::nix> const&, native_view>
);
static_assert(
   !cat::is_convertible<
      cat::basic_file_path_inplace<8u, cat::path_os::nt> const&, native_view>
);
static_assert(!cat::is_convertible<
              cat::basic_file_path_inplace<8u, cat::path_os::nix> const&,
              cat::file_path_view>);
static_assert(cat::is_convertible<cat::file_path const&, native_view>);

constexpr auto nt_path =
   cat::make_file_path_inplace<16u, cat::path_os::nt>(uR"(C:\alpha)").verify();
static_assert(nt_path.root_name() == u"C:");
static_assert(nt_path.filename() == u"alpha");
static_assert(cat::make_file_path_inplace<16u>(nt_path).verify().is_absolute());

}  // namespace

$test(path_inplace_join) {
   cat::file_path_inplace<12u> path;
   cat::verify(path.is_empty());
   cat::verify(path.data()[0] == '\0');

   path.append("alpha").verify();
   path.append("beta").verify();
   cat::verify(path == "alpha/beta");
   cat::verify(path.data()[path.size()] == '\0');
   path.append("").verify();
   cat::verify(path == "alpha/beta");

   // `"alpha/beta/c"` is 12 characters and fits exactly.
   path.append("c").verify();
   cat::verify(path == "alpha/beta/c");
   cat::verify(path.append("d").is_empty());
   cat::verify(path == "alpha/beta/c");

   path.append("/rooted").verify();
   cat::verify(path == "/rooted");
   cat::verify(path.append("/far/too/long/a/path").is_empty());
   cat::verify(path == "/rooted");

   cat::verify(cat::make_file_path_inplace<4u>("abcde").is_empty());
   auto from_path = cat::make_file_path_inplace<16u>(path).verify();
   cat::verify(from_path == path);

   auto nt = cat::make_file_path_inplace<32u, cat::path_os::nt>(u"C:").verify();
   nt.append(u"alpha").verify();
   cat::verify(nt == u"C:alpha");
   nt.append(uR"(\\server\share\x)").verify();
   cat::verify(nt == uR"(\\server\share\x)");
   nt.append(uR"(\y)").verify();
   cat::verify(nt == uR"(\\server\share\y)");
   nt.append(u"z").verify();
   cat::verify(nt == uR"(\\server\share\y\z)");
}

$test(path_view) {
   auto const path = cat::make_file_path_inplace<16u>("/alpha/b.txt").verify();
   cat::file_path_view const view = path;
   cat::verify(view == path.view());
   cat::verify(view.data() == path.data());
   cat::verify(view.size() == path.size());
   cat::verify(view.extension() == ".txt");
   cat::verify(view[1u] == 'a');

   cat::zstr_view const native = view;
   cat::verify(native.size() == path.size() + 1u);
   cat::verify(cat::fmt(g_tests_alloc, "{}", view).verify() == "/alpha/b.txt");

   auto joined = cat::make_file_path_inplace<32u>(view).verify();
   joined.append("c").verify();
   cat::verify(joined == "/alpha/b.txt/c");
}

$test(path_inplace_decomposition) {
   auto value =
      cat::make_file_path_inplace<32u>("/alpha//beta/file.tar.gz").verify();
   cat::verify(value.is_absolute());
   cat::verify(value.root_directory() == "/");
   cat::verify(value.parent_path() == "/alpha//beta");
   cat::verify(value.stem() == "file.tar");
   cat::verify(value.extension() == ".gz");

   cat::array<cat::str_view, 3u> expected = {"alpha", "beta", "file.tar.gz"};
   cat::idx index = 0u;
   for (cat::path_component component : value.components()) {
      if (component.kind == cat::path_component_kind::root_directory) {
         continue;
      }

      cat::verify(component.text == expected[index]);
      ++index;
   }
   cat::verify(index == 3u);

   cat::zstr_view const native = value;
   cat::verify(native.data() == value.data());
   cat::verify(native.size() == value.size() + 1u);

   auto copy = value;
   cat::verify(copy == value);
}

$test(path_inplace_format) {
   auto value = cat::make_file_path_inplace<8u>("a/b").verify();
   cat::verify(cat::fmt(g_tests_alloc, "{}", value).verify() == "a/b");
   cat::verify(cat::fmt(g_tests_alloc, "{:?}", value).verify() == R"("a/b")");
}

$test(path_inplace_open) {
   auto const path =
      cat::make_file_path_inplace<16u>("/proc/self/exe").verify();
   auto handle = cat::file_handle::open(path).verify();
   cat::verify(handle.is_open());
}
