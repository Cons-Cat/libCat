#include <cat/demangle>

cat::demangled_name::demangled_name(
   basic_dyn_allocator<dyn_reallocate> allocator
)
    : m_allocator(allocator) {
}

cat::demangled_name::demangled_name(demangled_name&& other)
    : m_allocator(other.m_allocator),
      m_p_data(other.m_p_data),
      m_size(other.m_size),
      m_capacity(other.m_capacity) {
   other.m_p_data = nullptr;
   other.m_size = 0u;
   other.m_capacity = 0u;
}

auto
cat::demangled_name::operator=(demangled_name&& other) -> demangled_name& {
   if (this == __builtin_addressof(other)) {
      return *this;
   }
   reset();
   m_allocator = other.m_allocator;
   m_p_data = other.m_p_data;
   m_size = other.m_size;
   m_capacity = other.m_capacity;
   other.m_p_data = nullptr;
   other.m_size = 0u;
   other.m_capacity = 0u;
   return *this;
}

cat::demangled_name::~demangled_name() {
   reset();
}

auto
cat::demangled_name::grow(idx minimum_capacity) -> bool {
   idx new_capacity = m_capacity == 0u ? 32u : m_capacity * 2u;
   if (new_capacity < minimum_capacity) {
      new_capacity = minimum_capacity;
   }
   allocator_ref allocator(m_allocator);
   if (m_p_data == nullptr) {
      maybe result = allocator.unalign_alloc_multi_uninit<char>(new_capacity);
      if (result.is_empty()) {
         return false;
      }
      m_p_data = result.value().data();
   } else {
      maybe result = allocator.unalign_realloc_multi_uninit(
         m_p_data, m_capacity, new_capacity
      );
      if (result.is_empty()) {
         return false;
      }
      m_p_data = result.value().data();
   }
   m_capacity = new_capacity;
   return true;
}

auto
cat::demangled_name::append(char value) -> maybe<void> {
   if (m_size == m_capacity && !grow(m_size + 1u)) {
      return nullopt;
   }
   m_p_data[m_size.raw] = value;
   ++m_size;
   return monostate;
}

auto
cat::demangled_name::append(str_view value) -> maybe<void> {
   idx const new_size = m_size + value.size();
   if (new_size > m_capacity && !grow(new_size)) {
      return nullopt;
   }
   for (idx index = 0u; index < value.size(); ++index) {
      m_p_data[m_size.raw + index.raw] = value[index];
   }
   m_size = new_size;
   return monostate;
}

void
cat::demangled_name::reset() {
   if (m_p_data == nullptr) {
      return;
   }
   allocator_ref allocator(m_allocator);
   allocator.free(span(m_p_data, m_capacity));
   m_p_data = nullptr;
   m_size = 0u;
   m_capacity = 0u;
}

namespace cat::detail {

using allocator_type = basic_dyn_allocator<dyn_reallocate>;
using string_type = demangled_name;

struct text_reference {
   cat::idx offset;
   cat::idx size;
};

struct name_result {
   bool final_template = false;
   bool member_const = false;
   bool member_volatile = false;
   bool member_reference = false;
   bool member_rvalue_reference = false;
};

class demangle_parser {
 private:
   static constexpr cat::idx maximum_references = 256u;

 public:
   demangle_parser(allocator_type allocator, cat::str_view input)
       : m_input(input),
         m_result(allocator),
         m_name(allocator),
         m_substitution_text(allocator),
         m_template_text(allocator),
         m_output(&m_result) {
   }

   [[nodiscard]]
   auto
   run() -> cat::maybe<string_type> {
      if (!consume("_Z")) {
         return cat::nullopt;
      }

      m_output = &m_name;
      m_record_template_arguments = true;
      name_result const name = parse_name();
      m_record_template_arguments = false;
      m_output = &m_result;
      if (m_failed) {
         return cat::nullopt;
      }

      if (name.final_template && !at_end()) {
         if (!parse_type() || !append(' ')) {
            return cat::nullopt;
         }
      }
      if (!append(m_name.view())) {
         return cat::nullopt;
      }

      if (!at_end()) {
         if (!append('(')) {
            return cat::nullopt;
         }
         if (peek() == 'v' && remaining() == 1u) {
            ++m_position;
         } else {
            bool first = true;
            while (!at_end()) {
               if (!first && !append(", ")) {
                  return cat::nullopt;
               }
               first = false;
               if (!parse_type()) {
                  return cat::nullopt;
               }
            }
         }
         if (!append(')')) {
            return cat::nullopt;
         }
      }

      if (name.member_const && !append(" const")) {
         return cat::nullopt;
      }
      if (name.member_volatile && !append(" volatile")) {
         return cat::nullopt;
      }
      if (name.member_reference && !append(" &")) {
         return cat::nullopt;
      }
      if (name.member_rvalue_reference && !append(" &&")) {
         return cat::nullopt;
      }
      if (m_failed || !at_end()) {
         return cat::nullopt;
      }
      return cat::move(m_result);
   }

