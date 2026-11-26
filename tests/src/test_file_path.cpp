#include <cat/allocator_page>
#include <cat/file_path>
#include <cat/file_path_inplace>

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

   cat::array<cat::path_component, 4u> const expected = {
      cat::path_component{cat::path_component_kind::root_directory, "/"          },
      cat::path_component{cat::path_component_kind::normal,         "alpha"      },
      cat::path_component{cat::path_component_kind::normal,         "beta"       },
      cat::path_component{cat::path_component_kind::normal,         "file.tar.gz"},
   };
   cat::idx index = 0u;
   for (cat::path_component component : value.components()) {
      cat::verify(index < 4u);
      cat::verify(component == expected[index]);
      ++index;
   }

   cat::verify(index == 4u);

   cat::idx character_count = 0u;
   cat::iterate(value).run_while([&](char character) {
      cat::verify(character == value[character_count]);
      ++character_count;
      return true;
   });

   cat::verify(character_count == value.size());

   auto components = value.components();
   index = 0u;
   cat::iterate(components).run_while([&](cat::path_component component) {
      cat::verify(component == expected[index]);
      ++index;
      return true;
   });

   cat::verify(index == 4u);

   // Like Rust, only a leading `.` is a component.
   auto dots =
      cat::raii::make_file_path(g_tests_alloc, "./a/./../.profile").verify();
   cat::array<cat::path_component, 4u> const dot_expected = {
      cat::path_component{cat::path_component_kind::current_directory, "."       },
      cat::path_component{cat::path_component_kind::normal,            "a"       },
      cat::path_component{cat::path_component_kind::parent_directory,  ".."      },
      cat::path_component{cat::path_component_kind::normal,            ".profile"},
   };
   index = 0u;

   for (cat::path_component component : dots.components()) {
      cat::verify(index < 4u);
      cat::verify(component == dot_expected[index]);
      ++index;
   }

   cat::verify(index == 4u);

   // A trailing separator is not a component.
   auto trailing = cat::raii::make_file_path(g_tests_alloc, "a//").verify();
   auto trailing_components = trailing.components();
   auto trailing_component = trailing_components.begin();
   cat::verify(
      *trailing_component
      == cat::path_component{cat::path_component_kind::normal, "a"}
   );
   ++trailing_component;
   cat::verify(trailing_component == trailing_components.end());
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
   for (cat::path_component part : value.components()) {
      if (component == 0u) {
         cat::verify(part.kind == cat::path_component_kind::root_directory);
      } else if (component == 1u) {
         cat::verify(part.text.size() == 65u);
      } else {
         cat::verify(component == 2u);
         cat::verify(part.text.size() == 74u);
      }

      ++component;
   }

   cat::verify(component == 3u);

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
   auto root_components = root.components();
   auto root_component = root_components.begin();
   cat::verify(
      (*root_component).kind == cat::path_component_kind::root_directory
   );
   ++root_component;
   cat::verify(root_component == root_components.end());

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
static_assert(cat::detail::path_find_first_equal(u"ab\\c/", '/').value() == 4u);
static_assert(
   cat::detail::path_find_first_separator(
      u"ab\\c/", cat::detail::path_nt_separators
   )
      .value()
   == 2u
);
static_assert(
   cat::detail::path_find_last_not_separator(
      u"a\\/\\/", cat::detail::path_nt_separators
   )
      .value()
   == 0u
);

// Check the root of an `nt` path at compile time.
consteval auto
nt_root(
   cat::u16str_view value, cat::u16str_view name, cat::u16str_view directory,
   bool is_absolute
) -> bool {
   cat::detail::path_root const root =
      cat::detail::path_parse_root<cat::path_os::nt>(value);
   return cat::detail::path_slice(value, 0u, root.name_end) == name
          && cat::detail::path_slice(value, root.name_end, root.end)
                == directory
          && root.is_absolute == is_absolute;
}

