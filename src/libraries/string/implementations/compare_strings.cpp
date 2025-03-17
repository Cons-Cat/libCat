#include <cat/array>
#include <cat/simd>
#include <cat/string>

auto
cat::detail::compare_strings_detail(str_view const string_1,
                                    str_view const string_2) -> bool {
   if (string_1.size() != string_2.size()) {
      return false;
   }

   // TODO: Use a type for an ISA-specific widest vector.
   using vector = char1x32;

   array<vector, 4u> vectors_1;
   array<vector, 4u> vectors_2;
   array<vector::mask_type, 4u> comparisons;
   idx length_iterator = string_1.size();
   uword vector_size = sizeof(vector);
   char const* p_string_1_iterator = string_1.data();
   char const* p_string_2_iterator = string_2.data();

   auto loop = [&](idx size) -> bool {
      while (length_iterator >= vector_size * size) {
         for (idx i; i < size; ++i) {
            vectors_1[i].load(string_1.data() + (i * size));
            vectors_2[i].load(string_2.data() + (i * size));
            comparisons[i] = (vectors_1[i] == vectors_2[i]);
         }

         for (idx i; i < size; ++i) {
            // If any lanes are not equal to each other:
            if (!comparisons[i].all_of()) {
               return false;
            }
         }

         length_iterator -= vector_size * size;
         p_string_1_iterator += vector_size * size;
         p_string_2_iterator += vector_size * size;
      }

      return true;
   };

   // Compare four, two, then one vectors of characters at a time.
   if (!loop(4u)) {
      return false;
   }
   if (!loop(2u)) {
      return false;
   }
   if (!loop(1u)) {
      return false;
   }

   // Compare remaining characters individually.
   for (idx i = 0u; i < length_iterator;
        ++i, ++p_string_1_iterator, ++p_string_2_iterator) {
      if (*p_string_1_iterator != *p_string_2_iterator) {
         return false;
      }
   }

   return true;
}