 private:
   [[nodiscard]]
   auto
   at_end() const -> bool {
      return m_position == m_input.size();
   }

   [[nodiscard]]
   auto
   remaining() const -> cat::idx {
      return cat::idx(m_input.size() - m_position);
   }

   [[nodiscard]]
   auto
   peek(cat::idx offset = 0u) const -> char {
      if (offset >= remaining()) {
         return '\0';
      }
      return m_input[m_position + offset];
   }

   auto
   consume(char value) -> bool {
      if (peek() != value) {
         return false;
      }
      ++m_position;
      return true;
   }

   auto
   consume(cat::str_view value) -> bool {
      if (remaining() < value.size()) {
         return false;
      }
      for (cat::idx index = 0u; index < value.size(); ++index) {
         if (m_input[m_position + index] != value[index]) {
            return false;
         }
      }
      m_position += value.size();
      return true;
   }

   auto
   append(char value) -> bool {
      if (m_output->append(value).is_empty()) {
         m_failed = true;
         return false;
      }
      return true;
   }

   auto
   append(cat::str_view value) -> bool {
      if (m_output->append(value).is_empty()) {
         m_failed = true;
         return false;
      }
      return true;
   }

   [[nodiscard]]
   auto
   output_slice(cat::idx start) -> cat::str_view {
      cat::str_view output = m_output->view();
      return output.substring(start, cat::idx(output.size() - start));
   }

   auto
   save_text(
      string_type& storage, text_reference* _Nonnull p_references,
      cat::idx& count, cat::str_view text
   ) -> bool {
      if (count == maximum_references) {
         m_failed = true;
         return false;
      }
      text_reference const reference = {
         .offset = storage.size(),
         .size = text.size(),
      };
      if (storage.append(text).is_empty()) {
         m_failed = true;
         return false;
      }
      p_references[count.raw] = reference;
      ++count;
      return true;
   }

   auto
   add_substitution(cat::idx start) -> bool {
      cat::str_view const text = output_slice(start);
      if (text.is_empty()) {
         m_failed = true;
         return false;
      }
      if (m_substitution_count > 0u) {
         text_reference const last =
            m_substitutions[cat::idx(m_substitution_count - 1u).raw];
         cat::str_view storage = m_substitution_text.view();
         if (storage.substring(last.offset, last.size) == text) {
            return true;
         }
      }
      return save_text(
         m_substitution_text, m_substitutions, m_substitution_count, text
      );
   }

   [[nodiscard]]
   auto
   reference_text(string_type& storage, text_reference const& reference)
      -> cat::str_view {
      cat::str_view view = storage.view();
      return view.substring(reference.offset, reference.size);
   }

   [[nodiscard]]
   auto
   parse_number(cat::idx& value) -> bool {
      if (peek() < '0' || peek() > '9') {
         return false;
      }
      value = 0u;
      do {
         cat::idx const digit = cat::idx(peek() - '0');
         if (value > (cat::idx::max() - digit) / 10u) {
            m_failed = true;
            return false;
         }
         value = value * 10u + digit;
         ++m_position;
      } while (peek() >= '0' && peek() <= '9');
      return true;
   }

   [[nodiscard]]
   auto
   parse_source_name(cat::str_view& identifier) -> bool {
      cat::idx length;
      if (!parse_number(length) || length == 0u || length > remaining()) {
         m_failed = true;
         return false;
      }
      identifier = {
         m_input.data() + m_position,
         length,
      };
      m_position += length;
      return append(identifier);
   }