static_assert(nt_root(u"", u"", u"", false));
static_assert(nt_root(u"foo\\bar", u"", u"", false));
static_assert(nt_root(u"\\foo", u"", u"\\", false));
static_assert(nt_root(u"/foo", u"", u"/", false));
static_assert(nt_root(u"C:", u"C:", u"", false));
static_assert(nt_root(u"c:foo", u"c:", u"", false));
static_assert(nt_root(u"C:\\foo", u"C:", u"\\", true));
static_assert(nt_root(u"C:/foo", u"C:", u"/", true));
static_assert(nt_root(u"1:\\foo", u"", u"", false));
static_assert(nt_root(u"\\\\server\\share", u"\\\\server\\share", u"", true));
static_assert(nt_root(u"//server/share/x", u"//server/share", u"/", true));
// Without a share name, `\\server` is only a root directory and a name.
static_assert(nt_root(u"\\\\server", u"", u"\\", false));
static_assert(nt_root(u"\\\\server\\", u"", u"\\", false));
static_assert(nt_root(u"\\\\.\\COM1", u"\\\\.\\COM1", u"", true));
static_assert(nt_root(u"//./COM1/x", u"//./COM1", u"/", true));
static_assert(nt_root(u"\\\\?\\C:", u"\\\\?\\C:", u"", true));
static_assert(nt_root(u"\\\\?\\C:\\x", u"\\\\?\\C:", u"\\", true));
// `/` is not a separator in verbatim paths.
static_assert(nt_root(u"\\\\?\\C:/x", u"\\\\?\\C:/x", u"", true));
static_assert(nt_root(
   u"\\\\?\\UNC\\server\\share\\x", u"\\\\?\\UNC\\server\\share", u"\\", true
));
static_assert(nt_root(u"\\\\?\\pictures\\x", u"\\\\?\\pictures", u"\\", true));
// A verbatim prefix spelled with `/` is a share named `?`.
static_assert(nt_root(u"//?/C:/x", u"//?/C:", u"/", true));

}  // namespace

