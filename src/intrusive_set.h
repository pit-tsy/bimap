#pragma once

#include "bimap_iterator.h"

#include <cstddef>
#include <iterator>
#include <utility>

template <typename Key, typename Compare, typename Tag>
class intrusive_set {
public:
  using node_t = node<Key, Tag>;

public:
  intrusive_set(Compare&& compare = Compare()) noexcept
      : compare_(std::move(compare)) {}

  intrusive_set(const Compare& compare)
      : compare_(compare) {}

  intrusive_set(intrusive_set&& other)
      : compare_(std::move(other.compare_)) {}

  ~intrusive_set() = default;

  void swap(intrusive_set& other) noexcept {
    using std::swap;
    swap(compare_, other.compare_);
  }

  friend void swap(intrusive_set& lhs, intrusive_set& rhs) noexcept {
    lhs.swap(rhs);
  }

  bool empty(node_base* sentinel) const {
    return !is_valid_node(sentinel->right);
  }

  node_base* lower_bound(node_base* sentinel, const Key& data) const {
    if (sentinel->right == nullptr) {
      return sentinel;
    }
    return lower_bound(sentinel, sentinel->right, data);
  }

  node_base* upper_bound(node_base* sentinel, const Key& data) const {
    auto res = lower_bound(sentinel, data);
    if (res == sentinel || !equal(get_data(res), data)) {
      return res;
    }
    return res->next();
  }

  node_t* insert(node_base* sentinel, node_t* new_node) {
    if (empty(sentinel)) {
      sentinel->left = new_node;
      sentinel->link_right(new_node);
      new_node->link_right(sentinel);
      return new_node;
    }

    auto current = sentinel->right;
    while (true) {
      if (compare_(get_data(new_node), get_data(current))) {
        if (is_valid_node(current->left)) {
          current = current->left;
        } else {
          current->link_left(new_node);
          if (current == sentinel->left) {
            sentinel->left = new_node;
          }
          return new_node;
        }
      } else if (compare_(get_data(current), get_data(new_node))) {
        if (is_valid_node(current->right)) {
          current = current->right;
        } else {
          new_node->link_right(current->right);
          current->link_right(new_node);
          return new_node;
        }
      } else {
        return static_cast<node_t*>(current);
      }
    }
  }

  node_base* find(node_base* sentinel, const Key& key) const {
    auto found = lower_bound(sentinel, key);
    return found != sentinel && equal(get_data(found), key) ? found : sentinel;
  }

  node_base* erase(node_base* sentinel, node_base* pos) {
    if (pos->dad == nullptr) {
      return sentinel;
    }

    auto next = pos->next();
    if (pos == sentinel->left) {
      sentinel->left = next;
    }

    if (pos->left == nullptr) {
      if (pos->right != nullptr) {
        pos->right->link_dad_from(pos);
      } else {
        if (pos->dad != sentinel) {
          pos->dad->unlink_son(pos);
        } else {
          sentinel->right = sentinel->left;
        }
      }
      return next;
    }
    if (pos->right == nullptr) {
      pos->left->link_dad_from(pos);
      return next;
    }

    if (pos->left->right == nullptr) {
      pos->left->link_right(pos->right);
      pos->left->link_dad_from(pos);
      return next;
    }

    auto left_rightmost = rightmost(pos->left);
    erase(sentinel, left_rightmost);
    left_rightmost->link_left(pos->left);
    left_rightmost->link_right(pos->right);
    left_rightmost->link_dad_from(pos);
    return next;
  }

  const node_base* begin(node_base* sentinel) const noexcept {
    return is_valid_node(sentinel->right) ? sentinel->left : end(sentinel);
  }

  const node_base* end(node_base* sentinel) const noexcept {
    return sentinel;
  }

  bool equal(const Key& a, const Key& b) const {
    return !compare_(a, b) && !compare_(b, a);
  }

private:
  node_base* rightmost(node_base* current) {
    while (is_valid_node(current->right)) {
      current = current->right;
    }
    return current;
  }

  const Key& get_data(const node_base* node) const {
    return static_cast<const node_t*>(node)->data;
  }

  const Key& get_data(const node_t* node) const {
    return node->data;
  }

  node_base* lower_bound(node_base* sentinel, node_base* current, const Key& data) const {
    if (current == sentinel) {
      return current;
    }
    if (compare_(get_data(current), data)) {
      return current->right == nullptr ? sentinel : lower_bound(sentinel, current->right, data);
    } else {
      if (current->left == nullptr) {
        return current;
      } else {
        node_base* tmp = lower_bound(sentinel, current->left, data);
        return tmp == sentinel ? current : tmp;
      }
    }
  }

  node_base* upper_bound(node_base* sentinel, node_base* current, const Key& data) const {
    if (current == sentinel) {
      return current;
    }
    if (compare_(get_data(current), data)) {
      return current->right == nullptr ? sentinel : upper_bound(current->right, data);
    } else {
      if (current->left == nullptr) {
        return current;
      } else {
        node_base* tmp = lower_bound(current->left, data);
        return tmp == sentinel ? current : tmp;
      }
    }
  }

private:
  template <typename K, typename V, typename C1, typename C2>
  friend class bimap;

  [[no_unique_address]] Compare compare_;
};
