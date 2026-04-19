#include <cat/array>
#include <cat/simd>
#include <cat/string>

auto
cat::detail::compare_strings_detail(str_view const string_1,
                                    str_view const string_2) -> bool {
   if (string_1.size() != string_2.size()) {
      return false;
   }

   using vector = char1x_;

   array<vector, 4u> vectors_1;
   array<vector, 4u> vectors_2;
   array<vector::mask_type, 4u> comparisons;
   idx length_iterator = string_1.size();
   uword vector_size = sizeof(vector);
   char const* p_string_1_iterator = string_1.data();
   char const* p_string_2_iterator = string_2.data();

   auto loop = [&](idx batch_count) -> bool {
      while (length_iterator >= vector_size * batch_count) {
         for (idx i = 0u; i < batch_count; ++i) {
            char const* const chunk_1 =
               p_string_1_iterator + i.raw * vector_size.raw;
            char const* const chunk_2 =
               p_string_2_iterator + i.raw * vector_size.raw;
            vectors_1[i].load_unaligned(chunk_1);
            vectors_2[i].load_unaligned(chunk_2);
            comparisons[i] = vectors_1[i].equal_lanes(vectors_2[i]);
         }

         for (idx i = 0u; i < batch_count; ++i) {
            // If any lanes are not equal to each other:
            if (!comparisons[i].all_of()) {
               return false;
            }
         }

         length_iterator.raw -= vector_size.raw * batch_count.raw;
         p_string_1_iterator += vector_size.raw * batch_count.raw;
         p_string_2_iterator += vector_size.raw * batch_count.raw;
      }

      return true;
   };

   // Compare four, two, then one vectors of characters at a time.
   if (!loop(4_idx)) {
      return false;
   }
   if (!loop(2_idx)) {
      return false;
   }
   if (!loop(1_idx)) {
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
