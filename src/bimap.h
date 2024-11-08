#pragma once

#include "bimap_iterator.h"
#include "intrusive_set.h"

#include <cstddef>
#include <stdexcept>

template <
    typename Left,
    typename Right,
    typename CompareLeft = std::less<Left>,
    typename CompareRight = std::less<Right>>
class bimap {
public:
  using left_t = Left;
  using right_t = Right;

  using node_t = bimap_node<Left, Right>;
  using left_node_t = node<Left, left_tag>;
  using right_node_t = node<Right, right_tag>;

  using right_iterator = bimap_iterator<node_t, right_node_t, left_node_t>;
  using left_iterator = bimap_iterator<node_t, left_node_t, right_node_t>;

public:
  bimap(CompareLeft compare_left = CompareLeft(), CompareRight compare_right = CompareRight())
      : left_set(sentinel_.get_left_sentinel(), std::move(compare_left))
      , right_set(sentinel_.get_right_sentinel(), std::move(compare_right)) {}

  bimap(const bimap& other)
      : left_set(sentinel_.get_left_sentinel(), other.left_set.compare_)
      , right_set(sentinel_.get_right_sentinel(), other.right_set.compare_) {
    try {
      for (auto it = other.begin_left(); it != other.end_left(); ++it) {
        const node_t* other_node = static_cast<node_t*>(static_cast<left_node_t*>(it.ptr()));
        insert(new node_t(other_node->get_left_data(), other_node->get_right_data()));
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  bimap(bimap&& other) noexcept
      : sentinel_(std::move(other.sentinel_))
      , left_set(sentinel_.get_left_sentinel(), std::move(other.left_set))
      , right_set(sentinel_.get_right_sentinel(), std::move(other.right_set))
      , size_(std::exchange(other.size_, 0)) {
    other.sentinel_.get_left_sentinel().unlink();
    other.sentinel_.get_right_sentinel().unlink();
  }

  bimap& operator=(const bimap& other) {
    if (&other != this) {
      bimap temp(other);
      *this = std::move(temp);
    }
    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    if (&other != this) {
      clear();
      swap(other);
    }
    return *this;
  }

  ~bimap() {
    clear();
  }

  void clear() noexcept {
    while (!empty()) {
      erase(begin_left());
    }
  }

  void swap(bimap& other) noexcept {
    using std::swap;
    swap(sentinel_, other.sentinel_);
    swap(left_set, other.left_set);
    swap(right_set, other.right_set);
    swap(size_, other.size_);
  }

  friend void swap(bimap& lhs, bimap& rhs) noexcept {
    lhs.swap(rhs);
  }

  left_iterator insert(const left_t& left, const right_t& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }
    node_t* new_node = new node_t(left, right);
    return insert(new_node);
  }

  left_iterator insert(const left_t& left, right_t&& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }
    node_t* new_node = new node_t(left, std::move(right));
    return insert(new_node);
  }

  left_iterator insert(left_t&& left, const right_t& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }
    node_t* new_node = new node_t(std::move(left), right);
    return insert(new_node);
  }