   [[nodiscard]]
   auto
   parse_sequence_id(cat::idx& index) -> bool {
      if (consume('_')) {
         index = 0u;
         return true;
      }

      cat::idx value = 0u;
      bool found = false;
      while (true) {
         char const character = peek();
         cat::idx digit;
         if (character >= '0' && character <= '9') {
            digit = cat::idx(character - '0');
         } else if (character >= 'A' && character <= 'Z') {
            digit = cat::idx(character - 'A' + 10);
         } else {
            break;
         }
         found = true;
         if (value > (cat::idx::max() - digit) / 36u) {
            m_failed = true;
            return false;
         }
         value = value * 36u + digit;
         ++m_position;
      }
      if (!found || !consume('_') || value == cat::idx::max()) {
         m_failed = true;
         return false;
      }
      index = value + 1u;
      return true;
   }

   [[nodiscard]]
   auto
   parse_standard_substitution() -> bool {
      cat::str_view replacement;
      switch (peek()) {
         case 't':
            replacement = "std";
            break;
         case 'a':
            replacement = "std::allocator";
            break;
         case 'b':
            replacement = "std::basic_string";
            break;
         case 's':
            replacement = "std::basic_string<char, std::char_traits<char>, "
                          "std::allocator<char> >";
            break;
         case 'i':
            replacement = "std::basic_istream<char, std::char_traits<char> >";
            break;
         case 'o':
            replacement = "std::basic_ostream<char, std::char_traits<char> >";
            break;
         case 'd':
            replacement = "std::basic_iostream<char, std::char_traits<char> >";
            break;
         default:
            return false;
      }
      ++m_position;
      return append(replacement);
   }

   [[nodiscard]]
   auto
   parse_substitution() -> bool {
      if (!consume('S')) {
         return false;
      }
      if (parse_standard_substitution()) {
         return true;
      }

      cat::idx index;
      if (!parse_sequence_id(index) || index >= m_substitution_count) {
         m_failed = true;
         return false;
      }
      return append(
         reference_text(m_substitution_text, m_substitutions[index.raw])
      );
   }

   [[nodiscard]]
   auto
   parse_template_parameter() -> bool {
      if (!consume('T')) {
         return false;
      }
      cat::idx index;
      if (!parse_sequence_id(index) || index >= m_template_count) {
         m_failed = true;
         return false;
      }
      return append(
         reference_text(m_template_text, m_template_arguments[index.raw])
      );
   }

   [[nodiscard]]
   auto
   parse_operator() -> bool {
      static constexpr char operators[] = "nwoperator new\0"
                                          "naoperator new[]\0"
                                          "dloperator delete\0"
                                          "daoperator delete[]\0"
                                          "psoperator+\0"
                                          "ngoperator-\0"
                                          "adoperator&\0"
                                          "deoperator*\0"
                                          "cooperator~\0"
                                          "ploperator+\0"
                                          "mioperator-\0"
                                          "mloperator*\0"
                                          "dvoperator/\0"
                                          "rmoperator%\0"
                                          "anoperator&\0"
                                          "oroperator|\0"
                                          "eooperator^\0"
                                          "aSoperator=\0"
                                          "pLoperator+=\0"
                                          "mIoperator-=\0"
                                          "mLoperator*=\0"
                                          "dVoperator/=\0"
                                          "rMoperator%=\0"
                                          "aNoperator&=\0"
                                          "oRoperator|=\0"
                                          "eOoperator^=\0"
                                          "lsoperator<<\0"
                                          "rsoperator>>\0"
                                          "lSoperator<<=\0"
                                          "rSoperator>>=\0"
                                          "eqoperator==\0"
                                          "neoperator!=\0"
                                          "ltoperator<\0"
                                          "gtoperator>\0"
                                          "leoperator<=\0"
                                          "geoperator>=\0"
                                          "ssoperator<=>\0"
                                          "ntoperator!\0"
                                          "aaoperator&&\0"
                                          "oooperator||\0"
                                          "ppoperator++\0"
                                          "mmoperator--\0"
                                          "cmoperator,\0"
                                          "pmoperator->*\0"
                                          "ptoperator->\0"
                                          "cloperator()\0"
                                          "ixoperator[]\0"
                                          "quoperator?\0";
      char const* p_entry = operators;
      while (*p_entry != '\0') {
         cat::str_view const replacement = p_entry + 2u;
         if (p_entry[0u] == peek() && p_entry[1u] == peek(1u)) {
            m_position += 2u;
            return append(replacement);
         }
         p_entry += replacement.size().raw + 3u;
      }
      return false;
   }

