// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>
#include <cat/str_literal>

namespace cat {

enum class character_set : unsigned char {
   utf8,
   portable,
};

template <__SIZE_TYPE__ utf8_size, __SIZE_TYPE__ portable_size>
struct symbol_text {
   // We keep two spellings of the symbol for both convenience and
   // pretty-printing.
   basic_str_literal<char8_t, utf8_size> m_utf8;
   basic_str_literal<char, portable_size> m_ascii;

   consteval symbol_text(char const (&text)[portable_size + 1])
       : m_utf8(text), m_ascii(text) {
   }

   consteval symbol_text(
      char8_t const (&utf8)[utf8_size + 1],
      char const (&portable)[portable_size + 1]
   )
       : m_utf8(utf8), m_ascii(portable) {
   }

   [[nodiscard]]
   constexpr auto
   utf8() const -> auto const& {
      return m_utf8;
   }

   [[nodiscard]]
   constexpr auto
   portable() const -> auto const& {
      return m_ascii;
   }

   template <__SIZE_TYPE__ right_utf8_size, __SIZE_TYPE__ right_portable_size>
   [[nodiscard]]
   friend consteval auto
   operator+(
      symbol_text left, symbol_text<right_utf8_size, right_portable_size> right
   ) {
      char8_t utf8[utf8_size + right_utf8_size + 1]{};
      char portable[portable_size + right_portable_size + 1]{};
      for (__SIZE_TYPE__ index = 0; index < utf8_size; ++index) {
         utf8[index] = left.m_utf8.data_[index];
      }
      for (__SIZE_TYPE__ index = 0; index <= right_utf8_size; ++index) {
         utf8[utf8_size + index] = right.m_utf8.data_[index];
      }
      for (__SIZE_TYPE__ index = 0; index < portable_size; ++index) {
         portable[index] = left.m_ascii.data_[index];
      }
      for (__SIZE_TYPE__ index = 0; index <= right_portable_size; ++index) {
         portable[portable_size + index] = right.m_ascii.data_[index];
      }
      return cat::symbol_text<
         utf8_size + right_utf8_size, portable_size + right_portable_size>{
         utf8, portable
      };
   }

