#include <cat/allocator_page>
#include <cat/file_path>

#include "../unit_tests.hpp"

namespace {

static_assert(cat::detail::path_find_first_equal("abc/", '/').value() == 3u);
static_assert(cat::detail::path_find_last_equal("a/b/c", '/').value() == 3u);
static_assert(cat::is_iterable<cat::file_path>);
static_assert(cat::is_stepanov_iterable<cat::file_path>);
static_assert(cat::is_iterable<cat::path_components>);
static_assert(cat::is_stepanov_iterable<cat::path_components>);
static_assert(
   cat::is_forward_stepanov_iterator<cat::path_components::iterator>
);

template <cat::is_path_segment Segment>
void
verify_path_string_type(Segment const& segment) {
   auto manual = cat::make_file_path(g_tests_alloc, segment).verify();
   $defer {
      manual.free(g_tests_alloc);
   };

   cat::verify(manual == "leaf");

   auto managed = cat::raii::make_file_path(g_tests_alloc, segment).verify();
   cat::verify(managed == "leaf");

   auto base = cat::raii::make_file_path(g_tests_alloc, "root").verify();
   auto joined = (base / segment).verify();
   cat::verify(joined == "root/leaf");
}

}  // namespace

$test(path_maybe_niche) {
   static_assert(sizeof(cat::maybe<cat::file_path>) == sizeof(cat::file_path));
   static_assert(
      sizeof(cat::maybe<cat::raii::file_path<>>)
      == sizeof(cat::raii::file_path<>)
   );

   cat::maybe<cat::file_path> manual_empty;
   cat::maybe<cat::raii::file_path<>> raii_empty;
   cat::verify(manual_empty.is_empty());
   cat::verify(raii_empty.is_empty());

   auto manual = cat::make_file_path(g_tests_alloc, "manual");
   $defer {
      manual.value().free(g_tests_alloc);
   };

   cat::verify(manual.has_value());
   cat::verify(manual.value() == "manual");

   auto managed = cat::raii::make_file_path(g_tests_alloc, "managed");
   cat::verify(managed.has_value());
   cat::verify(managed.value() == "managed");
}

$test(path_flags_and_factories) {
   constexpr auto custom_growth = cat::vec_flags::initial_growth(64u);

   auto manual = cat::make_file_path(g_tests_alloc, "alpha").verify();
   cat::verify(manual == "alpha");
   cat::verify(manual.capacity() == 255u);
   cat::verify(manual.data()[manual.size()] == '\0');
   cat::zstr_view native = manual;
   cat::verify(native == "alpha");

   char text[] = "beta";
   char const* p_text = text;
   auto from_pointer = cat::make_file_path(g_tests_alloc, p_text).verify();
   cat::verify(from_pointer == "beta");

   auto managed =
      cat::raii::make_file_path(g_tests_alloc, cat::str_view("gamma")).verify();
   cat::verify(managed == "gamma");
   cat::verify(managed.data()[managed.size()] == '\0');
   cat::file_path released = managed.release();
   auto custom =
      cat::make_file_path<
         cat::allocator_musl_shared, cat::path_os::portable, custom_growth>(
         g_tests_alloc, "delta"
      )
         .verify();
   $defer {
      custom.free(g_tests_alloc);
      released.free(g_tests_alloc);
      from_pointer.free(g_tests_alloc);
      manual.free(g_tests_alloc);
   };

   cat::verify(released == "gamma");
   cat::verify(custom.capacity() >= 64u);
}

