// ChunkedStack.h created on 2019-12-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_CHUNKED_STACK_H
#define XCI_CORE_CHUNKED_STACK_H

#include <memory>
#include <algorithm>
#include <cstdint>
#include <cassert>

#ifdef __linux__
#include <malloc.h>
#elif defined(__APPLE__)
#include <malloc/malloc.h>
#endif

namespace xci::core {


/// Custom stack container.
///
/// Unlike std::stack, this is not an adapter, the container is tailored
/// to act as an stack but also offer forward iteration.
///
/// Features:
/// - stable
///     - iterators are only invalidated by clear() and shrink_to_fit()
///     - references are also stable
/// - memory flexibility
///     - can start small and grow slowly
///     - with initial capacity, it can also start with big chunk of memory
///       and avoid many further allocations
/// - partial memory continuity
///     - memory is allocated in bigger chunks (buckets)
///     - objects in each buckets are continuous
///     - bucket size is configurable
///     - special constructor allocates a single bucket of requested size
/// - debug mode checks
///     - asserts on invalid operations, e.g. top() with empty stack
///     - #define NDEBUG to disable the checks (i.e. switch to Release)
/// - standard behaviour
///     - all methods behave the same as on std::stack or std::deque
///     - (at least, that's the intention, there might be bugs)

template < class T >
class ChunkedStack {

    struct Bucket;  // fwd for iterators
    static constexpr size_t project_capacity(uint32_t prev_cap);

public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;

    explicit ChunkedStack(size_t init_capacity = project_capacity(0));

    ChunkedStack(const ChunkedStack& other);
    ChunkedStack(ChunkedStack&& other) noexcept;
    ~ChunkedStack() { destroy(); }

    ChunkedStack& operator=(const ChunkedStack& other);
    ChunkedStack& operator=(ChunkedStack&& other) noexcept;
    bool operator==(const ChunkedStack& other) const;
    bool operator!=(const ChunkedStack& other) const { return !(*this == other); }

    void swap(ChunkedStack& other) noexcept;

    size_type capacity() const noexcept;
    void shrink_to_fit();  // optimize storage, invalidates references

    bool empty() const;
    size_type size() const;
    void clear() noexcept;

    reference top();
    const_reference top() const;

    template<class ...Args> void emplace(Args&&... args);
    void push(const T& value);
    void push(T&& value);

    void pop();

    // random access
    reference operator[](size_type pos);
    const_reference operator[](size_type pos) const;

    // deque compatibility
    reference back() { return top(); }
    const_reference back() const { return top(); }
    template <class... Args>
    void emplace_back(Args&&... args) { emplace(std::forward<Args>(args)...); }
    void push_back(const value_type& x) { push(x); }
    void push_back(value_type&& x) { push(std::move(x)); }
    void pop_back() { pop(); }

    class iterator {
        friend ChunkedStack;
    public:
        using difference_type = typename ChunkedStack<T>::difference_type;
        using value_type = typename ChunkedStack<T>::value_type;
        using reference = T&;
        using pointer = T*;
        using iterator_category = std::forward_iterator_tag;

        iterator() = default;

        bool operator==(const iterator& rhs) const { return m_bucket == rhs.m_bucket && m_item == rhs.m_item; }
        bool operator!=(const iterator& rhs) const { return m_bucket != rhs.m_bucket || m_item != rhs.m_item; }

        iterator& operator++();
        iterator operator++(int) { auto v = *this; ++*this; return v; }

        reference operator*() const { return m_bucket->items[m_item]; }
        pointer operator->() const { return &m_bucket->items[m_item]; }

    private:
        explicit iterator(Bucket* head)
            : m_head(head), m_bucket{head->count == 0 ? nullptr : head} {}

        Bucket* const m_head = nullptr;
        Bucket* m_bucket = nullptr;
        uint32_t m_item = 0;
    };

    class const_iterator {
        friend ChunkedStack;
    public:
        using difference_type = typename ChunkedStack<T>::difference_type;
        using value_type = typename ChunkedStack<T>::value_type;
        using reference = const T&;
        using pointer = const T*;
        using iterator_category = std::forward_iterator_tag;

        const_iterator() = default;

        bool operator==(const const_iterator& rhs) const { return m_bucket == rhs.m_bucket && m_item == rhs.m_item; }
        bool operator!=(const const_iterator& rhs) const { return m_bucket != rhs.m_bucket || m_item != rhs.m_item; }

        const_iterator& operator++();
        const_iterator operator++(int) { auto v = *this; ++*this; return v; }

        reference operator*() const { return m_bucket->items[m_item]; }
        pointer operator->() const { return &m_bucket->items[m_item]; }

    private:
        explicit const_iterator(Bucket* head)
            : m_head(head), m_bucket{head->count == 0 ? nullptr : head} {}

        const Bucket* const m_head = nullptr;
        const Bucket* m_bucket = nullptr;
        uint32_t m_item = 0;
    };