   template <__SIZE_TYPE__ right_utf8_size, __SIZE_TYPE__ right_portable_size>
   [[nodiscard]]
   friend consteval auto
   operator==(
      symbol_text left, symbol_text<right_utf8_size, right_portable_size> right
   ) -> bool {
      if constexpr (
         utf8_size != right_utf8_size || portable_size != right_portable_size
      ) {
         return false;
      } else {
         for (__SIZE_TYPE__ index = 0; index < utf8_size; ++index) {
            if (left.m_utf8.data_[index] != right.m_utf8.data_[index]) {
               return false;
            }
         }
         for (__SIZE_TYPE__ index = 0; index < portable_size; ++index) {
            if (left.m_ascii.data_[index] != right.m_ascii.data_[index]) {
               return false;
            }
         }
         return true;
      }
   }
};

template <__SIZE_TYPE__ size>
symbol_text(char const (&)[size]) -> symbol_text<size - 1, size - 1>;

template <__SIZE_TYPE__ utf8_size, __SIZE_TYPE__ portable_size>
symbol_text(char8_t const (&)[utf8_size], char const (&)[portable_size])
   -> symbol_text<utf8_size - 1, portable_size - 1>;

template <symbol_text id>
struct base_dimension {
   static constexpr auto identifier = id;
};

namespace detail {

consteval auto
quantity_abs(int value) -> int {
   return value < 0 ? -value : value;
}

consteval auto
quantity_gcd(int left, int right) -> int {
   left = quantity_abs(left);
   right = quantity_abs(right);
   while (right != 0) {
      int const remainder = left % right;
      left = right;
      right = remainder;
   }
   return left == 0 ? 1 : left;
}

template <int numerator, int denominator>
struct normalized_power {
   static_assert(denominator != 0);
   static constexpr int divisor = quantity_gcd(numerator, denominator);
   static constexpr int sign = denominator < 0 ? -1 : 1;
   static constexpr int num = numerator / divisor * sign;
   static constexpr int den = quantity_abs(denominator) / divisor;
};

}  // namespace detail

template <typename Base, int numerator_value, int denominator_value = 1>
   requires(denominator_value != 0)
struct dimension_power {
   using base = Base;
   static constexpr int numerator =
      detail::normalized_power<numerator_value, denominator_value>::num;
   static constexpr int denominator =
      detail::normalized_power<numerator_value, denominator_value>::den;
};

template <typename Base, int numerator, int denominator = 1>
using normalized_dimension_power = dimension_power<
   Base, detail::normalized_power<numerator, denominator>::num,
   detail::normalized_power<numerator, denominator>::den>;

template <typename... Powers>
struct dimension {};

namespace detail {

template <bool value>
struct quantity_bool {
   static constexpr bool result = value;
};

template <bool condition, typename True, typename False>
struct quantity_select {
   using type = False;
};

template <typename True, typename False>
struct quantity_select<true, True, False> {
   using type = True;
};

template <bool condition, typename True, typename False>
using quantity_select_type = quantity_select<condition, True, False>::type;

template <typename T>
consteval auto
quantity_type_name() {
   return basic_str_literal{__PRETTY_FUNCTION__};
}

template <typename Left, typename Right>
consteval auto
quantity_type_less() -> bool {
   constexpr auto left = quantity_type_name<Left>();
   constexpr auto right = quantity_type_name<Right>();
   constexpr __SIZE_TYPE__ common_size =
      sizeof(left.data_) < sizeof(right.data_) ? sizeof(left.data_)
                                               : sizeof(right.data_);
   for (__SIZE_TYPE__ index = 0; index < common_size; ++index) {
      if (left.data_[index] < right.data_[index]) {
         return true;
      }
      if (left.data_[index] > right.data_[index]) {
         return false;
      }
   }
   return sizeof(left.data_) < sizeof(right.data_);
}

template <typename Left, typename Right>
inline constexpr bool dimension_base_less = quantity_type_less<Left, Right>();

template <typename Power, typename Dimension>
struct prepend_dimension;

template <typename Power, typename... Powers>
struct prepend_dimension<Power, dimension<Powers...>> {
   using type = dimension<Power, Powers...>;
};

template <typename Dimension, int numerator, int denominator = 1>
struct scale_dimension_powers;

template <int numerator, int denominator, typename... Powers>
struct scale_dimension_powers<dimension<Powers...>, numerator, denominator> {
   using type = dimension<normalized_dimension_power<
      typename Powers::base, Powers::numerator * numerator,
      Powers::denominator * denominator>...>;
};

template <typename Left, typename Right, int right_multiplier>
struct merge_dimensions;

template <int right_multiplier>
struct merge_dimensions<dimension<>, dimension<>, right_multiplier> {
   using type = dimension<>;
};

template <typename... LeftPowers, int right_multiplier>
struct merge_dimensions<
   dimension<LeftPowers...>, dimension<>, right_multiplier> {
   using type = dimension<LeftPowers...>;
};

template <typename... RightPowers, int right_multiplier>
struct merge_dimensions<
   dimension<>, dimension<RightPowers...>, right_multiplier>
    : scale_dimension_powers<dimension<RightPowers...>, right_multiplier> {};

template <
   typename LeftPower, typename... LeftPowers, typename RightPower,
   typename... RightPowers, int right_multiplier>
struct merge_dimensions<
   dimension<LeftPower, LeftPowers...>, dimension<RightPower, RightPowers...>,
   right_multiplier> {
 private:
   static constexpr bool same_base =
      __is_same(typename LeftPower::base, typename RightPower::base);
   static constexpr bool left_first =
      dimension_base_less<typename LeftPower::base, typename RightPower::base>;
   static constexpr bool right_first =
      dimension_base_less<typename RightPower::base, typename LeftPower::base>;
   static_assert(
      same_base || left_first || right_first,
      "Base dimensions require unique identifiers! Use a distinct "
      "`base_dimension` identifier."
   );

   using left_tail = dimension<LeftPowers...>;
   using right_tail = dimension<RightPowers...>;

   using left_result = prepend_dimension<
      LeftPower, typename merge_dimensions<
                    left_tail, dimension<RightPower, RightPowers...>,
                    right_multiplier>::type>::type;

   using right_result = prepend_dimension<
      normalized_dimension_power<
         typename RightPower::base, RightPower::numerator * right_multiplier,
         RightPower::denominator>,
      typename merge_dimensions<
         dimension<LeftPower, LeftPowers...>, right_tail,
         right_multiplier>::type>::type;

   static constexpr int combined_numerator =
      (LeftPower::numerator * RightPower::denominator)
      + (RightPower::numerator * LeftPower::denominator * right_multiplier);
   static constexpr int combined_denominator =
      LeftPower::denominator * RightPower::denominator;
   using combined_tail =
      merge_dimensions<left_tail, right_tail, right_multiplier>::type;
   using combined_result = prepend_dimension<
      normalized_dimension_power<
         typename LeftPower::base, combined_numerator, combined_denominator>,
      combined_tail>::type;

 public:
   using type = quantity_select_type<
      same_base,
      quantity_select_type<
         combined_numerator == 0, combined_tail, combined_result>,
      quantity_select_type<left_first, left_result, right_result>>;
};

template <typename Dimension>
struct is_dimension : quantity_bool<false> {};

template <typename... Powers>
struct is_dimension<dimension<Powers...>> : quantity_bool<true> {};

}  // namespace detail

template <typename Dimension>
concept is_dimension = detail::is_dimension<Dimension>::result;

template <is_dimension Left, is_dimension Right>
using multiplied_dimension = detail::merge_dimensions<Left, Right, 1>::type;

template <is_dimension Left, is_dimension Right>
using divided_dimension = detail::merge_dimensions<Left, Right, -1>::type;

template <is_dimension Dimension, int numerator, int denominator = 1>
using powered_dimension =
   detail::scale_dimension_powers<Dimension, numerator, denominator>::type;

template <typename Dimension>
concept is_base_dimension = requires { Dimension::identifier; };

namespace detail {

template <typename Dimension, bool = is_base_dimension<Dimension>>
struct normalized_dimension {
   using type = Dimension;
};

template <typename Dimension>
struct normalized_dimension<Dimension, true> {
   using type = dimension<dimension_power<Dimension, 1>>;
};

}  // namespace detail

template <typename T>
concept is_dimension_specifier = is_dimension<T> || is_base_dimension<T>;

template <is_dimension_specifier DimensionValue>
using normalized_dimension = detail::normalized_dimension<DimensionValue>::type;

template <is_dimension_specifier Left, is_dimension_specifier Right>
[[nodiscard]]
consteval auto
operator*([[maybe_unused]] Left left, [[maybe_unused]] Right right) {
   return multiplied_dimension<
      normalized_dimension<Left>, normalized_dimension<Right>>{};
}

template <is_dimension_specifier Left, is_dimension_specifier Right>
[[nodiscard]]
consteval auto
operator/([[maybe_unused]] Left left, [[maybe_unused]] Right right) {
   return divided_dimension<
      normalized_dimension<Left>, normalized_dimension<Right>>{};
}

template <
   int numerator, int denominator = 1, is_dimension_specifier DimensionValue>
   requires(denominator != 0)
[[nodiscard]]
consteval auto
pow([[maybe_unused]] DimensionValue dimension_value) {
   return powered_dimension<
      normalized_dimension<DimensionValue>, numerator, denominator>{};
}

template <is_dimension_specifier DimensionValue>
[[nodiscard]]
consteval auto
inverse(DimensionValue dimension_value) {
   return pow<-1>(dimension_value);
}

template <is_dimension_specifier DimensionValue>
[[nodiscard]]
consteval auto
sqrt(DimensionValue dimension_value) {
   return pow<1, 2>(dimension_value);
}

template <is_dimension_specifier DimensionValue>
[[nodiscard]]
consteval auto
cbrt(DimensionValue dimension_value) {
   return pow<1, 3>(dimension_value);
}

namespace detail {

template <
   typename Equation,
   bool has_dimension = requires { typename Equation::dimension_type; }>
struct quantity_equation_dimension {
   using type = normalized_dimension<Equation>::type;
};

template <typename Equation>
struct quantity_equation_dimension<Equation, true> {
   using type = Equation::dimension_type;
};

template <typename Equation>
concept has_dimension_type = requires { typename Equation::dimension_type; };

}  // namespace detail

enum class quantity_tensor_order : unsigned char {
   scalar,
   vector,
   tensor,
};

enum class quantity_field : unsigned char {
   real,
   complex,
};

struct quantity_character {
   quantity_tensor_order order = quantity_tensor_order::scalar;
   quantity_field field = quantity_field::real;
};

namespace detail {

template <typename T>
concept is_named_quantity_specification_type = [] {
   if constexpr (requires { T::is_named_quantity_specification; }) {
      return T::is_named_quantity_specification;
   } else {
      return false;
   }
}();

template <typename T>
concept is_quantity_spec_property = __is_same(T, quantity_character);

struct no_quantity_parent {};

template <bool has_parent, auto first, auto... arguments>
struct quantity_equation_argument {
   static constexpr auto value = first;
};

template <auto first, auto second, auto... arguments>
struct quantity_equation_argument<true, first, second, arguments...> {
   static constexpr auto value = [] {
      if constexpr (is_quantity_spec_property<__typeof_unqual(second)>) {
         return first;
      } else {
         return second;
      }
   }();
};

template <bool has_parent, auto first>
struct quantity_parent {
   using type = no_quantity_parent;
};

template <auto first>
struct quantity_parent<true, first> {
   using type = __typeof_unqual(first);
};

template <auto... arguments>
struct quantity_character_property {
   static constexpr quantity_character value{};
};

template <auto first, auto... arguments>
struct quantity_character_property<first, arguments...> {
   static constexpr quantity_character value = [] {
      if constexpr (__is_same(__typeof_unqual(first), quantity_character)) {
         return first;
      } else {
         return quantity_character_property<arguments...>::value;
      }
   }();
};

}  // namespace detail

template <auto... arguments>
struct quantity_spec;

template <auto first, auto... arguments>
struct quantity_spec<first, arguments...> {
 private:
   static constexpr bool has_parent =
      detail::is_named_quantity_specification_type<__typeof_unqual(first)>;
   static constexpr auto equation = detail::quantity_equation_argument<
      has_parent, first, arguments...>::value;

