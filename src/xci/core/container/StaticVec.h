// StaticVec.h created on 2023-09-25 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_STATIC_VEC_H
#define XCI_CORE_STATIC_VEC_H

#include <memory>
#include <span>
#include <algorithm>

namespace xci::core {


/// Like std::vector, but with "static" size (no insert, remove etc.)
/// Does not need to distinguish between capacity and size - it's always
/// size == capacity. This saves one size_t field, making the container class
/// one third smaller.
template <class T>
class StaticVec {
public:
    using value_type = T;
    using const_iterator = const T*;
    using iterator = T*;

    StaticVec() = default;
    explicit StaticVec(size_t size)
            : m_vec(std::make_unique<T[]>(size)), m_size(size) {}
    StaticVec(std::initializer_list<T> r);
    StaticVec(std::span<const T> r);

    StaticVec(const StaticVec& r);
    StaticVec(StaticVec&& r) = default;

    StaticVec& operator=(const StaticVec& r);
    StaticVec& operator=(StaticVec&& r) = default;

    friend bool operator==(const StaticVec& a, const StaticVec& b) {
        return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
    }

    void resize(size_t new_size);

    bool empty() const { return m_size == 0; }
    size_t size() const { return m_size; }

    T& front() { return *m_vec.get(); }
    const T& front() const { return *m_vec.get(); }
    T& operator[](size_t i) { return m_vec[i]; }
    const T& operator[](size_t i) const { return m_vec[i]; }

    const_iterator begin() const { return m_vec.get(); }
    const_iterator end() const { return &m_vec[m_size]; }
    iterator begin() { return m_vec.get(); }
    iterator end() { return &m_vec[m_size]; }

    operator std::span<const T>() const { return {begin(), end()}; }

private:
    std::unique_ptr<T[]> m_vec;
    size_t m_size = 0;
};


template <class T>
std::span<const T> to_span(const std::vector<T>& v) { return {v.data(), v.size()}; }


// ----------------------------------------------------------------------------
// Implementation details


template<class T>
StaticVec<T>::StaticVec(std::initializer_list<T> r)
            : m_vec(std::make_unique<T[]>(r.size())), m_size(r.size())
{
    std::move(r.begin(), r.end(), m_vec.get());
}


template<class T>
StaticVec<T>::StaticVec(std::span<const T> r)
        : m_vec(std::make_unique<T[]>(r.size())), m_size(r.size())
{
    std::copy(r.begin(), r.end(), m_vec.get());
}


template<class T>
StaticVec<T>::StaticVec(const StaticVec& r)
    : m_vec(std::make_unique<T[]>(r.size())), m_size(r.size())
{
    std::copy(r.begin(), r.end(), m_vec.get());
}

template<class T>
auto StaticVec<T>::operator=(const StaticVec& r) -> StaticVec& {
    resize(r.size());
    std::copy(r.begin(), r.end(), m_vec.get());
    return *this;
}


template<class T>
void StaticVec<T>::resize(size_t new_size) {
    m_vec = std::make_unique<T[]>(new_size);
    m_size = new_size;
}


} // namespace xci::core

#endif  // include guard