$test(path_nt_decomposition) {
   auto drive = cat::raii::make_file_path<cat::path_os::nt>(
                   g_tests_alloc, u"C:\\alpha/beta\\\\file.tar.gz"
   )
                   .verify();
   static_assert(cat::is_same<__typeof_unqual(drive), nt_path>);
   cat::verify(drive.is_absolute());
   cat::verify(drive.root_name() == u"C:");
   cat::verify(drive.root_directory() == u"\\");
   cat::verify(drive.root_path() == u"C:\\");
   cat::verify(drive.relative_path() == u"alpha/beta\\\\file.tar.gz");
   cat::verify(drive.parent_path() == u"C:\\alpha/beta");
   cat::verify(drive.filename() == u"file.tar.gz");
   cat::verify(drive.stem() == u"file.tar");
   cat::verify(drive.extension() == u".gz");
   cat::verify(drive.has_root_name());
   cat::verify(drive.has_root_directory());

   using nt_component = cat::basic_path_component<char16_t>;
   cat::array<nt_component, 5u> const expected = {
      nt_component{cat::path_component_kind::root_name,      u"C:"         },
      nt_component{cat::path_component_kind::root_directory, u"/"          },
      nt_component{cat::path_component_kind::normal,         u"alpha"      },
      nt_component{cat::path_component_kind::normal,         u"beta"       },
      nt_component{cat::path_component_kind::normal,         u"file.tar.gz"},
   };
   cat::idx index = 0u;
   for (nt_component component : drive.components()) {
      cat::verify(index < 5u);
      cat::verify(component == expected[index]);
      ++index;
   }

   cat::verify(index == 5u);

   auto drive_relative =
      cat::raii::make_file_path<cat::path_os::nt>(g_tests_alloc, u"C:foo")
         .verify();
   cat::verify(drive_relative.is_relative());
   cat::verify(drive_relative.root_name() == u"C:");
   cat::verify(!drive_relative.has_root_directory());
   cat::verify(drive_relative.relative_path() == u"foo");
   cat::verify(drive_relative.parent_path() == u"C:");
   cat::verify(drive_relative.filename() == u"foo");

   auto share = cat::raii::make_file_path<cat::path_os::nt>(
                   g_tests_alloc, u"\\\\server\\share"
   )
                   .verify();
   cat::verify(share.is_absolute());
   cat::verify(share.root_name() == u"\\\\server\\share");
   cat::verify(share.parent_path() == u"\\\\server\\share");
   cat::verify(share.filename().size() == 0u);
   // A share's root directory is implicit.
   auto share_components = share.components();
   auto share_component = share_components.begin();
   cat::verify(
      *share_component
      == nt_component{cat::path_component_kind::root_name, u"\\\\server\\share"}
   );
   ++share_component;
   cat::verify(
      (*share_component).kind == cat::path_component_kind::root_directory
   );
   cat::verify((*share_component).text.size() == 0u);
   ++share_component;
   cat::verify(share_component == share_components.end());

   auto verbatim = cat::raii::make_file_path<cat::path_os::nt>(
                      g_tests_alloc, u"\\\\?\\C:\\a/b\\c"
   )
                      .verify();
   cat::verify(verbatim.root_name() == u"\\\\?\\C:");
   cat::verify(verbatim.parent_path() == u"\\\\?\\C:\\a/b");
   cat::verify(verbatim.filename() == u"c");

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
      cat::raii::make_file_path<cat::path_os::nt>(g_tests_alloc, u"C:\\a")
         .verify();
   path.append(u"b").verify();
   cat::verify(path == u"C:\\a\\b");
   path.append(u"c/").verify();
   path.append(u"d").verify();
   cat::verify(path == u"C:\\a\\b\\c/d");
   // A root directory keeps the root name.
   path.append(u"\\rooted").verify();
   cat::verify(path == u"C:\\rooted");
   // The same root name keeps the relative path.
   path.append(u"C:e").verify();
   cat::verify(path == u"C:\\rooted\\e");
   // A different root name replaces the path.
   path.append(u"D:f").verify();
   cat::verify(path == u"D:f");
   // Drive-relative paths do not gain a separator.
   auto drive =
      cat::raii::make_file_path<cat::path_os::nt>(g_tests_alloc, u"C:")
         .verify();
   drive.append(u"g").verify();
   cat::verify(drive == u"C:g");
   // Shares have an implicit root directory, so they gain one.
   auto share = cat::raii::make_file_path<cat::path_os::nt>(
                   g_tests_alloc, u"\\\\server\\share"
   )
                   .verify();
   auto joined = (share / u"h").verify();
   cat::verify(joined == u"\\\\server\\share\\h");
   joined.append(u"\\\\other\\place\\i").verify();
   cat::verify(joined == u"\\\\other\\place\\i");

   // Factories deduce `nt` from an `nt` path.
   auto copy = cat::raii::make_file_path(g_tests_alloc, joined).verify();
   static_assert(cat::is_same<__typeof_unqual(copy), nt_path>);
   cat::verify(copy == u"\\\\other\\place\\i");

   auto manual =
      cat::make_file_path<cat::path_os::nt>(g_tests_alloc, u"x").verify();
   auto manual_joined =
      cat::make_file_path_joined(g_tests_alloc, manual, u"y").verify();
   $defer {
      manual_joined.free(g_tests_alloc);
      manual.free(g_tests_alloc);
   };

   static_assert(
      cat::is_same<
         __typeof_unqual(manual_joined), cat::basic_file_path<cat::path_os::nt>>
   );
   cat::verify(manual_joined == u"x\\y");
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

$test(path_lexically_normal) {
   struct normal_case {
      cat::str_view input;
      cat::str_view expected;
   };

   cat::array const cases = {
      normal_case{"",                ""       },
      normal_case{".",               "."      },
      normal_case{"./",              "."      },
      normal_case{"a/..",            "."      },
      normal_case{"a/../",           "."      },
      normal_case{"foo/./bar/..",    "foo/"   },
      normal_case{"foo/.///bar/../", "foo/"   },
      normal_case{"a//b",            "a/b"    },
      normal_case{"a/./b/",          "a/b/"   },
      normal_case{"a/.",             "a/"     },
      normal_case{"../a/..",         ".."     },
      normal_case{"../../a",         "../../a"},
      normal_case{"a/../../b",       "../b"   },
      normal_case{"/..",             "/"      },
      normal_case{"//a/../b",        "/b"     },
      normal_case{"/a/",             "/a/"    },
   };

   for (normal_case const& test_case : cases) {
      auto manual =
         cat::make_file_path_lexically_normal(g_tests_alloc, test_case.input)
            .verify();
      $defer {
         manual.free(g_tests_alloc);
      };

      cat::verify(manual == test_case.expected);
      cat::verify(manual.data()[manual.size()] == '\0');
   }

   auto managed =
      cat::raii::make_file_path_lexically_normal(g_tests_alloc, "x/./y/../z")
         .verify();
   cat::verify(managed == "x/z");

   auto drive = cat::raii::make_file_path_lexically_normal<cat::path_os::nt>(
                   g_tests_alloc, u"C:/a/./b/..\\c"
   )
                   .verify();
   cat::verify(drive == u"C:\\a\\c");

   auto drive_relative =
      cat::raii::make_file_path_lexically_normal<cat::path_os::nt>(
         g_tests_alloc, u"C:a\\..\\.."
      )
         .verify();
   cat::verify(drive_relative == u"C:..");

   auto verbatim = cat::raii::make_file_path_lexically_normal<cat::path_os::nt>(
                      g_tests_alloc, u"\\\\?\\C:\\a\\..\\."
   )
                      .verify();
   cat::verify(verbatim == u"\\\\?\\C:\\a\\..\\.");
}

$test(path_canonical) {
   auto unique =
      cat::make_file_path_unique(g_tests_alloc, "/tmp/libcat-canonical-%%%%")
         .verify();
   $defer {
      unique.free(g_tests_alloc);
   };

   nix::file_permissions const permissions =
      nix::file_permissions::user_read
      | nix::file_permissions::user_write
      | nix::file_permissions::user_execute;
   nix::sys_mkdir(unique, permissions).verify();

   // `/tmp` may itself be a symbolic link.
   auto made_root =
      cat::make_file_path_canonical(unique, g_tests_alloc).verify();
   auto root = cat::raii::make_file_path(g_tests_alloc, made_root).verify();
   made_root.free(g_tests_alloc);
   auto directory = (root / "dir").verify();
   auto file = (directory / "file").verify();
   auto link_directory = (root / "link_dir").verify();
   auto up = (directory / "up").verify();
   auto absolute = (root / "absolute").verify();
   auto loop = (root / "loop").verify();
   $defer {
      auto _ = nix::sys_unlink(loop);
      _ = nix::sys_unlink(absolute);
      _ = nix::sys_unlink(up);
      _ = nix::sys_unlink(link_directory);
      _ = nix::sys_unlink(file);
      _ = nix::sys_rmdir(directory);
      _ = nix::sys_rmdir(root);
   };

   nix::sys_mkdir(directory, permissions).verify();
   nix::sys_close(
      nix::sys_openat(
         nix::at_fdcwd, file, nix::open_mode::write_only,
         nix::open_flags::create, permissions
      )
         .verify()
   )
      .verify();
   nix::sys_symlink("dir", link_directory).verify();
   nix::sys_symlink("..", up).verify();
   nix::sys_symlink(file, absolute).verify();
   nix::sys_symlink("loop", loop).verify();

   auto const verify_canonical =
      [&](cat::str_view input, cat::str_view expected) {
         auto joined = (root / input).verify();
         auto canonical =
            cat::make_file_path_canonical(joined, g_tests_alloc).verify();
         $defer {
            canonical.free(g_tests_alloc);
         };

         auto expected_path = (root / expected).verify();
         cat::verify(canonical == expected_path);
      };
   verify_canonical("link_dir/file", "dir/file");
   verify_canonical("dir/up/link_dir/./file", "dir/file");
   verify_canonical("absolute", "dir/file");
   verify_canonical("link_dir/", "dir");
   verify_canonical("dir/up/dir", "dir");
   verify_canonical(".//dir/../link_dir", "dir");

   auto const verify_error = [&](cat::str_view input, nix::linux_error error) {
      auto joined = (root / input).verify();
      auto canonical = cat::make_file_path_canonical(joined, g_tests_alloc);
      cat::verify(canonical.is_empty());
      cat::verify(canonical.error() == error);
   };
   verify_error("missing", nix::linux_error::noent);
   verify_error("dir/file/", nix::linux_error::notdir);
   verify_error("dir/file/..", nix::linux_error::notdir);
   verify_error("dir/file/x", nix::linux_error::notdir);
   verify_error("loop", nix::linux_error::loop);

   auto slash = cat::make_file_path_canonical("/", g_tests_alloc).verify();
   $defer {
      slash.free(g_tests_alloc);
   };

   cat::verify(slash == "/");
}

$test(path_compare) {
   struct compare_case {
      cat::str_view left;
      cat::str_view right;
      int order;
   };

   // These orders come from Rust's `std::path::Path::cmp`.
   cat::array const cases = {
      compare_case{"a//b",                      "a/b",                       0 },
      compare_case{"a/b",                       "a/b/",                      0 },
      compare_case{"a/b/",                      "a/b//",                     0 },
      compare_case{"a-",                        "a/",                        1 },
      compare_case{"a/-",                       "a//b",                      -1},
      compare_case{"ab/x",                      "a/x",                       1 },
      compare_case{"a",                         "/a",                        1 },
      compare_case{"/a",                        "//a",                       0 },
      compare_case{"",                          "",                          0 },
      compare_case{"",                          "a",                         -1},
      compare_case{"a/b",                       "a/c",                       -1},
      compare_case{"a/./b",                     "a/b",                       0 },
      compare_case{"\xff",                      "a",                         1 },
      compare_case{"a/b/",                      "a/b/c",                     -1},
      compare_case{"./a",                       "a",                         -1},
      compare_case{"..",                        "a",                         -1},
      // These share a spelling before their first difference.
      compare_case{"/usr/lib/x86_64/libfoo.so", "/usr/lib/x86_64/libbar.so", 1 },
      compare_case{"/a/b/./c",                  "/a/b/c",                    0 },
      compare_case{"a/bc",                      "a/b/c",                     1 },
      compare_case{"a/b",                       "a/b/.",                     0 },
      compare_case{"/a/b//c",                   "/a/b/c/",                   0 },
      compare_case{"./x/y",                     "./x/z",                     -1},
      compare_case{"/a/..",                     "/a/..",                     0 },
      compare_case{"/a/../b",                   "/a/./b",                    -1},
      compare_case{"x/yy/z",                    "x/y",                       1 },
   };

   for (compare_case const& test_case : cases) {
      auto left =
         cat::raii::make_file_path(g_tests_alloc, test_case.left).verify();
      auto right =
         cat::raii::make_file_path(g_tests_alloc, test_case.right).verify();
      std::strong_ordering const order = left <=> right;
      cat::verify((order < 0) == (test_case.order < 0));
      cat::verify((order > 0) == (test_case.order > 0));
      cat::verify((left == right) == (test_case.order == 0));
      // Strings compare as paths on either side.
      cat::verify((left == test_case.right) == (test_case.order == 0));
      cat::verify((test_case.right == left) == (test_case.order == 0));
   }

   auto manual = cat::make_file_path(g_tests_alloc, "x//y/").verify();
   $defer {
      manual.free(g_tests_alloc);
   };

   cat::file_path_view const view = manual;
   cat::verify(manual == "x/y/");
   cat::verify("x/y//" == manual);
   cat::verify(view == "x/y/");
   cat::verify(manual == "x/./y");
   cat::verify(manual != "x/y/z");
   cat::verify(manual < "x/z");
   cat::verify(cat::str_view("x/z") > manual);
   cat::verify(cat::str_view("x//y") == manual);

   // Like P3698's `inplace_vector`, paths of any storage and capacity compare.
   auto small = cat::make_file_path_inplace<8u>("x/y//").verify();
   auto large = cat::make_file_path_inplace<64u>("x//y/").verify();
   auto managed = cat::raii::make_file_path(g_tests_alloc, "x/z").verify();
   cat::verify(small == large);
   cat::verify(large == manual);
   cat::verify(view == small);
   cat::verify(small < managed);
   cat::verify(managed > view);

   auto drive =
      cat::raii::make_file_path<cat::path_os::nt>(g_tests_alloc, u"C:\\a/b")
         .verify();
   cat::verify(drive == u"C:/a\\\\b");
   cat::verify(drive != u"c:\\a\\b");
   // A root directory orders before a filename.
   cat::verify(drive < u"C:a\\b");
   cat::verify(drive < u"C:\\a\\c");
}
