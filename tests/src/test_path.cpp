#include <cat/page_allocator>
#include <cat/path>

#include "../unit_tests.hpp"

namespace {

static_assert(cat::detail::path_find_first_equal("abc/", '/').value() == 3u);
static_assert(
   cat::detail::path_find_first_not_equal("///a", '/').value() == 3u
);
static_assert(cat::detail::path_find_last_equal("a/b/c", '/').value() == 3u);
static_assert(
   cat::detail::path_find_last_not_equal("abc///", '/').value() == 2u
);
static_assert(cat::is_iterable<cat::path>);
static_assert(cat::is_stepanov_iterable<cat::path>);
static_assert(cat::is_iterable<cat::path_components>);
static_assert(cat::is_stepanov_iterable<cat::path_components>);
static_assert(
   cat::is_forward_stepanov_iterator<cat::path_components::iterator>
);

template <cat::is_path_segment Segment>
void
verify_path_string_type(Segment const& segment) {
   auto manual = cat::make_path(pager, segment).verify();
   $defer {
      manual.free(pager);
   };
   cat::verify(manual == "leaf");

   auto managed = cat::raii::make_path(pager, segment).verify();
   cat::verify(managed == "leaf");

   auto base = cat::raii::make_path(pager, "root").verify();
   auto joined = (base / segment).verify();
   cat::verify(joined == "root/leaf");
}

}  // namespace

$test(path_maybe_niche) {
   static_assert(sizeof(cat::maybe<cat::path>) == sizeof(cat::path));
   static_assert(
      sizeof(cat::maybe<cat::raii::path<>>) == sizeof(cat::raii::path<>)
   );

   cat::maybe<cat::path> manual_empty;
   cat::maybe<cat::raii::path<>> raii_empty;
   cat::verify(manual_empty.is_empty());
   cat::verify(raii_empty.is_empty());

   auto manual = cat::make_path(pager, "manual");
   $defer {
      manual.value().free(pager);
   };
   cat::verify(manual.has_value());
   cat::verify(manual.value() == "manual");

   auto managed = cat::raii::make_path(pager, "managed");
   cat::verify(managed.has_value());
   cat::verify(managed.value() == "managed");
}

$test(path_flags_and_factories) {
   static_assert(cat::path::flags.str.is_null_terminated);
   static_assert(cat::path::flags.vec.uses_pointer_size_layout);
   static_assert(cat::path::flags.vec.initial_growth_count == 48u);
   static_assert(cat::path_fixed<>::flags.vec.is_fixed_size);
   constexpr auto custom_growth = cat::vec_flags::initial_growth(64u);
   static_assert(
      cat::basic_path<custom_growth>::flags.vec.initial_growth_count == 64u
   );

   auto manual = cat::make_path(pager, "alpha").verify();
   cat::verify(manual == "alpha");
   cat::verify(manual.capacity() == 48u);
   cat::verify(manual.data()[manual.size()] == '\0');
   cat::zstr_view native = manual;
   cat::verify(native == "alpha");

   char text[] = "beta";
   char const* p_text = text;
   auto from_pointer = cat::make_path(pager, p_text).verify();
   cat::verify(from_pointer == "beta");

   auto managed = cat::raii::make_path(pager, cat::str_view("gamma")).verify();
   cat::verify(managed == "gamma");
   cat::verify(managed.data()[managed.size()] == '\0');
   cat::path released = managed.release();
   $defer {
      released.free(pager);
      from_pointer.free(pager);
      manual.free(pager);
   };
   cat::verify(released == "gamma");
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
   auto vector = cat::make_str_vec(pager, "leaf").verify();
   auto zvector = cat::make_zstr_vec(pager, "leaf").verify();
   $defer {
      zvector.free(pager);
      vector.free(pager);
   };
   auto raii_vector = cat::raii::make_str_vec(pager, "leaf").verify();
   auto raii_zvector = cat::raii::make_zstr_vec(pager, "leaf").verify();

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
   auto manual = cat::make_path(pager, "alpha").verify();
   manual.append(pager, "beta").verify();
   cat::verify(manual == "alpha/beta");
   manual.append(pager, "").verify();
   cat::verify(manual == "alpha/beta");
   manual.append(pager, "/rooted").verify();
   cat::verify(manual == "/rooted");

   auto joined = cat::make_path_joined(pager, manual, "leaf").verify();
   $defer {
      joined.free(pager);
      manual.free(pager);
   };
   cat::verify(joined == "/rooted/leaf");

   auto managed = cat::raii::make_path(pager, "one").verify();
   auto managed_joined = (managed / "two").verify();
   cat::verify(managed == "one");
   cat::verify(managed_joined == "one/two");
   managed_joined.append("/replacement").verify();
   cat::verify(managed_joined == "/replacement");
}

