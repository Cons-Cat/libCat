#include <cat/linux>

auto
nix::block_all_signals() noexcept(!__has_feature(cxx_exceptions))
   -> signals_mask_set {
   signals_mask_set current_mask;
   // Most Linux runtimes use an `app_mask` which excludes signals 32-34, which
   // are used for something by GNU pthreads. Because libCat doesn't use
   // pthreads, simply block all signals for now.
   // TODO: What should this do when pthreads are enabled?
   sys_rt_sigprocmask(signal_action::block, &all_signals_mask, &current_mask)
      .assert();
   return current_mask;
}