  left_iterator insert(left_t&& left, right_t&& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }
    node_t* new_node = new node_t(std::move(left), std::move(right));
    return insert(new_node);
  }

  left_iterator erase_left(left_iterator it) {
    if (it == end_left()) {
      return end_left();
    }
    return erase(it);
  }

  right_iterator erase_right(right_iterator it) {
    if (it == end_right()) {
      return end_right();
    }
    return erase(it);
  }

  bool erase_left(const left_t& left) {
    auto found = find_left(left);
    if (found == end_left()) {
      return false;
    } else {
      erase(found);
      return true;
    }
  }

  bool erase_right(const right_t& right) {
    auto found = find_right(right);
    if (found == end_right()) {
      return false;
    } else {
      erase(found);
      return true;
    }
  }

  left_iterator erase_left(left_iterator first, left_iterator last) {
    auto it = first;
    while (it != last) {
      erase(it++);
    }
    return last;
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
    auto it = first;
    while (it != last) {
      erase(it++);
    }
    return last;
  }

  left_iterator find_left(const left_t& left) const {
    return left_set.find(left);
  }

  right_iterator find_right(const right_t& right) const {
    return right_set.find(right);
  }

  const right_t& at_left(const left_t& key) const {
    auto found = find_left(key);
    if (found == end_left()) {
      throw std::out_of_range("key not found");
    }
    return *found.flip();
  }

  const left_t& at_right(const right_t& key) const {
    auto found = find_right(key);
    if (found == end_right()) {
      throw std::out_of_range("key not found");
    }
    return *found.flip();
  }

  const right_t& at_left_or_default(const left_t& key) {
    auto found = find_left(key);
    if (found != end_left()) {
      return *found.flip();
    }

    if constexpr (std::is_default_constructible_v<right_t>) {
      // erase_right(right_t());
      return *insert(new node_t(key, right_t())).flip();
    } else {
      throw;
    }
  }

  const left_t& at_right_or_default(const right_t& key) {
    auto found = find_right(key);
    if (found != end_right()) {
      return *found.flip();
    }

    if constexpr (std::is_default_constructible_v<left_t>) {
      // erase_left(left_t());
      return *insert(new node_t(left_t(), key));
    } else {
      throw;
    }
  }

  left_iterator lower_bound_left(const left_t& left) const {
    return left_set.lower_bound(left);
  }

  left_iterator upper_bound_left(const left_t& left) const {
    return left_set.upper_bound(left);
  }

  right_iterator lower_bound_right(const right_t& right) const {
    return right_set.lower_bound(right);
  }

  right_iterator upper_bound_right(const right_t& right) const {
    return right_set.upper_bound(right);
  }

  left_iterator begin_left() const {
    return left_set.begin();
  }

  left_iterator end_left() const {
    return left_set.end();
  }

  right_iterator begin_right() const {
    return right_set.begin();
  }

  right_iterator end_right() const {
    return right_set.end();
  }

  bool empty() const {
    return size() == 0;
  }

  std::size_t size() const {
    return size_;
  }

  friend bool operator==(const bimap& lhs, const bimap& rhs) {
    if (lhs.size() != rhs.size()) {
      return false;
    }

    auto lhs_it = lhs.begin_left();
    auto rhs_it = rhs.begin_left();
    while (lhs_it != lhs.end_left()) {
      if (!lhs.left_set.equal(*lhs_it, *rhs_it) || !rhs.right_set.equal(*lhs_it.flip(), *rhs_it.flip())) {
        return false;
      }

      ++lhs_it;
      ++rhs_it;
    }

    return true;
  }

  friend bool operator!=(const bimap& lhs, const bimap& rhs) {
    return !(lhs == rhs);
  }

private:
  left_iterator insert(node_t* new_node) {
    auto new_left_node = static_cast<left_node_t*>(new_node);
    auto new_right_node = static_cast<right_node_t*>(new_node);

    node_base* inserted_right;
    node_base* inserted_left;
    try {
      inserted_right = right_set.insert(new_right_node);
    } catch (...) {
      delete new_node;
      throw;
    }
    try {
      inserted_left = left_set.insert(new_left_node);
    } catch (...) {
      if (inserted_right == new_right_node) {
        right_set.erase(new_right_node);
      }
      delete new_node;
      throw;
    }

    if (inserted_left != new_left_node && inserted_right != new_right_node) {
      delete new_node;
      return inserted_left;
    }
    if (inserted_left != new_left_node) {
      new_left_node->unlink();
      inserted_left->swap_links(new_left_node);
      if (sentinel_.get_left_sentinel().left == inserted_left) {
        sentinel_.get_left_sentinel().left = new_left_node;
      }
      erase(left_iterator(inserted_left));
    }
    if (inserted_right != new_right_node) {
      new_right_node->unlink();
      inserted_right->swap_links(new_right_node);
      if (sentinel_.get_right_sentinel().left == inserted_right) {
        sentinel_.get_right_sentinel().left = new_right_node;
      }
      erase(right_iterator(inserted_right));
    }

    ++size_;
    return static_cast<node_base*>(new_left_node);
  }

  left_iterator erase(left_iterator pos) noexcept {
    return erase(static_cast<node_t*>(static_cast<left_node_t*>(pos.ptr())));
  }

  right_iterator erase(right_iterator pos) noexcept {
    return erase(static_cast<node_t*>(static_cast<right_node_t*>(pos.ptr()))).flip();
  }

  left_iterator erase(node_t* node) noexcept {
    auto left_it = left_set.erase(static_cast<left_node_t*>(node));
    right_set.erase(static_cast<right_node_t*>(node));
    delete node;
    --size_;

    return static_cast<left_node_t*>(left_it);
  }

private:
  bimap_node_sentinel sentinel_;
  [[no_unique_address]] intrusive_set<Left, CompareLeft, left_tag> left_set;
  [[no_unique_address]] intrusive_set<Right, CompareRight, right_tag> right_set;
  int size_{0};
};