$test(path_string_type_interop) {
   char mutable_array[] = "leaf";
   char const_array[] = "leaf";
   char* p_mutable = mutable_array;
   char const* p_const = const_array;
   cat::str_view view(p_const, 4u);
   cat::zstr_view zview(p_const, 5u);
   cat::str_span span(mutable_array, 4u);
   cat::zstr_span zspan(mutable_array, 5u);
   constexpr cat::basic_str_literal literal = "leaf";
   cat::str_inplace inplace = "leaf";
   cat::zstr_inplace zinplace = cat::make_zstr_inplace<4u>("leaf");
   auto vector = cat::make_str_vec(g_tests_alloc, "leaf").verify();
   auto zvector = cat::make_zstr_vec(g_tests_alloc, "leaf").verify();
   $defer {
      zvector.free(g_tests_alloc);
      vector.free(g_tests_alloc);
   };

   auto raii_vector = cat::raii::make_str_vec(g_tests_alloc, "leaf").verify();
   auto raii_zvector = cat::raii::make_zstr_vec(g_tests_alloc, "leaf").verify();

   verify_path_string_type("leaf");
   verify_path_string_type(mutable_array);
   verify_path_string_type(const_array);
   verify_path_string_type(p_mutable);
   verify_path_string_type(p_const);
   verify_path_string_type(view);
   verify_path_string_type(zview);
   verify_path_string_type(span);
   verify_path_string_type(zspan);
   verify_path_string_type(literal);
   verify_path_string_type(inplace);
   verify_path_string_type(zinplace);
   verify_path_string_type(vector);
   verify_path_string_type(zvector);
   verify_path_string_type(raii_vector);
   verify_path_string_type(raii_zvector);
}

$test(path_join) {
   auto manual = cat::make_file_path(g_tests_alloc, "alpha").verify();
   manual.append(g_tests_alloc, "beta").verify();
   cat::verify(manual == "alpha/beta");
   manual.append(g_tests_alloc, "").verify();
   cat::verify(manual == "alpha/beta");
   manual.append(g_tests_alloc, "/rooted").verify();
   cat::verify(manual == "/rooted");

   auto joined =
      cat::make_file_path_joined(g_tests_alloc, manual, "leaf").verify();
   $defer {
      joined.free(g_tests_alloc);
      manual.free(g_tests_alloc);
   };

   cat::verify(joined == "/rooted/leaf");

   auto managed = cat::raii::make_file_path(g_tests_alloc, "one").verify();
   auto managed_joined = (managed / "two").verify();
   cat::verify(managed == "one");
   cat::verify(managed_joined == "one/two");
   managed_joined.append("/replacement").verify();
   cat::verify(managed_joined == "/replacement");
}

$test(path_decomposition_and_components) {
   auto value =
      cat::raii::make_file_path(g_tests_alloc, "/alpha//beta/file.tar.gz")
         .verify();
   cat::verify(value.is_absolute());
   cat::verify(!value.is_relative());
   cat::verify(value.root_name().size() == 0u);
   cat::verify(value.root_directory() == "/");
   cat::verify(value.root_path() == "/");
   cat::verify(value.relative_path() == "alpha//beta/file.tar.gz");
   cat::verify(value.parent_path() == "/alpha//beta");
   cat::verify(value.filename() == "file.tar.gz");
   cat::verify(value.stem() == "file.tar");
   cat::verify(value.extension() == ".gz");
   cat::verify(!value.has_root_name());
   cat::verify(value.has_root_directory());
   cat::verify(value.has_relative_path());
   cat::verify(value.has_parent_path());
   cat::verify(value.has_filename());
   cat::verify(value.has_stem());
   cat::verify(value.has_extension());

   cat::str_view expected[] = {"alpha", "beta", "file.tar.gz"};
   cat::idx index = 0u;
   for (cat::str_view component : value.components()) {
      cat::verify(index < 3u);
      cat::verify(component == expected[index]);
      ++index;
   }

   cat::verify(index == 3u);

   cat::idx character_count = 0u;
   cat::iterate(value).run_while([&](char character) {
      cat::verify(character == value[character_count]);
      ++character_count;
      return true;
   });

   cat::verify(character_count == value.size());

   auto components = value.components();
   index = 0u;
   cat::iterate(components).run_while([&](cat::str_view component) {
      cat::verify(component == expected[index]);
      ++index;
      return true;
   });

   cat::verify(index == 3u);

   auto dots =
      cat::raii::make_file_path(g_tests_alloc, "a/./../.profile").verify();
   cat::str_view dot_expected[] = {"a", ".", "..", ".profile"};
   index = 0u;

   for (cat::str_view component : dots.components()) {
      cat::verify(component == dot_expected[index]);
      ++index;
   }

   cat::verify(index == 4u);
   cat::verify(dots.stem() == ".profile");
   cat::verify(dots.extension().size() == 0u);
}