 public:
   static constexpr bool is_quantity_specification = true;
   static constexpr bool is_named_quantity_specification = true;
   static constexpr auto quantity_equation = equation;
   static constexpr quantity_character character =
      detail::quantity_character_property<arguments...>::value;
   using equation_type = __typeof_unqual(equation);
   using dimension_type =
      detail::quantity_equation_dimension<equation_type>::type;
   using parent_type = detail::quantity_parent<has_parent, first>::type;

   [[nodiscard]]
   consteval
   operator parent_type() const
      requires(has_parent)
   {
      return first;
   }

   template <typename Self, typename Unit>
   [[nodiscard]]
   consteval auto
   operator[](this Self self, Unit unit_value) {
      return make_reference(self, unit_value);
   }
};

template <is_dimension Dimension>
struct derived_quantity_spec {
   static constexpr bool is_quantity_specification = true;
   static constexpr bool is_named_quantity_specification = false;
   static constexpr quantity_character character{};
   using dimension_type = Dimension;
   using parent_type = detail::no_quantity_parent;
};

template <typename QuantitySpec>
concept is_quantity_spec =
   requires {
      QuantitySpec::is_quantity_specification;
      typename QuantitySpec::dimension_type;
      requires is_dimension<typename QuantitySpec::dimension_type>;
   };

namespace detail {

template <typename QuantitySpec>
using quantity_parent_type = QuantitySpec::parent_type;

template <typename Ancestor, typename Descendant>
struct is_quantity_ancestor {
   static constexpr bool value = [] {
      if constexpr (__is_same(Ancestor, Descendant)) {
         return true;
      } else if constexpr (
         __is_same(quantity_parent_type<Descendant>, no_quantity_parent)
      ) {
         return false;
      } else {
         return is_quantity_ancestor<
            Ancestor, quantity_parent_type<Descendant>>::value;
      }
   }();
};

template <
   typename Left, typename Right,
   bool left_is_ancestor = is_quantity_ancestor<Left, Right>::value,
   bool reached_root =
      __is_same(quantity_parent_type<Left>, no_quantity_parent)>
struct common_quantity_ancestor;

template <typename Left, typename Right, bool reached_root>
struct common_quantity_ancestor<Left, Right, true, reached_root> {
   using type = Left;
};

template <typename Left, typename Right>
struct common_quantity_ancestor<Left, Right, false, false>
    : common_quantity_ancestor<quantity_parent_type<Left>, Right> {};

template <typename Left, typename Right>
struct common_quantity_ancestor<Left, Right, false, true> {
   using type = no_quantity_parent;
};

template <typename Left, typename Right>
using common_quantity_ancestor_type =
   common_quantity_ancestor<Left, Right>::type;

template <typename Left, typename Right>
concept has_common_quantity_ancestor =
   !__is_same(common_quantity_ancestor_type<Left, Right>, no_quantity_parent);

template <typename QuantitySpec>
struct quantity_kind {
   using parent = quantity_parent_type<QuantitySpec>;
   using type = quantity_select_type<
      __is_same(parent, no_quantity_parent), QuantitySpec,
      typename quantity_kind<parent>::type>;
};

template <>
struct quantity_kind<no_quantity_parent> {
   using type = no_quantity_parent;
};

}  // namespace detail

template <auto quantity_specification>
   requires is_quantity_spec<__typeof_unqual(quantity_specification)>
inline constexpr detail::quantity_kind<
   __typeof_unqual(quantity_specification)>::type kind_of;

template <is_quantity_spec QuantitySpec>
[[nodiscard]]
consteval auto
get_dimension([[maybe_unused]] QuantitySpec quantity_specification) {
   return typename QuantitySpec::dimension_type{};
}

template <is_quantity_spec Left, is_quantity_spec Right>
[[nodiscard]]
consteval auto
operator*([[maybe_unused]] Left left, [[maybe_unused]] Right right) {
   using result_dimension = multiplied_dimension<
      typename Left::dimension_type, typename Right::dimension_type>;
   return derived_quantity_spec<result_dimension>{};
}

template <is_quantity_spec Left, is_quantity_spec Right>
[[nodiscard]]
consteval auto
operator/([[maybe_unused]] Left left, [[maybe_unused]] Right right) {
   using result_dimension = divided_dimension<
      typename Left::dimension_type, typename Right::dimension_type>;
   return derived_quantity_spec<result_dimension>{};
}

template <int numerator, int denominator = 1, is_quantity_spec QuantitySpec>
   requires(denominator != 0)
[[nodiscard]]
consteval auto
pow([[maybe_unused]] QuantitySpec quantity_specification) {
   using result_dimension = powered_dimension<
      typename QuantitySpec::dimension_type, numerator, denominator>;
   return derived_quantity_spec<result_dimension>{};
}

template <is_quantity_spec QuantitySpec>
[[nodiscard]]
consteval auto
inverse(QuantitySpec quantity_specification) {
   return pow<-1>(quantity_specification);
}

template <is_quantity_spec QuantitySpec>
[[nodiscard]]
consteval auto
sqrt(QuantitySpec quantity_specification) {
   return pow<1, 2>(quantity_specification);
}

template <is_quantity_spec QuantitySpec>
[[nodiscard]]
consteval auto
cbrt(QuantitySpec quantity_specification) {
   return pow<1, 3>(quantity_specification);
}

namespace detail {

template <typename Definition, auto... arguments>
struct named_unit_quantity_spec {
   using type = Definition;
};

template <typename Definition, auto first, auto... arguments>
struct named_unit_quantity_spec<Definition, first, arguments...> {
   using first_type = __typeof_unqual(first);
   using type = quantity_select_type<
      is_quantity_spec<first_type>, first_type,
      typename named_unit_quantity_spec<Definition, arguments...>::type>;
};

template <auto definition, bool = is_quantity_spec<__typeof_unqual(definition)>>
struct named_unit_definition {
   using type = __typeof_unqual(definition);
   using quantity_spec_type = type::quantity_spec_type;
   static constexpr bool defines_unit = true;
};

template <auto definition>
struct named_unit_definition<definition, true> {
   using type = __typeof_unqual(definition);
   using quantity_spec_type = type;
   static constexpr bool defines_unit = false;
};

}  // namespace detail

template <symbol_text symbol, auto definition, auto... arguments>
struct named_unit {
 private:
   using definition_data = detail::named_unit_definition<definition>;
   using inferred_quantity_spec = definition_data::quantity_spec_type;

