// -*- mode: c++ -*-
#pragma once

// P3480 / C++26. Lane iterators for `cat::simd` and `cat::simd_mask` as
// random-access ranges. `end` is `default_sentinel_t`.
//
// Semantics match the draft. There is no lane subobject inside the pack, so
// `operator*` yields a lane prvalue (`dereference()` returns `value_type` by
// value). `iterator_concept` is `random_access_iterator_tag`. `reference` and
// `pointer` follow P2727 `proxy_iterator_interface`. The `pointer` type is
// `proxy_arrow_result<value_type>`, so `operator->` stores a lane value per
// `proxy_arrow_result`.
//
// Preconditions from C++26. After `advance`, `lane_index()` lies in
// `[0, pack_type::size()]`. `distance_to`, ordering, and iterator difference
// need the same underlying pack pointer (`data_`). Violations trap
// (`__builtin_trap`).

#include <cat/arithmetic>
#include <cat/iterator>
#include <cat/meta>

namespace cat {

template <typename T, typename in_abi>
   requires(is_same<typename in_abi::scalar_type, T>)
class simd;

template <typename T, typename in_abi>
class simd_mask;

namespace detail {

template <typename Pack>
class simd_range_iterator
    : public proxy_iterator_interface<
         random_access_iterator_tag, typename remove_const<Pack>::value_type,
         typename remove_const<Pack>::value_type, iword> {
   using pack_type = remove_const<Pack>;
   using self_type = simd_range_iterator<Pack>;

 public:
   using value_type = typename pack_type::value_type;

 private:
   Pack* data_ = nullptr;
   idx offset_{};

 public:
   constexpr simd_range_iterator() = default;

   constexpr simd_range_iterator(simd_range_iterator const&) = default;

   constexpr simd_range_iterator(Pack* data_in, idx offset_in)
       : data_(data_in), offset_(offset_in) {
   }

   constexpr simd_range_iterator(simd_range_iterator<pack_type> const& other)
      requires(is_const<Pack>)
       : data_(other.data_), offset_(other.offset_) {
   }

   [[nodiscard]]
   constexpr auto
   lane_index() const -> idx {
      return offset_;
   }

   [[nodiscard]]
   constexpr auto
   dereference() -> value_type {
      if (data_ == nullptr || offset_ >= pack_type::size()) {
         __builtin_trap();
      }
      using plain = remove_const<Pack>;
      return (*static_cast<plain const*>(data_))[offset_];
   }

   [[nodiscard]]
   constexpr auto
   dereference() const -> value_type {
      if (data_ == nullptr || offset_ >= pack_type::size()) {
         __builtin_trap();
      }
      using plain = remove_const<Pack>;
      return (*static_cast<plain const*>(data_))[offset_];
   }

   constexpr void
   advance(iword offset) {
      if (data_ == nullptr) {
         __builtin_trap();
      }
      iword const new_off = iword(offset_) + offset;
      iword const sz = iword(pack_type::size());
      if (new_off < iword(0) || new_off > sz) {
         __builtin_trap();
      }
      offset_ = idx(new_off);
   }

   [[nodiscard]]
   constexpr auto
   distance_to(self_type const& other) const -> iword {
      if (data_ != other.data_) {
         __builtin_trap();
      }
      return iword(other.offset_) - iword(offset_);
   }

   [[nodiscard]]
   constexpr auto
   equal_to(self_type const& other) const -> bool {
      return data_ == other.data_ && offset_ == other.offset_;
   }
};

template <typename Pack>
[[nodiscard]]
constexpr auto
operator==(simd_range_iterator<Pack> const& i, default_sentinel_t) -> bool {
   using pack_type = remove_const<Pack>;
   return i.lane_index() == pack_type::size();
}

template <typename Pack>
[[nodiscard]]
constexpr auto
operator==(default_sentinel_t, simd_range_iterator<Pack> const& i) -> bool {
   return i == default_sentinel;
}

template <typename Pack>
[[nodiscard]]
constexpr auto
operator-(simd_range_iterator<Pack> const& i, default_sentinel_t) ->
   typename simd_range_iterator<Pack>::difference_type {
   using pack_type = remove_const<Pack>;
   return iword(i.lane_index()) - iword(pack_type::size());
}

template <typename Pack>
[[nodiscard]]
constexpr auto
operator-(default_sentinel_t, simd_range_iterator<Pack> const& i) ->
   typename simd_range_iterator<Pack>::difference_type {
   using pack_type = remove_const<Pack>;
   return iword(pack_type::size()) - iword(i.lane_index());
}

}  // namespace detail

}  // namespace cat
