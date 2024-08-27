//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cereal/cereal.hpp>
#include <immer/array.hpp>
#include <immer/array_transient.hpp>
#include <type_traits>

// This code has mostly been adapted from <cereal/types/vector.hpp>
// We don't deal for now with data that could be potentially serialized
// directly in binary format.

namespace cereal {

template <typename Archive, typename T, typename MP>
void CEREAL_SAVE_FUNCTION_NAME(Archive& ar, const immer::array<T, MP>& array)
{
    ar(make_size_tag(static_cast<size_type>(array.size())));
    for (auto&& v : array)
        ar(v);
}

template <typename Archive, typename T, typename MP>
void CEREAL_LOAD_FUNCTION_NAME(Archive& ar, immer::array<T, MP>& array)
{
    size_type size{};
    ar(make_size_tag(size));

    if (!size)
        return;

    auto t = immer::array<T, MP>{}.transient();
    for (auto i = size_type{}; i < size; ++i) {
        T x;
        ar(x);
        t.push_back(std::move(x));
    }
    array = std::move(t).persistent();

    assert(size == array.size());
}

} // namespace cereal
