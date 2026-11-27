// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/format>
#include <cat/string>

namespace cat::detail {

template <int decimal_power>
inline constexpr bool has_si_prefix =
   decimal_power == 3 || decimal_power == 6 || decimal_power == 9
   || decimal_power == 12 || decimal_power == 15 || decimal_power == 18
   || decimal_power == 21 || decimal_power == 24 || decimal_power == 27
   || decimal_power == 30;

template <int decimal_power, int power>
inline constexpr int quantity_unit_prefix = [] {
   if constexpr ((power == 0) || (decimal_power % power != 0)) {
      return 0;
   } else {
      constexpr int prefix = decimal_power / power;
      if constexpr (has_si_prefix<prefix>) {
         return prefix;
      } else {
         return 0;
      }
   }
}();

template <int decimal_power>
[[nodiscard]]
constexpr auto
si_prefix() -> str_view {
   if constexpr (decimal_power == 3) {
      return "k";
   } else if constexpr (decimal_power == 6) {
      return "M";
   } else if constexpr (decimal_power == 9) {
      return "G";
   } else if constexpr (decimal_power == 12) {
      return "T";
   } else if constexpr (decimal_power == 15) {
      return "P";
   } else if constexpr (decimal_power == 18) {
      return "E";
   } else if constexpr (decimal_power == 21) {
      return "Z";
   } else if constexpr (decimal_power == 24) {
      return "Y";
   } else if constexpr (decimal_power == 27) {
      return "R";
   } else if constexpr (decimal_power == 30) {
      return "Q";
   } else {
      return "";
   }
}

template <int numerator, int denominator, int prefix_power>
auto
append_quantity_unit(format_context context, str_view symbol, bool& wrote_unit)
   -> scaredy_format<void> {
   if (wrote_unit) {
      $prop(context.append('*'));
   }

   if constexpr (has_si_prefix<prefix_power>) {
      $prop(context.append(si_prefix<prefix_power>()));
   }

   $prop(context.append(symbol));
   constexpr int exponent = numerator < 0 ? -numerator : numerator;

   if constexpr (exponent != 1 || denominator != 1) {
      $prop(context.append('^'));
      $prop(append_arithmetic(context, int4::raw_type(exponent)));
      if constexpr (denominator != 1) {
         $prop(context.append('/'));
         $prop(append_arithmetic(context, int4::raw_type(denominator)));
      }
   }

   wrote_unit = true;
   return monostate;
}

template <int decimal_power, typename Unit>
struct quantity_prefix;

template <int decimal_power>
struct quantity_prefix<decimal_power, unit<>> {
   using unit_type = void;
   static constexpr int power = 0;
};

template <int decimal_power, typename Power, typename... Powers>
struct quantity_prefix<decimal_power, unit<Power, Powers...>> {
 private:
   static constexpr int candidate =
      Power::denominator == 1
         ? quantity_unit_prefix<decimal_power, Power::numerator>
         : 0;
   using remaining = quantity_prefix<decimal_power, unit<Powers...>>;

 public:
   using unit_type = conditional<
      candidate != 0, typename Power::unit_type, typename remaining::unit_type>;
   static constexpr int power = candidate != 0 ? candidate : remaining::power;
};

template <typename Unit>
struct unit_has_denominator;

template <typename... Powers>
struct unit_has_denominator<unit<Powers...>> {
   static constexpr bool value = ((Powers::numerator < 0) || ...);
};

template <bool numerator, int prefix_power, typename PrefixUnit, typename Unit>
struct quantity_unit_formatter;

template <bool numerator, int prefix_power, typename PrefixUnit>
struct quantity_unit_formatter<numerator, prefix_power, PrefixUnit, unit<>> {
   static auto
   append(format_context /*unused*/, bool& /*unused*/) -> scaredy_format<void> {
      return monostate;
   }
};

template <
   bool numerator, int prefix_power, typename PrefixUnit, typename Power,
   typename... Powers>
struct quantity_unit_formatter<
   numerator, prefix_power, PrefixUnit, unit<Power, Powers...>> {
   using power_unit = Power::unit_type;

   static auto
   append(format_context context, bool& wrote_unit) -> scaredy_format<void> {
      if constexpr (
         (numerator && Power::numerator > 0)
         || (!numerator && Power::numerator < 0)
      ) {
         constexpr int selected_prefix =
            __is_same(power_unit, PrefixUnit) ? prefix_power : 0;
         $prop((append_quantity_unit<
                Power::numerator, Power::denominator, selected_prefix>(
            context, str_view(power_unit::unit_symbol.portable().data_),
            wrote_unit
         )));
      }

      return quantity_unit_formatter<
         numerator, prefix_power, PrefixUnit,
         unit<Powers...>>::append(context, wrote_unit);
   }
};

template <is_quantity Quantity>
auto
append_quantity_symbol(format_context context) -> scaredy_format<void> {
   if constexpr (is_dimensionless_quantity<Quantity>) {
      return monostate;
   }

   $prop(context.append(' '));
   using quantity_unit = Quantity::unit_type;
   using prefix = quantity_prefix<Quantity::scale, quantity_unit>;
   using magnitude_type = __typeof_unqual(Quantity::unit_magnitude);

   if constexpr (
      magnitude_type::numerator != 1 || magnitude_type::denominator != 1
   ) {
      $prop(
         append_arithmetic(context, uint8::raw_type(magnitude_type::numerator))
      );
      if constexpr (magnitude_type::denominator != 1) {
         $prop(context.append('/'));
         $prop(append_arithmetic(
            context, uint8::raw_type(magnitude_type::denominator)
         ));
      }
      $prop(context.append(' '));
   }

   if constexpr (prefix::power == 0 && Quantity::scale != 0) {
      $prop(context.append("10^"));
      $prop(append_arithmetic(context, int4::raw_type(Quantity::scale)));
      $prop(context.append(' '));
   }

   if constexpr (magnitude_type::pi_power != 0) {
      $prop(context.append("pi"));
      if constexpr (magnitude_type::pi_power != 1) {
         $prop(context.append('^'));
         $prop(
            append_arithmetic(context, int4::raw_type(magnitude_type::pi_power))
         );
      }
      $prop(context.append(' '));
   }

   bool wrote_unit = false;
   $prop((quantity_unit_formatter<
          true, prefix::power, typename prefix::unit_type,
          quantity_unit>::append(context, wrote_unit)));

   if constexpr (unit_has_denominator<quantity_unit>::value) {
      if (!wrote_unit) {
         $prop(context.append('1'));
      }

      $prop(context.append('/'));
      wrote_unit = false;

      $prop((quantity_unit_formatter<
             false, prefix::power, typename prefix::unit_type,
             quantity_unit>::append(context, wrote_unit)));
   }

   return monostate;
}

template <typename T>
   requires(has_quantity<T>)
auto
append_arithmetic_quantity_symbol(format_context context)
   -> scaredy_format<void> {
   using quantity_specifier = __typeof_unqual(T::quantity_reference);
   if constexpr (is_named_unit<quantity_specifier>) {
      $prop(context.append(' '));

      return context.append(
         str_view(quantity_specifier::unit_symbol.portable().data_)
      );
   } else {
      return append_quantity_symbol<arithmetic_quantity<T>>(context);
   }
}

}  // namespace cat::detail