$test(path_simd_scan_boundaries) {
   char text[256];
   cat::idx length = 0u;
   for (cat::idx index = 0u; index < 33u; ++index) {
      text[length] = '/';
      ++length;
   }

   for (cat::idx index = 0u; index < 65u; ++index) {
      text[length] = 'a';
      ++length;
   }

   for (cat::idx index = 0u; index < 31u; ++index) {
      text[length] = '/';
      ++length;
   }

   for (cat::idx index = 0u; index < 67u; ++index) {
      text[length] = 'b';
      ++length;
   }

   for (char character : cat::str_view(".tar.gz")) {
      text[length] = character;
      ++length;
   }

   auto value =
      cat::raii::make_file_path(g_tests_alloc, cat::str_view(text, length))
         .verify();
   cat::verify(value.size() == 203u);
   cat::verify(value.relative_path().size() == 170u);
   cat::verify(value.parent_path().size() == 98u);
   cat::verify(value.filename().size() == 74u);
   cat::verify(value.stem().size() == 71u);
   cat::verify(value.extension() == ".gz");

   cat::idx component = 0u;
   for (cat::str_view part : value.components()) {
      if (component == 0u) {
         cat::verify(part.size() == 65u);
      } else {
         cat::verify(component == 1u);
         cat::verify(part.size() == 74u);
      }

      ++component;
   }

   cat::verify(component == 2u);

   char slashes[65];
   for (char& character : slashes) {
      character = '/';
   }

   auto root =
      cat::raii::make_file_path(g_tests_alloc, cat::str_view(slashes, 65u))
         .verify();
   cat::verify(root.relative_path().size() == 0u);
   cat::verify(root.parent_path() == "/");
   cat::verify(root.filename().size() == 0u);
   cat::verify(root.components().begin() == root.components().end());

   char trailing[106];
   trailing[0] = '/';
   for (cat::idx index = 1u; index < 66u; ++index) {
      trailing[index] = 'x';
   }

   for (cat::idx index = 66u; index < 106u; ++index) {
      trailing[index] = '/';
   }

   auto with_trailing =
      cat::raii::make_file_path(g_tests_alloc, cat::str_view(trailing, 106u))
         .verify();
   cat::verify(with_trailing.parent_path() == "/");
   cat::verify(with_trailing.filename().size() == 0u);
}

$test(path_formatter) {
   auto manual = cat::make_file_path(g_tests_alloc, "alpha/beta").verify();
   $defer {
      manual.free(g_tests_alloc);
   };

   cat::verify(cat::fmt(g_tests_alloc, "{}", manual).verify() == "alpha/beta");
   cat::verify(
      cat::fmt(g_tests_alloc, "{:g}", manual).verify() == "alpha/beta"
   );
   cat::verify(
      cat::fmt(g_tests_alloc, "{:?}", manual).verify() == R"("alpha/beta")"
   );
   cat::verify(
      cat::fmt(g_tests_alloc, "{:?g}", manual).verify() == R"("alpha/beta")"
   );

   auto managed = cat::raii::make_file_path(g_tests_alloc, "a\nb").verify();
   cat::verify(cat::fmt(g_tests_alloc, "{}", managed).verify() == "a\nb");
   cat::verify(
      cat::fmt(g_tests_alloc, "{:?}", managed).verify() == R"("a\nb")"
   );
}