 public:
   static constexpr bool is_unit_definition = true;
   using quantity_spec_type = detail::named_unit_quantity_spec<
      inferred_quantity_spec, arguments...>::type;
   using dimension_type = quantity_spec_type::dimension_type;
   static constexpr bool defines_unit = definition_data::defines_unit;
   static constexpr auto unit_definition = definition;
   static constexpr auto unit_symbol = symbol;
};

template <typename Unit, int numerator_value, int denominator_value = 1>
   requires(denominator_value != 0)
struct unit_power {
   using unit_type = Unit;
   static constexpr int numerator =
      detail::normalized_power<numerator_value, denominator_value>::num;
   static constexpr int denominator =
      detail::normalized_power<numerator_value, denominator_value>::den;
};

template <typename Unit, int numerator, int denominator = 1>
using normalized_unit_power = unit_power<
   Unit, detail::normalized_power<numerator, denominator>::num,
   detail::normalized_power<numerator, denominator>::den>;

template <typename... Powers>
struct unit {};

namespace detail {

template <typename Left, typename Right>
inline constexpr bool unit_base_less = quantity_type_less<Left, Right>();

template <typename Power, typename Unit>
struct prepend_unit;

template <typename Power, typename... Powers>
struct prepend_unit<Power, unit<Powers...>> {
   using type = unit<Power, Powers...>;
};

template <typename Unit, int numerator, int denominator = 1>
struct scale_unit_powers;

template <int numerator, int denominator, typename... Powers>
struct scale_unit_powers<unit<Powers...>, numerator, denominator> {
   using type = unit<normalized_unit_power<
      typename Powers::unit_type, Powers::numerator * numerator,
      Powers::denominator * denominator>...>;
};

template <typename Left, typename Right, int right_multiplier>
struct merge_units;

template <int right_multiplier>
struct merge_units<unit<>, unit<>, right_multiplier> {
   using type = unit<>;
};

template <typename... LeftPowers, int right_multiplier>
struct merge_units<unit<LeftPowers...>, unit<>, right_multiplier> {
   using type = unit<LeftPowers...>;
};

template <typename... RightPowers, int right_multiplier>
struct merge_units<unit<>, unit<RightPowers...>, right_multiplier>
    : scale_unit_powers<unit<RightPowers...>, right_multiplier> {};

template <
   typename LeftPower, typename... LeftPowers, typename RightPower,
   typename... RightPowers, int right_multiplier>
struct merge_units<
   unit<LeftPower, LeftPowers...>, unit<RightPower, RightPowers...>,
   right_multiplier> {
 private:
   static constexpr bool same_unit =
      __is_same(typename LeftPower::unit_type, typename RightPower::unit_type);
   static constexpr bool left_first = unit_base_less<
      typename LeftPower::unit_type, typename RightPower::unit_type>;
   static constexpr bool right_first = unit_base_less<
      typename RightPower::unit_type, typename LeftPower::unit_type>;
   static_assert(
      same_unit || left_first || right_first,
      "Base units require unique identifiers! Define each base unit with a "
      "distinct type."
   );

   using left_tail = unit<LeftPowers...>;
   using right_tail = unit<RightPowers...>;

   using left_result = prepend_unit<
      LeftPower, typename merge_units<
                    left_tail, unit<RightPower, RightPowers...>,
                    right_multiplier>::type>::type;

   using right_result = prepend_unit<
      normalized_unit_power<
         typename RightPower::unit_type,
         RightPower::numerator * right_multiplier, RightPower::denominator>,
      typename merge_units<
         unit<LeftPower, LeftPowers...>, right_tail,
         right_multiplier>::type>::type;

   static constexpr int combined_numerator =
      LeftPower::numerator * RightPower::denominator
      + RightPower::numerator * LeftPower::denominator * right_multiplier;
   static constexpr int combined_denominator =
      LeftPower::denominator * RightPower::denominator;
   using combined_tail =
      merge_units<left_tail, right_tail, right_multiplier>::type;
   using combined_result = prepend_unit<
      normalized_unit_power<
         typename LeftPower::unit_type, combined_numerator,
         combined_denominator>,
      combined_tail>::type;

 public:
   using type = quantity_select_type<
      same_unit,
      quantity_select_type<
         combined_numerator == 0, combined_tail, combined_result>,
      quantity_select_type<left_first, left_result, right_result>>;
};

template <typename Unit>
struct is_unit : quantity_bool<false> {};

template <typename... Powers>
struct is_unit<unit<Powers...>> : quantity_bool<true> {};

template <typename Unit>
struct unit_dimension;

template <>
struct unit_dimension<unit<>> {
   using type = dimension<>;
};

template <typename Power, typename... Powers>
struct unit_dimension<unit<Power, Powers...>> {
   using current = Power::unit_type::dimension_type;
   using powered = scale_dimension_powers<
      current, Power::numerator, Power::denominator>::type;
   using type = multiplied_dimension<
      powered, typename unit_dimension<unit<Powers...>>::type>;
};

}  // namespace detail

template <typename Unit>
concept is_unit = detail::is_unit<Unit>::result;

template <is_unit Left, is_unit Right>
using multiplied_unit = detail::merge_units<Left, Right, 1>::type;

template <is_unit Left, is_unit Right>
using divided_unit = detail::merge_units<Left, Right, -1>::type;

template <is_unit Unit, int numerator, int denominator = 1>
using powered_unit =
   detail::scale_unit_powers<Unit, numerator, denominator>::type;

template <is_unit Unit>
using unit_dimension = detail::unit_dimension<Unit>::type;

template <
   unsigned long long numerator_value = 1,
   unsigned long long denominator_value = 1, int decimal_power_value = 0,
   int pi_power_value = 0>
   requires(numerator_value != 0 && denominator_value != 0)
struct magnitude {
   static constexpr unsigned long long numerator = numerator_value;
   static constexpr unsigned long long denominator = denominator_value;
   static constexpr int decimal_power = decimal_power_value;
   static constexpr int pi_power = pi_power_value;
};

namespace detail {

// NOLINTNEXTLINE
inline constexpr long double pi_value = 0x1.921fb54442d18469898cc51701b8p+1L;

consteval auto
quantity_gcd(unsigned long long left, unsigned long long right)
   -> unsigned long long {
   while (right != 0) {
      auto const remainder = left % right;
      left = right;
      right = remainder;
   }
   return left;
}

}  // namespace detail

template <
   unsigned long long numerator, unsigned long long denominator = 1,
   int decimal_power = 0, int pi_power = 0>
using normalized_magnitude = magnitude<
   numerator / detail::quantity_gcd(numerator, denominator),
   denominator / detail::quantity_gcd(numerator, denominator), decimal_power,
   pi_power>;

inline constexpr magnitude<> magnitude_one;
inline constexpr magnitude<1, 1, 0, 1> mag_pi;

template <typename T>
concept is_unit_magnitude = requires {
                               T::numerator;
                               T::denominator;
                               T::decimal_power;
                               T::pi_power;
                            };

template <
   unsigned long long left_numerator, unsigned long long left_denominator,
   int left_power, int left_pi_power, unsigned long long right_numerator,
   unsigned long long right_denominator, int right_power, int right_pi_power>
[[nodiscard]]
consteval auto
operator*(
   [[maybe_unused]] magnitude<
      left_numerator, left_denominator, left_power, left_pi_power>
      left,
   [[maybe_unused]] magnitude<
      right_numerator, right_denominator, right_power, right_pi_power>
      right
) {
   return normalized_magnitude<
      left_numerator * right_numerator, left_denominator * right_denominator,
      left_power + right_power, left_pi_power + right_pi_power>{};
}

template <
   unsigned long long left_numerator, unsigned long long left_denominator,
   int left_power, int left_pi_power, unsigned long long right_numerator,
   unsigned long long right_denominator, int right_power, int right_pi_power>
[[nodiscard]]
consteval auto
operator/(
   [[maybe_unused]] magnitude<
      left_numerator, left_denominator, left_power, left_pi_power>
      left,
   [[maybe_unused]] magnitude<
      right_numerator, right_denominator, right_power, right_pi_power>
      right
) {
   return normalized_magnitude<
      left_numerator * right_denominator, left_denominator * right_numerator,
      left_power - right_power, left_pi_power - right_pi_power>{};
}

template <int decimal_power>
inline constexpr magnitude<1, 1, decimal_power> power_of_10;

template <unsigned long long numerator, unsigned long long denominator = 1>
inline constexpr normalized_magnitude<numerator, denominator> mag_ratio;

template <unsigned long long value>
inline constexpr magnitude<value> mag;

namespace detail {

consteval auto
magnitude_integer_power(unsigned long long base, unsigned long long exponent)
   -> unsigned long long {
   unsigned long long result = 1;
   while (exponent != 0) {
      if ((exponent & 1) != 0) {
         result *= base;
      }
      base *= base;
      exponent >>= 1;
   }
   return result;
}

template <auto value, int numerator, int denominator>
consteval auto
powered_magnitude() {
   using magnitude_type = __typeof_unqual(value);
   if constexpr (denominator == 1) {
      constexpr auto absolute_power = numerator < 0 ? -numerator : numerator;
      if constexpr (numerator < 0) {
         return normalized_magnitude<
            magnitude_integer_power(
               magnitude_type::denominator, absolute_power
            ),
            magnitude_integer_power(magnitude_type::numerator, absolute_power),
            magnitude_type::decimal_power * numerator,
            magnitude_type::pi_power * numerator>{};
      } else {
         return normalized_magnitude<
            magnitude_integer_power(magnitude_type::numerator, absolute_power),
            magnitude_integer_power(
               magnitude_type::denominator, absolute_power
            ),
            magnitude_type::decimal_power * numerator,
            magnitude_type::pi_power * numerator>{};
      }
   } else {
      return magnitude<
         1, 1, magnitude_type::decimal_power * numerator / denominator,
         magnitude_type::pi_power * numerator / denominator>{};
   }
}

template <auto value>
consteval auto
magnitude_value() -> long double {
   using magnitude_type = __typeof_unqual(value);
   long double result = static_cast<long double>(magnitude_type::numerator)
                        / magnitude_type::denominator;
   if constexpr (magnitude_type::decimal_power > 0) {
      for (int index = 0; index < magnitude_type::decimal_power; ++index) {
         result *= 10;
      }
   } else {
      for (int index = 0; index > magnitude_type::decimal_power; --index) {
         result /= 10;
      }
   }
   if constexpr (magnitude_type::pi_power > 0) {
      for (int index = 0; index < magnitude_type::pi_power; ++index) {
         result *= pi_value;
      }
   } else {
      for (int index = 0; index > magnitude_type::pi_power; --index) {
         result /= pi_value;
      }
   }
   return result;
}

}  // namespace detail

template <unsigned long long base, int numerator, int denominator = 1>
   requires(base > 0 && denominator == 1)
inline constexpr auto mag_power = [] {
   if constexpr (base == 10) {
      return power_of_10<numerator>;
   } else {
      constexpr auto absolute_power = detail::magnitude_integer_power(
         base, numerator < 0 ? -numerator : numerator
      );
      if constexpr (numerator < 0) {
         return magnitude<1, absolute_power>{};
      } else {
         return magnitude<absolute_power>{};
      }
   }
}();

template <is_unit_magnitude Left, is_unit_magnitude Right>
[[nodiscard]]
consteval auto
operator==([[maybe_unused]] Left left, [[maybe_unused]] Right right) -> bool {
   return detail::magnitude_value<Left{}>()
          == detail::magnitude_value<Right{}>();
}

template <is_unit_magnitude Left, is_unit_magnitude Right>
[[nodiscard]]
consteval auto
operator<=>([[maybe_unused]] Left left, [[maybe_unused]] Right right) {
   return detail::magnitude_value<Left{}>()
          <=> detail::magnitude_value<Right{}>();
}

template <int numerator, int denominator = 1, is_unit_magnitude Magnitude>
   requires(
      denominator != 0
      && (denominator == 1 || (Magnitude::numerator == 1 && Magnitude::denominator == 1 && (Magnitude::decimal_power * numerator) % denominator == 0 && (Magnitude::pi_power * numerator) % denominator == 0))
   )
[[nodiscard]]
consteval auto
pow([[maybe_unused]] Magnitude magnitude_value) {
   return detail::powered_magnitude<Magnitude{}, numerator, denominator>();
}

template <
   is_quantity_spec QuantitySpec, is_unit Unit,
   auto unit_magnitude_value = magnitude_one>
   requires(
      requires {
         unit_magnitude_value.numerator;
         unit_magnitude_value.denominator;
         unit_magnitude_value.decimal_power;
         unit_magnitude_value.pi_power;
      }
      && __is_same(typename QuantitySpec::dimension_type, unit_dimension<Unit>)
   )
// TODO: "reference" is meant here in the sense of ISO IEC quantity
// references. That is, a "measurement unit, a measurement procedure, a
// reference material, or a combination of such." This, combined with a
// value, is a "quantity".
// https://www.iso.org/obp/ui#iso:std:iso-iec:guide:99
//
// The C++ stdlib will likely change this, so we will eventually as well.
struct reference {
   using quantity_spec_type = QuantitySpec;
   using unit_type = Unit;
   using dimension_type = QuantitySpec::dimension_type;
   static constexpr auto unit_magnitude = unit_magnitude_value;
   static constexpr int scale = unit_magnitude_value.decimal_power;
};

template <
   is_quantity_spec QuantitySpec, is_unit UnitExpression,
   auto unit_magnitude_value = magnitude_one>
struct derived_unit {
   static constexpr bool is_unit_definition = true;
   using quantity_spec_type = QuantitySpec;
   using dimension_type = QuantitySpec::dimension_type;
   using reference_type =
      reference<QuantitySpec, UnitExpression, unit_magnitude_value>;
};

template <typename Unit>
concept is_named_unit = requires {
                           typename Unit::quantity_spec_type;
                           typename Unit::dimension_type;
                           Unit::unit_symbol;
                        };

template <typename T>
concept is_unit_specifier =
   is_named_unit<T> || requires {
                          T::is_unit_definition;
                          typename T::reference_type;
                       };

namespace detail {

template <
   typename QuantitySpecifier,
   bool has_quantity =
      requires { typename QuantitySpecifier::reference_type; },
   bool = is_named_unit<QuantitySpecifier>>
struct reference_quantity {
   using type = QuantitySpecifier;
};

template <typename Unit, bool = Unit::defines_unit>
struct named_reference_quantity {
   using type = reference<
      typename Unit::quantity_spec_type, unit<unit_power<Unit, 1>>>;
};

template <typename Unit>
struct named_reference_quantity<Unit, true> {
 private:
   using definition_quantity =
      reference_quantity<__typeof_unqual(Unit::unit_definition)>::type;