    iterator begin()                { return iterator{head()}; }
    const_iterator begin() const    { return const_iterator{head()}; }
    const_iterator cbegin() const   { return const_iterator{head()}; }
    iterator end()                  { return iterator{}; }
    const_iterator end() const      { return const_iterator{}; }
    const_iterator cend() const     { return const_iterator{}; }

    // dump current allocation info (the sizes and usage of buckets)
    template <class S> void alloc_info(S& stream);

private:
    struct Bucket {
        Bucket() = default;
        Bucket(Bucket* next, size_t capacity, size_t count)
            : next(next), capacity(uint32_t(capacity)), count(uint32_t(count)), items{} {}
        bool full() const { return count == capacity; }
        Bucket* prev();

        Bucket* next;       // ptr to next bucket or head if this is the tail
        uint32_t capacity;  // number of reserved items
        uint32_t count;     // number of initialized items
        T items[0];         // flexible array member (GNU extension in C++)
    };

    static Bucket* allocate(Bucket* next, size_t capacity, size_t count);
    static void deallocate(Bucket* bucket);
    void destroy();  // deallocate all buckets

    T& push_uninitialized();
    Bucket* head() const { return m_tail->next; }

    // handle to allocated storage:
    // - default ctor doesn't create any buckets
    // - empty buckets are dropped immediately, with exception of the first one
    // - popping the last item from the non-last bucket causes deallocation
    // - buckets are linked in circle, i.e. tail->next == head
    Bucket* m_tail;  // last active bucket (the one containing top item)
};


// ----------------------------------------------------------------------------
// Implementation details


template<class T>
constexpr size_t ChunkedStack<T>::project_capacity(uint32_t prev_cap)
{
    constexpr auto fstep = [](size_t cap_req) {
        return (cap_req * sizeof(T) - sizeof(Bucket)) / sizeof(T);
    };
    constexpr size_t step0 = fstep(16);
    constexpr size_t step1 = fstep(64);
    constexpr size_t step2 = fstep(256);
    constexpr size_t step3 = fstep(1024);
    constexpr size_t step4 = fstep(4096);
    if (prev_cap < step0)
        return step0;
    if (prev_cap < step1)
        return step1;
    if (prev_cap < step2)
        return step2;
    if (prev_cap < step3)
        return step3;
    return step4;
}


template<class T>
ChunkedStack<T>::ChunkedStack(size_t init_capacity)
        : m_tail{allocate(nullptr, init_capacity, 0)}
{
    m_tail->next = m_tail;
}


template<class T>
ChunkedStack<T>::ChunkedStack(const ChunkedStack& other)
    : m_tail{allocate(nullptr, other.size(), other.size())}
{
    m_tail->next = m_tail;
    std::uninitialized_copy(other.cbegin(), other.cend(), m_tail->items);
}


template<class T>
ChunkedStack<T>::ChunkedStack(ChunkedStack&& other) noexcept
{
    std::swap(m_tail, other.m_tail);
}


template<class T>
ChunkedStack<T>& ChunkedStack<T>::operator=(const ChunkedStack& other)  // NOLINT
{
    if (m_tail == other.m_tail)
        return *this;  // self-assignment
    destroy();
    m_tail = allocate(nullptr, other.size(), other.size());
    m_tail->next = m_tail;
    std::uninitialized_copy(other.cbegin(), other.cend(), m_tail->items);
    return *this;
}


template<class T>
ChunkedStack<T>& ChunkedStack<T>::operator=(ChunkedStack&& other) noexcept
{
    std::swap(m_tail, other.m_tail);
    return *this;
}


template<class T>
bool ChunkedStack<T>::operator==(const ChunkedStack& other) const
{
    return size() == other.size() &&
        std::equal(cbegin(), cend(), other.cbegin());
}


template<class T>
void ChunkedStack<T>::swap(ChunkedStack& other) noexcept
{
    std::swap(m_tail, other.m_tail);
}


template<class T>
auto ChunkedStack<T>::capacity() const noexcept -> size_type
{
    size_type res = m_tail->capacity;
    for (auto* b = m_tail->next; b != m_tail; b = b->next) {
        res += b->capacity;
    }
    return res;
}


template<class T>
void ChunkedStack<T>::shrink_to_fit()
{
    auto new_cap = size();
    if (head() == m_tail && new_cap == m_tail->capacity)
        return;  // already at one bucket which fits exactly
    // prepare new bucket
    Bucket* b = allocate(nullptr, new_cap, new_cap);
    b->next = b;
    std::uninitialized_copy(cbegin(), cend(), b->items);
    // destroy old buckets
    destroy();
    m_tail = b;
}


template<class T>
bool ChunkedStack<T>::empty() const
{
    return m_tail->count == 0;
}


template<class T>
auto ChunkedStack<T>::size() const -> ChunkedStack::size_type
{
    size_type res = m_tail->count;
    for (auto* b = m_tail->next; b != m_tail; b = b->next) {
        res += b->count;
    }
    return res;
}


template<class T>
void ChunkedStack<T>::clear() noexcept
{
    // remove all buckets except tail
    for (auto* b = head(); b != m_tail;) {
        Bucket* next = b->next;
        deallocate(b);
        b = next;
    }

    std::destroy_n(m_tail->items, m_tail->count);
    m_tail->next = m_tail;
    m_tail->count = 0;
}


template<class T>
auto ChunkedStack<T>::top() -> reference
{
    assert(!empty());
    return m_tail->items[m_tail->count - 1];
}


template<class T>
auto ChunkedStack<T>::top() const -> const_reference
{
    assert(!empty());
    return m_tail->items[m_tail->count - 1];
}


template<class T>
template<class... Args>
void ChunkedStack<T>::emplace(Args&& ... args)
{
    T& item = push_uninitialized();
    ::new (&item) T(std::forward<Args>(args)...);
}


template<class T>
void ChunkedStack<T>::push(const T& value)
{
    T& item = push_uninitialized();
    ::new (&item) T(value);
}


template<class T>
void ChunkedStack<T>::push(T&& value)
{
    T& item = push_uninitialized();
    ::new (&item) T(std::move(value));
}


template<class T>
void ChunkedStack<T>::pop()
{
    assert(!empty());
    std::destroy_at(&m_tail->items[--m_tail->count]);
    if (m_tail->count == 0 && m_tail != head()) {
        Bucket* b = m_tail->prev();
        b->next = m_tail->next;
        deallocate(m_tail);
        m_tail = b;
    }
}


template<class T>
auto ChunkedStack<T>::operator[](size_type pos) -> reference
{
    assert(!empty());
    assert(pos < size());
    auto* b = head();
    while (pos > b->count) {
        pos -= b->count;
        b = b->next;
    }
    return b->items[pos];
}


template<class T>
auto ChunkedStack<T>::operator[](size_type pos) const -> const_reference
{
    assert(!empty());
    assert(pos < size());
    auto* b = head();
    while (pos > b->count) {
        pos -= b->count;
        b = b->next;
    }
    return b->items[pos];
}


template<class T>
auto ChunkedStack<T>::iterator::operator++() -> iterator&
{
    ++m_item;
    if (m_item >= m_bucket->count) {
        m_bucket = m_bucket->next == m_head ? nullptr : m_bucket->next;
        m_item = 0;
    }
    return *this;
}


template<class T>
auto ChunkedStack<T>::const_iterator::operator++() -> const_iterator&
{
    ++m_item;
    if (m_item >= m_bucket->count) {
        m_bucket = m_bucket->next == m_head ? nullptr : m_bucket->next;
        m_item = 0;
    }
    return *this;
}


template <class T>
template<class S>
void ChunkedStack<T>::alloc_info(S& stream)
{
    for (auto* b = head();; b = b->next) {
        stream
            << "cap " << b->capacity
            << " used " << b->count
            << " size " << sizeof(Bucket) + sizeof(T) * b->capacity
#ifdef __linux__
            << " malloc " << malloc_usable_size(b);
#elif defined(__APPLE__)
            << " malloc " << malloc_size(b);
#else
            /* malloc size unknown */;
#endif
        if (b == head())
            stream << " [head]";
        if (b == m_tail) {
            stream << " [tail]\n";
            break;
        }
        stream << "\n";
    }
}


template<class T>
auto ChunkedStack<T>::allocate(Bucket* next, size_t capacity, size_t count) -> Bucket*
{
    const size_t size = sizeof(Bucket) + capacity * sizeof(T);
    char* buf = new char[size];
    return new(buf) Bucket (next, capacity, count);
}


template<class T>
void ChunkedStack<T>::deallocate(ChunkedStack::Bucket* bucket)
{
    std::destroy_n(bucket->items, bucket->count);
    char* buf = reinterpret_cast<char*>(bucket);
    delete[] buf;
}


template<class T>
void ChunkedStack<T>::destroy()
{
    for (auto* b = m_tail;;) {
        Bucket* next = b->next;
        deallocate(b);
        if (next == m_tail)
            break;
        b = next;
    }
}


template<class T>
auto ChunkedStack<T>::Bucket::prev() -> Bucket*
{
    auto* b = this;
    while (b->next != this)
        b = b->next;
    return b;
}


template<class T>
T& ChunkedStack<T>::push_uninitialized()
{
    if (m_tail->full()) {
        Bucket* b = allocate(head(), project_capacity(m_tail->capacity), 1);
        m_tail->next = b;
        m_tail = b;
        return m_tail->items[0];
    }
    // push to tail
    return m_tail->items[m_tail->count++];
}


} // namespace xci::core

#endif // include guard
