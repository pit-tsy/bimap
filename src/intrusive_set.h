#pragma once

#include "bimap_iterator.h"

#include <cstddef>
#include <iterator>
#include <utility>


template<typename Key, typename Compare, typename Tag>
class intrusive_set {
public:
    using node_t = node<Key, Tag>;

public:
    intrusive_set(node_base &sentinel, Compare &&compare = Compare()) noexcept
        : sentinel_(sentinel),
          compare_(std::move(compare)) {
    }

    intrusive_set(node_base &sentinel, const Compare &compare)
        : sentinel_(sentinel),
          compare_(compare) {
    }

    intrusive_set(node_base &sentinel, intrusive_set &&other)
        : sentinel_(sentinel),
          compare_(std::move(other.compare_)) {
    }

    ~intrusive_set() = default;

    void swap(intrusive_set &other) noexcept {
        using std::swap;
        swap(compare_, other.compare_);
    }

    friend void swap(intrusive_set &lhs, intrusive_set &rhs) noexcept {
        lhs.swap(rhs);
    }

    bool empty() const {
        return !is_valid_node(top());
    }

    node_base *lower_bound(const Key &data) const {
        if (top() == nullptr) {
            return &sentinel_;
        }
        return lower_bound(const_cast<node_base *>(top()), data);
    }

    node_base *upper_bound(const Key &data) const {
        auto res = lower_bound(data);
        if (res == &sentinel_ || !equal(get_data(res), data)) {
            return res;
        }
        return res->next();
    }

    node_t *insert(node_t *new_node) {
        if (empty()) {
            sentinel_.left = new_node;
            sentinel_.link_right(new_node);
            new_node->link_right(&sentinel_);
            return new_node;
        }

        auto current = top();
        while (true) {
            if (compare_(get_data(new_node), get_data(current))) {
                if (is_valid_node(current->left)) {
                    current = current->left;
                } else {
                    current->link_left(new_node);
                    if (current == leftmost()) {
                        sentinel_.left = new_node;
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
                return static_cast<node_t *>(current);
            }
        }
    }

    node_base *find(const Key &key) const {
        auto found = lower_bound(key);
        return found != end() && equal(get_data(found), key) ? found : &sentinel_;
    }

    node_base *erase(node_base *pos) {
        if (pos->dad == nullptr) {
          return &sentinel_;
        }

        auto next = pos->next();
        if (pos == leftmost()) {
            sentinel_.left = next;
        }

        if (pos->left == nullptr) {
            if (pos->right != nullptr) {
                pos->right->link_dad_from(pos);
            } else {
                if (pos->dad != &sentinel_) {
                    pos->dad->unlink_son(pos);
                } else {
                    sentinel_.right = &sentinel_;
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
        erase(left_rightmost);
        left_rightmost->link_left(pos->left);
        left_rightmost->link_right(pos->right);
        left_rightmost->link_dad_from(pos);
        return next;
    }

    const node_base *begin() const noexcept {
        return is_valid_node(top()) ? leftmost() : end();
    }

    const node_base *end() const noexcept {
        return &sentinel_;
    }

    bool equal(const Key &a, const Key &b) const {
        return !compare_(a, b) && !compare_(b, a);
    }

private:
    node_base *top() noexcept {
        return sentinel_.right;
    }

    const node_base *top() const noexcept {
        return sentinel_.right;
    }

    const node_base *leftmost() const noexcept {
        return sentinel_.left;
    }

    node_base *rightmost(node_base *current) {
        while (is_valid_node(current->right)) {
            current = current->right;
        }
        return current;
    }

    const Key &get_data(const node_base *node) const {
        return static_cast<const node_t *>(node)->data;
    }

    const Key &get_data(const node_t *node) const {
        return node->data;
    }

    node_base *lower_bound(node_base *current, const Key &data) const {
        if (current == &sentinel_) {
            return current;
        }
        if (compare_(get_data(current), data)) {
            return current->right == nullptr ? &sentinel_ : lower_bound(current->right, data);
        } else {
            if (current->left == nullptr) {
                return current;
            } else {
                node_base *tmp = lower_bound(current->left, data);
                return tmp == &sentinel_ ? current : tmp;
            }
        }
    }

    node_base *upper_bound(node_base *current, const Key &data) const {
        if (current == &sentinel_) {
            return current;
        }
        if (compare_(get_data(current), data)) {
            return current->right == nullptr ? &sentinel_ : upper_bound(current->right, data);
        } else {
            if (current->left == nullptr) {
                return current;
            } else {
                node_base *tmp = lower_bound(current->left, data);
                return tmp == &sentinel_ ? current : tmp;
            }
        }
    }

private:
    template<typename K, typename V, typename C1, typename C2>
    friend class bimap;

    node_base &sentinel_;
    [[no_unique_address]] Compare compare_;
};
