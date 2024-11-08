#pragma once

#include "bimap_node.h"

#include <iterator>
#include <utility>

template<typename BimapNode, typename Node, typename FlipNode>
class bimap_iterator {
public:
    using value_type = typename Node::value_type;
    using tag = typename Node::tag;
    using pointer = const value_type *;
    using reference = const value_type &;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;

    bimap_iterator() = default;

    reference operator*() const {
        return static_cast<Node const *>(ptr_)->data;
    }

    pointer operator->() const {
        return &static_cast<Node const *>(ptr_)->data;
    }

    bimap_iterator &operator++() {
        ptr_ = ptr_->next();
        return *this;
    }

    bimap_iterator operator++(int) {
        bimap_iterator tmp = *this;
        ++*this;
        return tmp;
    }

    bimap_iterator &operator--() {
        ptr_ = ptr_->prev();
        return *this;
    }

    bimap_iterator operator--(int) {
        bimap_iterator tmp = *this;
        --*this;
        return tmp;
    }

    bool operator==(const bimap_iterator &other) const {
        return ptr_ == other.ptr_;
    }

    bool operator!=(const bimap_iterator &other) const {
        return !(*this == other);
    }

    bimap_iterator<BimapNode, FlipNode, Node> flip() const {
        if (is_valid_node(ptr_)) {
            return static_cast<const FlipNode *>(static_cast<const BimapNode *>(static_cast<const Node *>(ptr_)));
        } else {
            return static_cast<const node_sentinel<typename FlipNode::tag> *>(static_cast<const bimap_node_sentinel *>(
                static_cast<const node_sentinel<tag> *>(ptr_)));
        }
    }

private:
    template<typename K, typename V, typename C1, typename C2>
    friend class bimap;

    template<typename B, typename N, typename F>
    friend class bimap_iterator;


    bimap_iterator(const node_base *ptr)
        : ptr_(ptr) {
    }

    const node_base *ptr() const {
        return ptr_;
    }

    node_base *ptr() {
        return const_cast<node_base *>(ptr_);
    }

protected:
    const node_base *ptr_;
};