   [[nodiscard]]
   auto
   parse_literal_template_argument() -> bool {
      if (!consume('L')) {
         return false;
      }
      char const type = peek();
      bool const integral = type == 'b' || type == 'c' || type == 'a'
                            || type == 'h' || type == 's' || type == 't'
                            || type == 'i' || type == 'j' || type == 'l'
                            || type == 'm' || type == 'x' || type == 'y';
      if (!integral) {
         m_failed = true;
         return false;
      }
      ++m_position;

      bool const negative = consume('n');
      cat::idx const digits_start = m_position;
      cat::idx ignored;
      if (!parse_number(ignored) || !consume('E')) {
         m_failed = true;
         return false;
      }
      cat::idx const digits_end = cat::idx(m_position - 1u);
      if (type == 'b') {
         if (negative || ignored > 1u) {
            m_failed = true;
            return false;
         }
         return append(
            ignored == 0u ? cat::str_view("false") : cat::str_view("true")
         );
      }
      if (negative && !append('-')) {
         return false;
      }
      return append(
         {m_input.data() + digits_start, cat::idx(digits_end - digits_start)}
      );
   }

   // NOLINTNEXTLINE(misc-no-recursion)
   [[nodiscard]]
   auto
   parse_template_argument() -> bool {
      if (peek() == 'L') {
         return parse_literal_template_argument();
      }
      if (peek() == 'X') {
         m_failed = true;
         return false;
      }
      return parse_type();
   }

   // NOLINTNEXTLINE(misc-no-recursion)
   [[nodiscard]]
   auto
   parse_template_arguments() -> bool {
      if (!consume('I') || !append('<')) {
         return false;
      }

      ++m_template_depth;
      bool first = true;
      while (!at_end() && peek() != 'E') {
         if (!first && !append(", ")) {
            return false;
         }
         first = false;
         cat::idx const argument_start = m_output->size();
         if (!parse_template_argument()) {
            return false;
         }
         if (
            m_record_template_arguments && m_template_depth == 1u
            && !save_text(
               m_template_text, m_template_arguments, m_template_count,
               output_slice(argument_start)
            )
         ) {
            return false;
         }
      }
      if (first || !consume('E')) {
         m_failed = true;
         return false;
      }
      m_template_depth.raw -= 1u;
      if (!m_output->is_empty() && m_output->back() == '>' && !append(' ')) {
         return false;
      }
      return append('>');
   }

   // NOLINTNEXTLINE(misc-no-recursion)
   [[nodiscard]]
   auto
   parse_component(cat::idx prefix_start, bool& is_template) -> bool {
      is_template = false;
      if (peek() >= '0' && peek() <= '9') {
         cat::str_view identifier;
         if (!parse_source_name(identifier)) {
            return false;
         }
         m_last_identifier = identifier;
         if (peek() == 'I') {
            if (!add_substitution(prefix_start)) {
               return false;
            }
            if (!parse_template_arguments()) {
               return false;
            }
            is_template = true;
         }
         return add_substitution(prefix_start);
      }

      if (peek() == 'S') {
         if (!parse_substitution()) {
            return false;
         }
         if (peek() == 'I') {
            if (!parse_template_arguments()) {
               return false;
            }
            is_template = true;
            return add_substitution(prefix_start);
         }
         return true;
      }

      if (
         (peek() == 'C' && (peek(1u) == '1' || peek(1u) == '2'))
         || (peek() == 'D' && (peek(1u) == '0' || peek(1u) == '1' || peek(1u) == '2'))
      ) {
         bool const destructor = peek() == 'D';
         m_position += 2u;
         if (m_last_identifier.is_empty()) {
            m_failed = true;
            return false;
         }
         return (!destructor || append('~')) && append(m_last_identifier);
      }

      return parse_operator();
   }

   // NOLINTNEXTLINE(misc-no-recursion)
   [[nodiscard]]
   auto
   parse_nested_name() -> name_result {
      name_result result;
      if (!consume('N')) {
         m_failed = true;
         return result;
      }
      while (peek() == 'K' || peek() == 'V' || peek() == 'R' || peek() == 'O') {
         if (consume('K')) {
            result.member_const = true;
         } else if (consume('V')) {
            result.member_volatile = true;
         } else if (consume('R')) {
            result.member_reference = true;
         } else {
            consume('O');
            result.member_rvalue_reference = true;
         }
      }

      cat::idx const prefix_start = m_output->size();
      bool first = true;
      while (!at_end() && peek() != 'E') {
         if (!first && !append("::")) {
            return result;
         }
         first = false;
         bool is_template = false;
         if (!parse_component(prefix_start, is_template)) {
            m_failed = true;
            return result;
         }
         result.final_template = is_template;
      }
      if (first || !consume('E')) {
         m_failed = true;
      }
      return result;
   }

