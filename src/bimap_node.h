#pragma once

#include <cstddef>
#include <utility>


struct left_tag;

struct right_tag;

struct node_base;

static bool is_valid_node(const node_base*);

struct node_base {
  node_base* dad = nullptr;
  node_base* left = nullptr;
  node_base* right = nullptr;

  node_base() = default;

  ~node_base() = default;

  void unlink() {
    left = right = dad = nullptr;
  }

  void swap_links(node_base* other) noexcept {
    bool left_son = is_left_son();
    bool other_left_son = other->is_left_son();

    std::swap(left, other->left);
    std::swap(right, other->right);
    std::swap(dad, other->dad);
    repair(other_left_son);
    other->repair(left_son);
  }

  void repair(bool left_son) {
    link_left(left);
    link_right(right);
    if (dad != nullptr) {
      if (left_son) {
        dad->left = this;
      } else {
        dad->right = this;
      }
    }
  }

  void unlink_son(node_base* son) {
    if (left == son) {
      left = nullptr;
    }
    if (right == son) {
      right = nullptr;
    }
    if (son != nullptr && son->dad == this) {
      son->dad = nullptr;
    }
  }

  bool is_left_son() {
    if (dad == nullptr) {
      return false;
    }
    return dad->left == this;
  }

  void link_son(node_base* son, bool is_left) {
    if (is_left) {
      left = son;
    } else {
      right = son;
    }
    son->dad = this;
  }

  void link_left(node_base* new_left) {
    if (new_left != nullptr) {
      new_left->dad = this;
    }
    left = new_left;
  }

  void link_right(node_base* new_right) {
    if (new_right != nullptr) {
      new_right->dad = this;
    }
    right = new_right;
  }

  void link_dad_from(node_base* other) {
    if (other == nullptr || other->dad == nullptr) {
      dad = nullptr;
    } else {
      dad = other->dad;
      other->dad = nullptr;
      if (dad->left == other) {
        dad->left = this;
      } else {
        dad->right = this;
      }
    }
  }

  node_base* next() {
    return const_cast<node_base*>(std::as_const(*this).next());
  }

  const node_base* next() const {
    const node_base* current = this;
    if (current->right == nullptr) {
      while (current->dad->left != current) {
        current = current->dad;
      }
      return current->dad;
    }

    current = current->right;
    while (current->left != nullptr && is_valid_node(current)) {
      current = current->left;
    }

    return current;
  }

  node_base* prev() {
    return const_cast<node_base*>(std::as_const(*this).prev());
  }

  const node_base* prev() const {
    const node_base* current = this;
    if (current->left == nullptr || !is_valid_node(current)) {
      while (current->dad->right != current) {
        current = current->dad;
      }
      return current->dad;
    }

    current = current->left;
    while (current->right != nullptr) {
      current = current->right;
    }

    return current;
  }

protected:
  node_base(node_base* _left, node_base* _right, node_base* _dad)
    : dad(_dad), left(_left), right(_right) {}
};

static bool is_valid_node(const node_base* node) {
  if (node == nullptr) {
    return false;
  }
  if (node->left != nullptr) {
    if (node->left->dad != node) {
      return false;
    }
    return node->left != node->right;
  }
  return true;
}

template <typename Tag>
struct node_sentinel : node_base {
  node_sentinel()
    : node_base(this, this, this) {}

  node_sentinel(node_sentinel&& other) noexcept
    : node_base(std::move(other)) {
    repair();
  }

  node_sentinel& operator=(node_sentinel&& other) noexcept {
    node_base::operator=(std::move(other));
    repair();
    return *this;
  }

  void repair() noexcept {
    if (is_valid_node(this->right)) {
      if (this->dad != nullptr) {
        this->dad->link_right(this);
      }
      this->link_right(this->right);
    } else {
      this->left = this;
      this->right = this;
      this->dad = this;
    }
  }
};

struct bimap_node_sentinel : node_sentinel<left_tag>, node_sentinel<right_tag> {
  node_base& get_left_sentinel() {
    return static_cast<node_base&>(static_cast<node_sentinel<left_tag>&>(*this));
  }

  node_base& get_right_sentinel() {
    return static_cast<node_base&>(static_cast<node_sentinel<right_tag>&>(*this));
  }

  void repair() noexcept {
    node_sentinel<left_tag>::repair();
    node_sentinel<right_tag>::repair();
  }

  // friend void swap(bimap_node_sentinel &lhs, bimap_node_sentinel &rhs) {
  //     std::swap(lhs, rhs);
  //     lhs.repair();
  //     rhs.repair();
  // }
};

template <typename T, typename Tag>
struct node : public node_base {
  using value_type = T;
  using tag = Tag;

  T data;

  node() = delete;

  node(const T& _data)
    : data(_data) {}

  node(T&& _data)
    : data(std::move(_data)) {}

  ~node() = default;
};

template <typename Left, typename Right>
struct bimap_node : node<Left, left_tag>, node<Right, right_tag> {
  bimap_node(const Left& left_data, const Right& right_data)
    : node<Left, left_tag>(left_data),
      node<Right, right_tag>(right_data) {}

  bimap_node(Left&& left_data, const Right& right_data)
    : node<Left, left_tag>(std::move(left_data)),
      node<Right, right_tag>(right_data) {}

  bimap_node(const Left& left_data, Right&& right_data)
    : node<Left, left_tag>(left_data),
      node<Right, right_tag>(std::move(right_data)) {}

  bimap_node(Left&& left_data, Right&& right_data)
    : node<Left, left_tag>(std::move(left_data)),
      node<Right, right_tag>(std::move(right_data)) {}

  ~bimap_node() = default;

  const Left& get_left_data() const {
    return node<Left, left_tag>::data;
  }

  const Right& get_right_data() const {
    return node<Right, right_tag>::data;
  }
};
