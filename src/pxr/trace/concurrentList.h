// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_TRACE_CONCURRENT_LIST_H
#define PXR_TRACE_CONCURRENT_LIST_H

#include <pxr/arch/align.h>

#include <tbb/cache_aligned_allocator.h>

#include <atomic>
#include <iterator>

namespace pxr {

////////////////////////////////////////////////////////////////////////////////
/// \class TraceConcurrentList
///
/// This class supports thread safe insertion and iteration over a list of items.
///
template <typename T>
class TraceConcurrentList {

    // Linked list node that is cache line aligned to prevent false sharing.
    struct alignas(ARCH_CACHE_LINE_SIZE*2) Node {
        T value;
        Node* next;
    };

public:
    ////////////////////////////////////////////////////////////////////////////
    /// \class iterator
    ///
    /// This class provides forward iterator support to iterate over all the
    /// items.
    ///
    class iterator {
    public:
        // iterator types
        using iterator_category = std::forward_iterator_tag;
        using value = T;
        using pointer = T*;
        using reference = T&;
        using difference_type = ptrdiff_t;

        iterator() : _node(nullptr) {}

        pointer operator->() {
            return _node ? &_node->value : nullptr;
        }

        reference operator*() {
            return _node->value;
        }

        iterator& operator++() {
            _node = _node->next;
            return *this;
        }

        iterator operator++(int) {
            iterator result(*this);
            _node = _node->next;
            return result;
        }

        bool operator !=(const iterator& other) const {
            return _node != other._node;
        }

        bool operator ==(const iterator& other) const {
            return _node == other._node;
        }

    private:
        explicit iterator(Node* node) : _node(node) {}
        Node* _node;
        friend class TraceConcurrentList;
    };

    /// Constructor.
    TraceConcurrentList() : _head(nullptr) {}

    /// Destructor.
    ~TraceConcurrentList() {
        // Delete all nodes in the list.
        Node* curNode = _head.load(std::memory_order_acquire);
        while (curNode) {
            Node* nodeToDelete = curNode;
            curNode = curNode->next;
            nodeToDelete->~Node();
            _alloc.deallocate(nodeToDelete, 1);
        }
    }

    // No copies
    TraceConcurrentList(const TraceConcurrentList&) = delete;
    TraceConcurrentList& operator=(const TraceConcurrentList&) = delete;

    /// \name Iterator support.
    /// @{
    iterator begin() { return iterator(_head.load(std::memory_order_acquire)); }
    iterator end() { return iterator(); }
    /// @}

    /// Inserts an item at the beginning of the list and returns an iterator to
    /// the newly created item.
    iterator Insert() {
        Node* newNode = _alloc.allocate(1);
        new(newNode) Node();

        // Add the node to the linked list in an atomic manner.
        do {
            newNode->next = _head.load(std::memory_order_relaxed);
        } while (!_head.compare_exchange_weak(newNode->next, newNode));
        return iterator(newNode);
    }

private:
    std::atomic<Node*> _head;
    tbb::cache_aligned_allocator<Node> _alloc;
};

}  // namespace pxr

#endif // PXR_TRACE_CONCURRENT_LIST_H