   // NOLINTNEXTLINE(misc-no-recursion)
   [[nodiscard]]
   auto
   parse_name() -> name_result {
      if (peek() == 'N') {
         return parse_nested_name();
      }

      name_result result;
      cat::idx const start = m_output->size();
      if (!parse_component(start, result.final_template)) {
         m_failed = true;
      }
      return result;
   }

   [[nodiscard]]
   auto
   parse_builtin_type() -> bool {
      cat::str_view name;
      switch (peek()) {
         case 'v':
            name = "void";
            break;
         case 'w':
            name = "wchar_t";
            break;
         case 'b':
            name = "bool";
            break;
         case 'c':
            name = "char";
            break;
         case 'a':
            name = "signed char";
            break;
         case 'h':
            name = "unsigned char";
            break;
         case 's':
            name = "short";
            break;
         case 't':
            name = "unsigned short";
            break;
         case 'i':
            name = "int";
            break;
         case 'j':
            name = "unsigned int";
            break;
         case 'l':
            name = "long";
            break;
         case 'm':
            name = "unsigned long";
            break;
         case 'x':
            name = "long long";
            break;
         case 'y':
            name = "unsigned long long";
            break;
         case 'n':
            name = "__int128";
            break;
         case 'o':
            name = "unsigned __int128";
            break;
         case 'f':
            name = "float";
            break;
         case 'd':
            name = "double";
            break;
         case 'e':
            name = "long double";
            break;
         case 'g':
            name = "__float128";
            break;
         case 'z':
            name = "...";
            break;
         default:
            return false;
      }
      ++m_position;
      return append(name);
   }

   // NOLINTNEXTLINE(misc-no-recursion)
   [[nodiscard]]
   auto
   parse_type() -> bool {
      cat::idx const start = m_output->size();
      if (parse_builtin_type()) {
         return true;
      }

      if (consume('P')) {
         return parse_type() && append('*') && add_substitution(start);
      }
      if (consume('R')) {
         return parse_type() && append('&') && add_substitution(start);
      }
      if (consume('O')) {
         return parse_type() && append("&&") && add_substitution(start);
      }
      if (consume('K')) {
         return parse_type() && append(" const") && add_substitution(start);
      }
      if (consume('V')) {
         return parse_type() && append(" volatile") && add_substitution(start);
      }
      if (consume('r')) {
         return parse_type() && append(" restrict") && add_substitution(start);
      }
      if (peek() == 'T') {
         return parse_template_parameter();
      }
      if (peek() == 'S') {
         bool const standard_namespace = peek(1u) == 't';
         if (!parse_substitution()) {
            return false;
         }
         if (standard_namespace && peek() >= '0' && peek() <= '9') {
            bool is_template = false;
            return append("::") && parse_component(start, is_template);
         }
         if (peek() == 'I') {
            return parse_template_arguments() && add_substitution(start);
         }
         return true;
      }
      if (peek() == 'N' || (peek() >= '0' && peek() <= '9')) {
         [[maybe_unused]]
         name_result const name = parse_name();
         return !m_failed;
      }

      m_failed = true;
      return false;
   }

   cat::str_view m_input;
   cat::idx m_position = 0u;
   string_type m_result;
   string_type m_name;
   string_type m_substitution_text;
   string_type m_template_text;
   string_type* _Nonnull m_output;
   text_reference m_substitutions[maximum_references.raw];
   text_reference m_template_arguments[maximum_references.raw];
   cat::idx m_substitution_count = 0u;
   cat::idx m_template_count = 0u;
   cat::idx m_template_depth = 0u;
   cat::str_view m_last_identifier;
   bool m_record_template_arguments = false;
   bool m_failed = false;
};

}  // namespace cat::detail

auto
cat::demangle(
   basic_dyn_allocator<dyn_reallocate> allocator, str_view const mangled
) -> maybe<demangled_name> {
   if (mangled.size() >= 2u && mangled[0u] == '_' && mangled[1u] == 'Z') {
      detail::demangle_parser demangler(allocator, mangled);
      maybe<demangled_name> result = demangler.run();
      if (result.has_value()) {
         return move(result.value());
      }
   }

   // Parsing is transactional. Unsupported or malformed names are copied
   // unchanged instead of exposing parser output.
   demangled_name result(allocator);
   if (result.append(mangled).is_empty()) {
      return nullopt;
   }
   return move(result);
}