$test(path_current_absolute_and_unique) {
   auto cwd = cat::make_file_path_current(g_tests_alloc).verify();
   cat::verify(cwd.is_absolute());
   cat::verify(!cwd.is_empty());
   cat::verify(cwd.data()[cwd.size()] == '\0');

   auto explicit_base =
      cat::make_file_path_absolute("child", "/tmp/base", g_tests_alloc)
         .verify();
   cat::verify(explicit_base == "/tmp/base/child");

   auto relative_base =
      cat::make_file_path_absolute("child", "base", g_tests_alloc).verify();
   auto expected =
      cat::make_file_path_joined(g_tests_alloc, cwd, "base").verify();
   expected.append(g_tests_alloc, "child").verify();
   cat::verify(relative_base == expected);

   auto absolute_input = cat::make_file_path_absolute(
                            "/already/absolute", "/ignored", g_tests_alloc
   )
                            .verify();
   cat::verify(absolute_input == "/already/absolute");

   auto unchanged =
      cat::make_file_path_unique(g_tests_alloc, "fixed-name").verify();
   cat::verify(unchanged == "fixed-name");

   auto unique =
      cat::make_file_path_unique(g_tests_alloc, "tmp-%%%%-%%").verify();
   $defer {
      unique.free(g_tests_alloc);
      unchanged.free(g_tests_alloc);
      absolute_input.free(g_tests_alloc);
      expected.free(g_tests_alloc);
      relative_base.free(g_tests_alloc);
      explicit_base.free(g_tests_alloc);
      cwd.free(g_tests_alloc);
   };

   cat::verify(unique.size() == 11u);
   cat::verify(cat::str_view(unique.data(), 4u) == "tmp-");
   cat::verify(unique.data()[8] == '-');

   for (cat::idx index = 4u; index < unique.size(); ++index) {
      if (index == 8u) {
         continue;
      }

      char const value = unique.data()[index];
      cat::verify(
         (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f')
      );
   }

   char model[66];
   for (char& character : model) {
      character = 'q';
   }

   constexpr cat::idx percent_positions[] = {15u, 16u, 31u, 32u, 63u, 64u};
   for (cat::idx position : percent_positions) {
      model[position] = '%';
   }

   auto boundary_unique =
      cat::make_file_path_unique(g_tests_alloc, cat::str_view(model, 66u))
         .verify();
   $defer {
      boundary_unique.free(g_tests_alloc);
   };
   for (cat::idx index = 0u; index < boundary_unique.size(); ++index) {
      char const value = boundary_unique.data()[index];
      bool was_percent = false;
      for (cat::idx position : percent_positions) {
         was_percent |= index == position;
      }

      if (was_percent) {
         cat::verify(
            (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f')
         );
      } else {
         cat::verify(value == 'q');
      }
   }
}

namespace {

using nt_path =
   cat::raii::basic_file_path<cat::allocator_dyn, cat::path_os::nt>;

static_assert(cat::path_os::native == cat::path_os::nix);
static_assert(cat::detail::path_find_first_equal("ab\\c/", '/').value() == 4u);
static_assert(
   cat::detail::path_find_first_separator(
      "ab\\c/", cat::detail::path_nt_separators
   )
      .value()
   == 2u
);
static_assert(
   cat::detail::path_find_last_not_separator(
      "a\\/\\/", cat::detail::path_nt_separators
   )
      .value()
   == 0u
);

// Check the root of an `nt` path at compile time.
consteval auto
nt_root(
   cat::str_view value, cat::str_view name, cat::str_view directory,
   bool is_absolute
) -> bool {
   cat::detail::path_root const root =
      cat::detail::path_parse_root<cat::path_os::nt>(value);
   return cat::detail::path_slice(value, 0u, root.name_end) == name
          && cat::detail::path_slice(value, root.name_end, root.end)
                == directory
          && root.is_absolute == is_absolute;
}

static_assert(nt_root("", "", "", false));
static_assert(nt_root("foo\\bar", "", "", false));
static_assert(nt_root("\\foo", "", "\\", false));
static_assert(nt_root("/foo", "", "/", false));
static_assert(nt_root("C:", "C:", "", false));
static_assert(nt_root("c:foo", "c:", "", false));
static_assert(nt_root("C:\\foo", "C:", "\\", true));
static_assert(nt_root("C:/foo", "C:", "/", true));
static_assert(nt_root("1:\\foo", "", "", false));
static_assert(nt_root("\\\\server\\share", "\\\\server\\share", "", true));
static_assert(nt_root("//server/share/x", "//server/share", "/", true));
// Without a share name, `\\server` is only a root directory and a name.
static_assert(nt_root("\\\\server", "", "\\", false));
static_assert(nt_root("\\\\server\\", "", "\\", false));
static_assert(nt_root("\\\\.\\COM1", "\\\\.\\COM1", "", true));
static_assert(nt_root("//./COM1/x", "//./COM1", "/", true));
static_assert(nt_root("\\\\?\\C:", "\\\\?\\C:", "", true));
static_assert(nt_root("\\\\?\\C:\\x", "\\\\?\\C:", "\\", true));
// `/` is not a separator in verbatim paths.
static_assert(nt_root("\\\\?\\C:/x", "\\\\?\\C:/x", "", true));
static_assert(nt_root(
   "\\\\?\\UNC\\server\\share\\x", "\\\\?\\UNC\\server\\share", "\\", true
));
static_assert(nt_root("\\\\?\\pictures\\x", "\\\\?\\pictures", "\\", true));
// A verbatim prefix spelled with `/` is a share named `?`.
static_assert(nt_root("//?/C:/x", "//?/C:", "/", true));

}  // namespace

$test(path_nt_decomposition) {
   auto drive = cat::raii::make_file_path<cat::path_os::nt>(
                   g_tests_alloc, "C:\\alpha/beta\\\\file.tar.gz"
   )
                   .verify();
   static_assert(cat::is_same<__typeof_unqual(drive), nt_path>);
   cat::verify(drive.is_absolute());
   cat::verify(drive.root_name() == "C:");
   cat::verify(drive.root_directory() == "\\");
   cat::verify(drive.root_path() == "C:\\");
   cat::verify(drive.relative_path() == "alpha/beta\\\\file.tar.gz");
   cat::verify(drive.parent_path() == "C:\\alpha/beta");
   cat::verify(drive.filename() == "file.tar.gz");
   cat::verify(drive.stem() == "file.tar");
   cat::verify(drive.extension() == ".gz");
   cat::verify(drive.has_root_name());
   cat::verify(drive.has_root_directory());

   cat::str_view expected[] = {"alpha", "beta", "file.tar.gz"};
   cat::idx index = 0u;
   for (cat::str_view component : drive.components()) {
      cat::verify(index < 3u);
      cat::verify(component == expected[index]);
      ++index;
   }

   cat::verify(index == 3u);

   auto drive_relative =
      cat::raii::make_file_path<cat::path_os::nt>(g_tests_alloc, "C:foo")
         .verify();
   cat::verify(drive_relative.is_relative());
   cat::verify(drive_relative.root_name() == "C:");
   cat::verify(!drive_relative.has_root_directory());
   cat::verify(drive_relative.relative_path() == "foo");
   cat::verify(drive_relative.parent_path() == "C:");
   cat::verify(drive_relative.filename() == "foo");

   auto share = cat::raii::make_file_path<cat::path_os::nt>(
                   g_tests_alloc, "\\\\server\\share"
   )
                   .verify();
   cat::verify(share.is_absolute());
   cat::verify(share.root_name() == "\\\\server\\share");
   cat::verify(share.parent_path() == "\\\\server\\share");
   cat::verify(share.filename().size() == 0u);
   cat::verify(share.components().begin() == share.components().end());

   auto verbatim = cat::raii::make_file_path<cat::path_os::nt>(
                      g_tests_alloc, "\\\\?\\C:\\a/b\\c"
   )
                      .verify();
   cat::verify(verbatim.root_name() == "\\\\?\\C:");
   cat::verify(verbatim.parent_path() == "\\\\?\\C:\\a/b");
   cat::verify(verbatim.filename() == "c");

   // `\` is an ordinary character in `portable` paths.
   auto portable =
      cat::raii::make_file_path(g_tests_alloc, "C:\\alpha\\beta").verify();
   cat::verify(portable.is_relative());
   cat::verify(!portable.has_root_name());
   cat::verify(portable.filename() == "C:\\alpha\\beta");
   cat::verify(portable.parent_path().size() == 0u);
}

$test(path_nt_join) {
   auto path =
      cat::raii::make_file_path<cat::path_os::nt>(g_tests_alloc, "C:\\a")
         .verify();
   path.append("b").verify();
   cat::verify(path == "C:\\a\\b");
   path.append("c/").verify();
   path.append("d").verify();
   cat::verify(path == "C:\\a\\b\\c/d");
   // A root directory keeps the root name.
   path.append("\\rooted").verify();
   cat::verify(path == "C:\\rooted");
   // The same root name keeps the relative path.
   path.append("C:e").verify();
   cat::verify(path == "C:\\rooted\\e");
   // A different root name replaces the path.
   path.append("D:f").verify();
   cat::verify(path == "D:f");
   // Drive-relative paths do not gain a separator.
   auto drive =
      cat::raii::make_file_path<cat::path_os::nt>(g_tests_alloc, "C:").verify();
   drive.append("g").verify();
   cat::verify(drive == "C:g");
   // Shares have an implicit root directory, so they gain one.
   auto share = cat::raii::make_file_path<cat::path_os::nt>(
                   g_tests_alloc, "\\\\server\\share"
   )
                   .verify();
   auto joined = (share / "h").verify();
   cat::verify(joined == "\\\\server\\share\\h");
   joined.append("\\\\other\\place\\i").verify();
   cat::verify(joined == "\\\\other\\place\\i");

   // Factories deduce `nt` from an `nt` path.
   auto copy = cat::raii::make_file_path(g_tests_alloc, joined).verify();
   static_assert(cat::is_same<__typeof_unqual(copy), nt_path>);
   cat::verify(copy == "\\\\other\\place\\i");

   auto manual =
      cat::make_file_path<cat::path_os::nt>(g_tests_alloc, "x").verify();
   auto manual_joined =
      cat::make_file_path_joined(g_tests_alloc, manual, "y").verify();
   $defer {
      manual_joined.free(g_tests_alloc);
      manual.free(g_tests_alloc);
   };

   static_assert(
      cat::is_same<
         __typeof_unqual(manual_joined), cat::basic_file_path<cat::path_os::nt>>
   );
   cat::verify(manual_joined == "x\\y");
   cat::verify(cat::fmt(g_tests_alloc, "{}", manual_joined).verify() == "x\\y");
}

$test(path_nix) {
   auto path = cat::raii::make_file_path<cat::path_os::native>(
                  g_tests_alloc, "/alpha\\beta"
   )
                  .verify();
   static_assert(
      cat::is_same<
         __typeof_unqual(path),
         cat::raii::basic_file_path<cat::allocator_dyn, cat::path_os::nix>>
   );
   path.append("gamma").verify();
   cat::verify(path == "/alpha\\beta/gamma");
   cat::verify(path.parent_path() == "/alpha\\beta");
   cat::zstr_view native = path;
   cat::verify(native == "/alpha\\beta/gamma");

   auto absolute =
      cat::make_file_path_absolute(path, "/ignored", g_tests_alloc).verify();
   $defer {
      absolute.free(g_tests_alloc);
   };

   static_assert(
      cat::is_same<
         __typeof_unqual(absolute), cat::basic_file_path<cat::path_os::nix>>
   );
   cat::verify(absolute == "/alpha\\beta/gamma");
}
