#include <cat/runtime>
#include <cat/span>

auto
cat::detail::get_command_arguments() noexcept
   -> span<char const* _Nonnull const> {
   char const* _Nonnull const* _Nullable const pp_arguments =
      pp_command_arguments;
   if (pp_arguments == nullptr) {
      return nullptr;
   }
   return {pp_arguments, command_arguments_count};
}