$test(path_decomposition_and_components) {
   auto value =
      cat::raii::make_path(pager, "/alpha//beta/file.tar.gz").verify();
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
   auto characters = cat::iterate(value);
   characters.run_while([&](char character) {
      cat::verify(character == value[character_count]);
      ++character_count;
      return true;
   });
   cat::verify(character_count == value.size());

   auto components = value.components();
   index = 0u;
   auto component_context = cat::iterate(components);
   component_context.run_while([&](cat::str_view component) {
      cat::verify(component == expected[index]);
      ++index;
      return true;
   });
   cat::verify(index == 3u);

   auto dots = cat::raii::make_path(pager, "a/./../.profile").verify();
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
      cat::raii::make_path(pager, cat::str_view(text, length)).verify();
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
      cat::raii::make_path(pager, cat::str_view(slashes, 65u)).verify();
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
      cat::raii::make_path(pager, cat::str_view(trailing, 106u)).verify();
   cat::verify(with_trailing.parent_path() == "/");
   cat::verify(with_trailing.filename().size() == 0u);
}

$test(path_formatter) {
   auto manual = cat::make_path(pager, "alpha/beta").verify();
   $defer {
      manual.free(pager);
   };
   cat::verify(cat::fmt(pager, "{}", manual).verify() == "alpha/beta");
   cat::verify(cat::fmt(pager, "{:g}", manual).verify() == "alpha/beta");
   cat::verify(cat::fmt(pager, "{:?}", manual).verify() == R"("alpha/beta")");
   cat::verify(cat::fmt(pager, "{:?g}", manual).verify() == R"("alpha/beta")");

   auto managed = cat::raii::make_path(pager, "a\nb").verify();
   cat::verify(cat::fmt(pager, "{}", managed).verify() == "a\nb");
   cat::verify(cat::fmt(pager, "{:?}", managed).verify() == R"("a\nb")");
}

$test(path_current_absolute_and_unique) {
   auto cwd = cat::make_path_current(pager).verify();
   cat::verify(cwd.is_absolute());
   cat::verify(!cwd.empty());
   cat::verify(cwd.data()[cwd.size()] == '\0');

   auto explicit_base =
      cat::make_path_absolute("child", "/tmp/base", pager).verify();
   cat::verify(explicit_base == "/tmp/base/child");

   auto relative_base =
      cat::make_path_absolute("child", "base", pager).verify();
   auto expected = cat::make_path_joined(pager, cwd, "base").verify();
   expected.append(pager, "child").verify();
   cat::verify(relative_base == expected);

   auto absolute_input =
      cat::make_path_absolute("/already/absolute", "/ignored", pager).verify();
   cat::verify(absolute_input == "/already/absolute");

   auto unchanged = cat::make_path_unique(pager, "fixed-name").verify();
   cat::verify(unchanged == "fixed-name");

   auto unique = cat::make_path_unique(pager, "tmp-%%%%-%%").verify();
   $defer {
      unique.free(pager);
      unchanged.free(pager);
      absolute_input.free(pager);
      expected.free(pager);
      relative_base.free(pager);
      explicit_base.free(pager);
      cwd.free(pager);
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
      cat::make_path_unique(pager, cat::str_view(model, 66u)).verify();
   $defer {
      boundary_unique.free(pager);
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
