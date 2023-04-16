
#pragma once
#include "default_allocator.hpp"
#include "link_registry.hpp"
#include "sparse_vector.hpp"
#include <memory>

namespace acl
{
template <typename Ty>
struct default_link_container_traits
{
  using size_type                      = uint32_t;
  static constexpr bool     use_sparse = true;
  static constexpr uint32_t pool_size  = 1024;
};

/// @brief Container for link_registry items. Item type must be standard layout, pod like objects.
/// @tparam Ty
/// @tparam Traits
template <typename Ty, typename Traits>
class basic_link_container
{
  using storage_type = acl::detail::aligned_storage<sizeof(Ty), alignof(Ty)>;
  using vector_type =
    std::conditional_t<detail::has_use_sparse_attrib<Traits>, sparse_vector<storage_type, default_allocator<>, Traits>,
                       vector<storage_type>>;
  using size_type = detail::choose_size_t<uint32_t, Traits>;

public:
  using registry = basic_link_registry<Ty, size_type>;
  using link     = typename registry::link;

  vector_type& data()
  {
    return items_;
  }

  vector_type const& data() const
  {
    return items_;
  }

  void sync(registry const& imax)
  {
    items_.resize(imax.max_size());
    if constexpr (acl::detail::debug)
      revisions_.resize(imax.max_size(), 0);
  }

  void resize(size_type imax)
  {
    items_.resize(imax);
    if constexpr (acl::detail::debug)
      revisions_.resize(imax, 0);
  }

  template <typename... Args>
  auto& emplace(link l, Args&&... args)
  {
    Ty* obj = (Ty*)&items_[l.as_index()];
    std::construct_at<Ty>(obj, std::forward<Args>(args)...);
    return *obj;
  }

  void erase(link l)
  {
    if constexpr (!std::is_trivially_destructible_v<Ty>)
      std::destroy_at((Ty*)&items_[l.as_index()]);
    if constexpr (acl::detail::debug)
      revisions_[l.as_index()]++;
  }

  Ty& at(link l)
  {
    if constexpr (acl::detail::debug)
      assert(revisions_[l.as_index()] == l.revision());
    return *(Ty*)&items_[l.as_index()];
  }

  Ty const& at(link l) const
  {
    if constexpr (acl::detail::debug)
      assert(revisions_[l.as_index()] == l.revision());
    return *(Ty const*)&items_[l.as_index()];
  }

private:
  vector_type items_;
#ifdef ACL_DEBUG
  vector<uint8_t> revisions_;
#endif
};

template <typename Ty, typename Traits = default_link_container_traits<Ty>>
using link_container = basic_link_container<Ty, Traits>;

} // namespace acl