 public:
   using type = reference<
      typename Unit::quantity_spec_type,
      typename definition_quantity::unit_type,
      definition_quantity::unit_magnitude>;
};

template <typename Unit>
struct reference_quantity<Unit, false, true> : named_reference_quantity<Unit> {
};

template <typename QuantitySpecifier, bool is_named>
struct reference_quantity<QuantitySpecifier, true, is_named> {
   using type = QuantitySpecifier::reference_type;
};

}  // namespace detail

template <auto quantity>
using reference_quantity =
   detail::reference_quantity<__typeof_unqual(quantity)>::type;

namespace detail {

template <typename>
inline constexpr bool is_generic_quantity_spec = false;

template <typename Dimension>
inline constexpr bool
   is_generic_quantity_spec<derived_quantity_spec<Dimension>> = true;

}  // namespace detail

template <typename Quantity>
concept is_quantity = requires {
                         typename Quantity::quantity_spec_type;
                         typename Quantity::unit_type;
                         typename Quantity::dimension_type;
                         Quantity::scale;
                      };

template <typename T>
concept is_quantity_specifier = is_quantity<T> || is_unit_specifier<T>;

template <symbol_text symbol, auto magnitude, auto unit_value>
   requires is_named_unit<__typeof_unqual(unit_value)>
struct prefixed_unit {
 private:
   using base_quantity = reference_quantity<unit_value>;

