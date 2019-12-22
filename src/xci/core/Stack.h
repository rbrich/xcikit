// Stack.h created on 2019-12-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_STACK_H
#define XCI_CORE_STACK_H

#include <memory>
#include <algorithm>
#include <cstdint>
#include <cassert>

namespace xci::core {


/// Custom stack container.
///
/// Unlike std::stack, this is not an adapter, the container is tailored
/// to act as an stack but also offer some methods of deque and vector
/// like begin(), end(), reserve().
///
/// Features:
/// - stable
///     - iterators are only invalidated by clear() and shrink_to_fit()
///     - references are also stable
/// - memory flexibility
///     - can start small or without any allocation and grow slowly
///     - with reserve, it can also start with big chunk of memory
///       and avoid many further allocations
/// - partial memory continuity
///     - memory is allocated in bigger chunks (buckets)
///     - objects in each buckets are continuous
///     - bucket size is configurable
///     - reserve() allocates single bucket of requested size
/// - debug mode checks
///     - asserts on invalid operations, e.g. top() with empty stack
///     - #define NDEBUG to disable the checks (i.e. switch to Release)
/// - standard behaviour
///     - all methods behave the same as on std::stack or std::deque
///     - (at least, that's the intention, there might be bugs)

template < class T >
class Stack {

    struct Bucket;  // fwd for iterators

public:
    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;
    typedef std::ptrdiff_t difference_type;
    typedef std::size_t size_type;

    Stack() = default;
    Stack(const Stack& other);
    Stack(Stack&& other) noexcept;
    ~Stack() { Bucket::destroy(m_head); }

    Stack& operator=(const Stack& other);
    Stack& operator=(Stack&& other) noexcept;
    bool operator==(const Stack& other) const;
    bool operator!=(const Stack& other) const { return !(*this == other); }

    void swap(Stack& other) noexcept;

    static constexpr uint32_t initial_capacity() { return Bucket::project_capacity(0); }
    size_type capacity() const noexcept;
    void reserve(size_type new_capacity);
    void shrink_to_fit();

    bool empty() const;
    size_type size() const;
    void clear() noexcept;

    reference top();
    const_reference top() const;

    template<class ...Args> void emplace(Args&&... args);
    void push(const T& value);
    void push(T&& value);

    void pop();

    // deque compatibility
    reference back() { return top(); }
    const_reference back() const { return top(); }
    template <class... Args>
    void emplace_back(Args&&... args) { emplace(std::forward<Args>(args)...); }
    void push_back(const value_type& x) { push(x); }
    void push_back(value_type&& x) { push(std::move(x)); }
    void pop_back() { pop(); }

    class iterator {
        friend Stack;
    public:
        typedef typename Stack::difference_type difference_type;
        typedef typename Stack::value_type value_type;
        typedef T& reference;
        typedef T* pointer;
        typedef std::forward_iterator_tag iterator_category;

        iterator() = default;

        bool operator==(const iterator& rhs) const { return m_bucket == rhs.m_bucket && m_item == rhs.m_item; }
        bool operator!=(const iterator& rhs) const { return m_bucket != rhs.m_bucket || m_item != rhs.m_item; }

        iterator& operator++();
        iterator operator++(int) { auto v = *this; ++*this; return v; }

        reference operator*() const { return m_bucket->items[m_item]; }
        pointer operator->() const { return &m_bucket->items[m_item]; }

    private:
        explicit iterator(Bucket* head) : m_bucket{head} {}

        Bucket* m_bucket = nullptr;
        uint32_t m_item = 0;
    };

    class const_iterator {
        friend Stack;
    public:
        typedef typename Stack::difference_type difference_type;
        typedef typename Stack::value_type value_type;
        typedef const T& reference;
        typedef const T* pointer;
        typedef std::forward_iterator_tag iterator_category;

        const_iterator() = default;

        bool operator==(const const_iterator& rhs) const { return m_bucket == rhs.m_bucket && m_item == rhs.m_item; }
        bool operator!=(const const_iterator& rhs) const { return m_bucket != rhs.m_bucket || m_item != rhs.m_item; }

        const_iterator& operator++();
        const_iterator operator++(int) { auto v = *this; ++*this; return v; }

        reference operator*() const { return m_bucket->items[m_item]; }
        pointer operator->() const { return &m_bucket->items[m_item]; }

    private:
        explicit const_iterator(Bucket* head) : m_bucket{head} {}

        Bucket* m_bucket = nullptr;
        uint32_t m_item = 0;
    };

    iterator begin()                { return iterator{m_head}; }
    const_iterator begin() const    { return const_iterator{m_head}; }
    const_iterator cbegin() const   { return const_iterator{m_head}; }
    iterator end()                  { return {}; }
    const_iterator end() const      { return {}; }
    const_iterator cend() const     { return {}; }

