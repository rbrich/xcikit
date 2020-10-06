// FlatSet.h created on 2020-10-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_CONTAINER_FLATSET_H
#define XCI_CORE_CONTAINER_FLATSET_H

#include <memory>
#include <algorithm>

namespace xci::core {


template <typename T>
class FlatSet {
public:
    template<typename... Args>
    void emplace(Args&& ...args) {
        m_elems.emplace_back(std::forward<Args>(args)...);
        std::sort(m_elems.begin(), m_elems.end());
        m_elems.erase(std::unique(m_elems.begin(), m_elems.end()),
                m_elems.end());
    }
    bool contains(const T& elem) const {
        return std::binary_search(m_elems.begin(), m_elems.end(), elem);
    }
private:
    std::vector<T> m_elems;
};


} // namespace xci::core

#endif // include guard