 public:
   static constexpr bool is_unit_definition = true;
   using quantity_spec_type = base_quantity::quantity_spec_type;
   using dimension_type = base_quantity::dimension_type;
   using reference_type = reference<
      quantity_spec_type, typename base_quantity::unit_type,
      base_quantity::unit_magnitude * magnitude>;
   static constexpr auto unit_symbol = symbol + unit_value.unit_symbol;
};

template <is_quantity Quantity>
[[nodiscard]]
consteval auto
get_quantity_spec([[maybe_unused]] Quantity quantity) {
   return typename Quantity::quantity_spec_type{};
}

template <is_unit_specifier Unit>
[[nodiscard]]
consteval auto
get_quantity_spec([[maybe_unused]] Unit unit) {
   return typename Unit::quantity_spec_type{};
}

template <is_quantity Quantity>
[[nodiscard]]
consteval auto
get_unit([[maybe_unused]] Quantity quantity) {
   return typename Quantity::unit_type{};
}

template <is_unit_specifier Unit>
[[nodiscard]]
consteval auto
get_unit(Unit unit) {
   return unit;
}

template <is_unit_specifier Left, is_unit_specifier Right>
[[nodiscard]]
consteval auto
operator==([[maybe_unused]] Left left, [[maybe_unused]] Right right) -> bool {
   return __is_same(Left, Right);
}

// Test whether two units share a canonical reference unit and magnitude, so
// that conversions between them preserve the numerical value.
template <is_unit_specifier Left, is_unit_specifier Right>
[[nodiscard]]
consteval auto
is_equivalent([[maybe_unused]] Left left, [[maybe_unused]] Right right)
   -> bool {
   using left_quantity = reference_quantity<Left{}>;
   using right_quantity = reference_quantity<Right{}>;
   return __is_same(
             typename left_quantity::unit_type,
             typename right_quantity::unit_type
          )
          && __is_same(
             __typeof_unqual(left_quantity::unit_magnitude),
             __typeof_unqual(right_quantity::unit_magnitude)
          );
}

template <is_quantity_spec QuantitySpec, is_unit_specifier UnitType>
[[nodiscard]]
consteval auto
make_reference(
   [[maybe_unused]] QuantitySpec quantity_specification, UnitType unit_value
) {
   if constexpr (
      __is_same(QuantitySpec, __typeof_unqual(get_quantity_spec(unit_value)))
   ) {
      return unit_value;
   } else {
      using unit_quantity = reference_quantity<UnitType{}>;
      return reference<
         QuantitySpec, typename unit_quantity::unit_type,
         unit_quantity::unit_magnitude>{};
   }
}

template <is_quantity_specifier R>
[[nodiscard]]
consteval auto
get_dimension(R reference) {
   using quantity_specification = __typeof_unqual(get_quantity_spec(reference));
   return typename quantity_specification::dimension_type{};
}

template <typename Quantity>
concept is_dimensionless_quantity =
   is_quantity<Quantity>
   && __is_same(typename Quantity::dimension_type, dimension<>);

struct scalar_quantity_spec : quantity_spec<dimension<>{}> {};

using scalar_quantity = reference<scalar_quantity_spec, unit<>>;

template <typename Left, typename Right>
concept is_same_quantity_dimension =
   is_quantity<Left> && is_quantity<Right>
   && __is_same(typename Left::dimension_type, typename Right::dimension_type);

template <typename Left, typename Right>
concept is_compatible_quantity =
   is_same_quantity_dimension<Left, Right>
   && __is_same(typename Left::unit_type, typename Right::unit_type) && (__is_same(typename Left::quantity_spec_type, typename Right::quantity_spec_type) || detail::is_generic_quantity_spec<typename Left::quantity_spec_type> || detail::is_generic_quantity_spec<typename Right::quantity_spec_type> || detail::has_common_quantity_ancestor<typename Left::quantity_spec_type, typename Right::quantity_spec_type>);

namespace detail {

template <typename Left, typename Right>
struct common_quantity_spec {
 private:
   using left_spec = Left::quantity_spec_type;
   using right_spec = Right::quantity_spec_type;
   using common_ancestor = common_quantity_ancestor_type<left_spec, right_spec>;