    // dump current allocation info (the sizes and usage of buckets)
    template <class S> void alloc_info(S& stream);

private:
    struct Bucket {
        Bucket* next;       // ptr to next bucket or nullptr if this is the tail
        uint32_t capacity;  // number of reserved items
        uint32_t count;     // number of initialized items
        T items[0];         // flexible array member (GNU extension in C++)

        static Bucket* allocate(uint32_t capacity, uint32_t count);
        static void deallocate(Bucket* bucket);
        static void destroy(Bucket* bucket);  // deallocate chain of buckets
        static constexpr uint32_t project_capacity(uint32_t prev_cap);

        bool full() const { return count == capacity; }
        Bucket* prev(Bucket* head) const;
    };

    T& push_uninitialized();

    // handles to allocated storage:
    // - default ctor doesn't create any buckets
    // - only one bucket can be empty and it must be the tail
    // - popping the last item from the non-last bucket causes deallocation
    //   of the tail, effectively shrinking the container
    Bucket* m_head = nullptr;  // first bucket
    Bucket* m_tail = nullptr;  // last active bucket (the one containing top item)
};


// ----------------------------------------------------------------------------
// Implementation details


template<class T>
Stack<T>::Stack(const Stack& other)
    : m_head{Bucket::allocate(other.size(), other.size())}, m_tail{m_head}
{
    std::uninitialized_copy(other.cbegin(), other.cend(), m_head->items);
}


template<class T>
Stack<T>::Stack(Stack&& other) noexcept
    : m_head(other.m_head), m_tail(other.m_tail)
{
    other.m_head = nullptr;
    other.m_tail = nullptr;
}


template<class T>
Stack<T>& Stack<T>::operator=(const Stack& other)  // NOLINT
{
    if (m_head == other.m_head)
        return *this;  // self-assignment or empty-to-empty
    Bucket::destroy(m_head);
    m_head = Bucket::allocate(other.size(), other.size());
    m_tail = m_head;
    std::uninitialized_copy(other.cbegin(), other.cend(), m_head->items);
    return *this;
}


template<class T>
Stack<T>& Stack<T>::operator=(Stack&& other) noexcept
{
    m_head = other.m_head;
    m_tail = other.m_tail;
    other.m_head = nullptr;
    other.m_tail = nullptr;
    return *this;
}


template<class T>
bool Stack<T>::operator==(const Stack& other) const
{
    return size() == other.size() &&
        std::equal(cbegin(), cend(), other.cbegin());
}


template<class T>
void Stack<T>::swap(Stack& other) noexcept
{
    std::swap(m_head, other.m_head);
    std::swap(m_tail, other.m_tail);
}


template<class T>
auto Stack<T>::capacity() const noexcept -> size_type
{
    size_type res = 0;
    for (auto* b = m_head; b; b = b->next) {
        res += b->capacity;
    }
    return res;
}


template<class T>
void Stack<T>::reserve(Stack::size_type new_capacity)
{
    if (!m_head) {
        // no allocation yet
        assert(!m_tail);
        m_head = Bucket::allocate(new_capacity, 0);
        m_tail = m_head;
        return;
    }

    auto cap = capacity();
    if (cap >= new_capacity)
        // already satisfied
        return;

    assert(m_tail);
    if (m_tail->next) {
        // some space already reserved after tail
        assert(m_tail->count != 0);
        const auto alloc_cap = new_capacity - (cap - m_tail->next->capacity);
        Bucket::deallocate(m_tail->next);
        m_tail->next = Bucket::allocate(alloc_cap, 0);
        return;
    }

    if (m_tail->count == 0) {
        // tail bucket is last and empty, reallocate it
        assert(m_head == m_tail);
        auto alloc_cap = new_capacity - (cap - m_tail->capacity);
        Bucket::deallocate(m_tail);
        m_tail = Bucket::allocate(alloc_cap, 0);
        m_head = m_tail;
        return;
    }

    // add new bucket as reserve after tail
    auto alloc_cap = std::max(uint32_t(new_capacity - cap),
            Bucket::project_capacity(m_tail->capacity));
    m_tail->next = Bucket::allocate(alloc_cap, 0);
}


template<class T>
void Stack<T>::shrink_to_fit()
{
    auto cap = size();
    if (m_head && m_head == m_tail && cap == m_head->capacity)
        return;  // already at one bucket which fits exactly
    // prepare new bucket at tail
    m_tail = Bucket::allocate(cap, cap);
    std::uninitialized_copy(cbegin(), cend(), m_tail->items);
    // destroy old buckets
    Bucket::destroy(m_head);
    m_head = m_tail;
}


template<class T>
bool Stack<T>::empty() const
{
    return m_head ? m_head->count == 0 : true;
}


template<class T>
auto Stack<T>::size() const -> Stack::size_type
{
    size_type res = 0;
    for (auto* b = m_head; b && b->count != 0; b = b->next) {
        res += b->count;
    }
    return res;
}


template<class T>
void Stack<T>::clear() noexcept
{
    size_t total_cap = 0;
    for (auto* b = m_head; b; b = b->next) {
        std::destroy_n(b->items, b->count);
        b->count = 0;
        total_cap += b->capacity;
    }
    if (m_head && m_head->next) {
        // deallocate old chain, allocate single bucket with same total capacity
        Bucket::destroy(m_head);
        m_head = Bucket::allocate(total_cap, 0);
    }
    m_tail = m_head;
}


template<class T>
auto Stack<T>::top() -> reference
{
    assert(!empty());
    assert(m_tail);
    return m_tail->items[m_tail->count - 1];
}


template<class T>
auto Stack<T>::top() const -> const_reference
{
    assert(!empty());
    assert(m_tail);
    return m_tail->items[m_tail->count - 1];
}


template<class T>
template<class... Args>
void Stack<T>::emplace(Args&& ... args)
{
    T& item = push_uninitialized();
    ::new (&item) T(std::forward<Args>(args)...);
}


template<class T>
void Stack<T>::push(const T& value)
{
    T& item = push_uninitialized();
    ::new (&item) T(value);
}


template<class T>
void Stack<T>::push(T&& value)
{
    T& item = push_uninitialized();
    ::new (&item) T(std::move(value));
}


template<class T>
void Stack<T>::pop()
{
    assert(!empty());
    assert(m_tail);
    assert(m_tail->count != 0);
    std::destroy_at(&m_tail->items[--m_tail->count]);
    if (m_tail->count == 0) {
        // move tail back
        if (m_tail->next) {
            Bucket::deallocate(m_tail->next);
            m_tail->next = nullptr;
        }
        if (m_tail != m_head) {
            Bucket* b = m_tail->prev(m_head);
            b->next = m_tail;
            m_tail = b;
        }
    }
}


template<class T>
auto Stack<T>::iterator::operator++() -> iterator&
{
    ++m_item;
    if (m_item >= m_bucket->count) {
        m_bucket = m_bucket->next;
        m_item = 0;
    }
    return *this;
}


template<class T>
auto Stack<T>::const_iterator::operator++() -> const_iterator&
{
    ++m_item;
    if (m_item >= m_bucket->count) {
        m_bucket = m_bucket->next;
        m_item = 0;
    }
    return *this;
}


template <class T>
template<class S>
void Stack<T>::alloc_info(S& stream)
{
    for (auto* b = m_head; b; b = b->next) {
        stream << "cap " << b->capacity << " used " << b->count;
        if (b == m_head)
            stream << " [head]";
        if (b == m_tail)
            stream << " [tail]";
        stream << "\n";
    }
}


template<class T>
auto Stack<T>::Bucket::allocate(uint32_t capacity, uint32_t count) -> Stack::Bucket*
{
    const size_t size = sizeof(Bucket) + capacity * sizeof(T);
    char* buf = new char[size];
    return new(buf) Bucket {nullptr, capacity, count};
}


template<class T>
void Stack<T>::Bucket::deallocate(Stack::Bucket* bucket)
{
    std::destroy_n(bucket->items, bucket->count);
    delete[] reinterpret_cast<char*>(bucket);
}


template<class T>
auto Stack<T>::Bucket::prev(Bucket* head) const -> Bucket*
{
    while (head->next != this)
        head = head->next;
    return head;
}


template<class T>
T& Stack<T>::push_uninitialized()
{
    if (!m_tail) {
        // empty, no allocation yet
        m_head = Bucket::allocate(Bucket::project_capacity(0), 1);
        m_tail = m_head;
        return m_tail->items[0];
    }
    if (m_tail->full()) {
        // tail full, move to next bucket
        if (!m_tail->next) {
            m_tail->next = Bucket::allocate(Bucket::project_capacity(m_tail->capacity), 1);
            m_tail = m_tail->next;
        } else {
            m_tail = m_tail->next;
            assert(m_tail->count == 0);
            m_tail->count = 1;
        }
        return m_tail->items[0];
    }
    // push to tail
    return m_tail->items[m_tail->count++];
}


template<class T>
void Stack<T>::Bucket::destroy(Bucket* bucket)
{
    Bucket* nb;
    for (auto* b = bucket; b; b = nb) {
        nb = b->next;
        Bucket::deallocate(b);
    }
}


template<class T>
constexpr uint32_t Stack<T>::Bucket::project_capacity(uint32_t prev_cap)
{
    constexpr auto fstep = [](size_t cap_req) {
        return (cap_req * sizeof(T) - sizeof(Bucket)) / sizeof(T);
    };
    constexpr uint32_t step0 = fstep(16);
    constexpr uint32_t step1 = fstep(64);
    constexpr uint32_t step2 = fstep(256);
    constexpr uint32_t step3 = fstep(1024);
    constexpr uint32_t step4 = fstep(4096);
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


} // namespace xci::core

#endif // include guard