 public:
   using type = quantity_select_type<
      __is_same(left_spec, right_spec), left_spec,
      quantity_select_type<
         is_generic_quantity_spec<left_spec>, right_spec,
         quantity_select_type<
            is_generic_quantity_spec<right_spec>, left_spec, common_ancestor>>>;
};

template <typename Left, typename Right, typename ResultDimension>
struct multiplied_quantity_spec {
   using type = quantity_select_type<
      __is_same(ResultDimension, dimension<>), scalar_quantity_spec,
      quantity_select_type<
         is_dimensionless_quantity<Left>, typename Right::quantity_spec_type,
         quantity_select_type<
            is_dimensionless_quantity<Right>, typename Left::quantity_spec_type,
            derived_quantity_spec<ResultDimension>>>>;
};

template <typename Left, typename Right, typename ResultDimension>
struct divided_quantity_spec {
   using type = quantity_select_type<
      __is_same(ResultDimension, dimension<>), scalar_quantity_spec,
      quantity_select_type<
         is_dimensionless_quantity<Right>, typename Left::quantity_spec_type,
         derived_quantity_spec<ResultDimension>>>;
};

}  // namespace detail

namespace detail {

template <typename Left, typename Right>
consteval auto
common_unit_magnitude() {
   if constexpr (
      magnitude_value<Left::unit_magnitude>()
      <= magnitude_value<Right::unit_magnitude>()
   ) {
      return Left::unit_magnitude;
   } else {
      return Right::unit_magnitude;
   }
}

}  // namespace detail

template <is_quantity Left, is_quantity Right>
   requires(is_compatible_quantity<Left, Right>)
using common_quantity = reference<
   typename detail::common_quantity_spec<Left, Right>::type,
   typename Left::unit_type, detail::common_unit_magnitude<Left, Right>()>;

template <is_quantity Left, is_quantity Right>
using multiplied_quantity = reference<
   typename detail::multiplied_quantity_spec<
      Left, Right,
      multiplied_dimension<
         typename Left::dimension_type, typename Right::dimension_type>>::type,
   multiplied_unit<typename Left::unit_type, typename Right::unit_type>,
   Left::unit_magnitude * Right::unit_magnitude>;

template <is_quantity Left, is_quantity Right>
using divided_quantity = reference<
   typename detail::divided_quantity_spec<
      Left, Right,
      divided_dimension<
         typename Left::dimension_type, typename Right::dimension_type>>::type,
   divided_unit<typename Left::unit_type, typename Right::unit_type>,
   Left::unit_magnitude / Right::unit_magnitude>;

template <is_quantity Quantity, int decimal_power>
using scaled_quantity = reference<
   typename Quantity::quantity_spec_type, typename Quantity::unit_type,
   Quantity::unit_magnitude * power_of_10<decimal_power>>;

template <is_quantity Quantity, int numerator, int denominator = 1>
   requires(
      denominator != 0
      && (denominator == 1
          || (Quantity::unit_magnitude.numerator == 1
              && Quantity::unit_magnitude.denominator == 1
              && (Quantity::scale * numerator) % denominator == 0
              && (Quantity::unit_magnitude.pi_power * numerator) % denominator
                    == 0))
   )
using powered_quantity = reference<
   derived_quantity_spec<powered_dimension<
      typename Quantity::dimension_type, numerator, denominator>>,
   powered_unit<typename Quantity::unit_type, numerator, denominator>,
   detail::powered_magnitude<
      Quantity::unit_magnitude, numerator, denominator>()>;

template <is_unit_magnitude Magnitude, is_quantity_specifier R>
[[nodiscard]]
consteval auto
operator*(Magnitude magnitude_value, [[maybe_unused]] R reference_value) {
   using source_quantity = reference_quantity<R{}>;
   using result_quantity = reference<
      typename source_quantity::quantity_spec_type,
      typename source_quantity::unit_type,
      magnitude_value * source_quantity::unit_magnitude>;
   if constexpr (is_unit_specifier<R>) {
      return derived_unit<
         typename result_quantity::quantity_spec_type,
         typename result_quantity::unit_type,
         result_quantity::unit_magnitude>{};
   } else {
      return result_quantity{};
   }
}

template <is_quantity_specifier R, is_unit_magnitude Magnitude>
[[nodiscard]]
consteval auto
operator*(R reference_value, Magnitude magnitude_value) {
   return magnitude_value * reference_value;
}

template <is_quantity_specifier Left, is_quantity_specifier Right>
[[nodiscard]]
consteval auto
operator*([[maybe_unused]] Left left, [[maybe_unused]] Right right) {
   using left_quantity = reference_quantity<Left{}>;
   using right_quantity = reference_quantity<Right{}>;
   using result_quantity = multiplied_quantity<left_quantity, right_quantity>;
   if constexpr (is_unit_specifier<Left> && is_unit_specifier<Right>) {
      return derived_unit<
         typename result_quantity::quantity_spec_type,
         typename result_quantity::unit_type,
         result_quantity::unit_magnitude>{};
   } else {
      return result_quantity{};
   }
}

template <is_quantity_specifier Left, is_quantity_specifier Right>
[[nodiscard]]
consteval auto
operator/([[maybe_unused]] Left left, [[maybe_unused]] Right right) {
   using left_quantity = reference_quantity<Left{}>;
   using right_quantity = reference_quantity<Right{}>;
   using result_quantity = divided_quantity<left_quantity, right_quantity>;
   if constexpr (is_unit_specifier<Left> && is_unit_specifier<Right>) {
      return derived_unit<
         typename result_quantity::quantity_spec_type,
         typename result_quantity::unit_type,
         result_quantity::unit_magnitude>{};
   } else {
      return result_quantity{};
   }
}

template <int numerator, int denominator = 1, is_quantity_specifier R>
   requires(
      denominator != 0
      && (denominator == 1
          || (reference_quantity<R{}>::unit_magnitude.numerator == 1
              && reference_quantity<R{}>::unit_magnitude.denominator == 1
              && (reference_quantity<R{}>::scale * numerator) % denominator
                    == 0
              && (reference_quantity<R{}>::unit_magnitude.pi_power * numerator)
                       % denominator
                    == 0))
   )
[[nodiscard]] consteval auto pow([[maybe_unused]] R reference_value) {
   using result_quantity =
      powered_quantity<reference_quantity<R{}>, numerator, denominator>;
   if constexpr (is_unit_specifier<R>) {
      return derived_unit<
         typename result_quantity::quantity_spec_type,
         typename result_quantity::unit_type,
         result_quantity::unit_magnitude>{};
   } else {
      return result_quantity{};
   }
}

template <is_quantity_specifier R>
[[nodiscard]]
consteval auto
inverse(R reference_value) {
   return pow<-1>(reference_value);
}

template <is_quantity_specifier R>
[[nodiscard]]
consteval auto
sqrt(R reference_value) {
   return pow<1, 2>(reference_value);
}

template <is_quantity_specifier R>
[[nodiscard]]
consteval auto
cbrt(R reference_value) {
   return pow<1, 3>(reference_value);
}

inline constexpr scalar_quantity scalar;

namespace detail {

template <typename T>
struct arithmetic_quantity_trait {
   using type = __typeof_unqual(scalar);
};

template <typename T, overflow_policies policy, auto quantity>
struct arithmetic_quantity_trait<basic_int<T, policy, quantity>> {
   using type = cat::reference_quantity<quantity>;
};

template <typename T, precision_policies precision, auto quantity>
struct arithmetic_quantity_trait<basic_float<T, precision, quantity>> {
   using type = cat::reference_quantity<quantity>;
};

template <typename T, auto new_quantity>
struct rebind_quantity {
   using type = T;
};

template <
   typename T, overflow_policies policy, auto old_quantity, auto new_quantity>
struct rebind_quantity<basic_int<T, policy, old_quantity>, new_quantity> {
   using type = basic_int<T, policy, new_quantity>;
};

template <
   typename T, precision_policies precision, auto old_quantity,
   auto new_quantity>
struct rebind_quantity<basic_float<T, precision, old_quantity>, new_quantity> {
   using type = basic_float<T, precision, new_quantity>;
};

}  // namespace detail

template <typename T>
using arithmetic_quantity =
   detail::arithmetic_quantity_trait<remove_cvref<T>>::type;

template <typename T>
concept has_quantity = !is_same<arithmetic_quantity<T>, scalar_quantity>;

namespace detail {

template <typename>
inline constexpr bool is_radian_or_degree_reference = false;

// TODO: Promote to `cat::` namespace?
template <typename T>
concept is_angle_quantity =
   has_quantity<T> && is_radian_or_degree_reference<arithmetic_quantity<T>>;

}  // namespace detail

template <typename T>
concept is_dimensionless_arithmetic =
   is_dimensionless_quantity<arithmetic_quantity<T>>;

namespace detail {

template <typename From, typename To>
concept is_quantity_conversion =
   is_dimensionless_quantity<arithmetic_quantity<From>>
   || is_dimensionless_quantity<To>
   || is_compatible_quantity<arithmetic_quantity<From>, To>;

template <typename From, typename To>
concept is_quantity_direct_conversion =
   is_quantity_conversion<From, To>
   && (!has_quantity<From> || __is_same(__typeof_unqual(arithmetic_quantity<From>::unit_magnitude), __typeof_unqual(To::unit_magnitude)));

template <is_quantity From, is_quantity To>
[[nodiscard]]
consteval auto
is_integral_quantity_scale() -> bool {
   constexpr auto conversion = From::unit_magnitude / To::unit_magnitude;
   using conversion_type = __typeof_unqual(conversion);
   if constexpr (conversion_type::pi_power != 0) {
      return false;
   } else if constexpr (conversion_type::decimal_power >= 0) {
      auto denominator = conversion_type::denominator;
      for (int i = 0; i < conversion_type::decimal_power; ++i) {
         denominator /= quantity_gcd(denominator, 10ull);
      }
      return denominator == 1;
   } else {
      auto numerator = conversion_type::numerator;
      if (numerator % conversion_type::denominator != 0) {
         return false;
      }
      numerator /= conversion_type::denominator;
      for (int i = 0; i > conversion_type::decimal_power; --i) {
         if (numerator % 10 != 0) {
            return false;
         }
         numerator /= 10;
      }
      return true;
   }
}

template <typename From, typename To>
concept is_integral_quantity_conversion =
   is_quantity<To> && is_quantity_conversion<From, To>
   && is_integral_quantity_scale<arithmetic_quantity<From>, To>();

template <typename Left, typename Right>
concept is_quantity_addable = is_compatible_quantity<
   arithmetic_quantity<Left>, arithmetic_quantity<Right>>;

template <typename Left, typename Right>
using add_quantity =
   common_quantity<arithmetic_quantity<Left>, arithmetic_quantity<Right>>;

template <typename Left, typename Right>
using multiply_quantity =
   multiplied_quantity<arithmetic_quantity<Left>, arithmetic_quantity<Right>>;

template <typename Left, typename Right>
using divide_quantity =
   divided_quantity<arithmetic_quantity<Left>, arithmetic_quantity<Right>>;

template <typename T, auto new_quantity>
using rebind_quantity_type =
   rebind_quantity<remove_cvref<T>, new_quantity>::type;

}  // namespace detail

}  // namespace cat
