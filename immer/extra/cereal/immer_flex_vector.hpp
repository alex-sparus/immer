//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cereal/cereal.hpp>
#include <immer/flex_vector.hpp>
#include <type_traits>

// This code has mostly been adapted from <cereal/types/vector.hpp>
// We don't deal for now with data that could be potentially serialized
// directly in binary format.

namespace cereal {

template <typename Archive,
          typename T,
          typename MP,
          std::uint32_t B,
          std::uint32_t BL>
void CEREAL_SAVE_FUNCTION_NAME(
    Archive& ar, const immer::flex_vector<T, MP, B, BL>& flex_vector)
{
    ar(make_size_tag(static_cast<size_type>(flex_vector.size())));
    for (auto&& v : flex_vector)
        ar(v);
}

template <typename Archive,
          typename T,
          typename MP,
          std::uint32_t B,
          std::uint32_t BL>
void CEREAL_LOAD_FUNCTION_NAME(Archive& ar,
                               immer::flex_vector<T, MP, B, BL>& flex_vector)
{
    size_type size;
    ar(make_size_tag(size));

    for (auto i = size_type{}; i < size; ++i) {
        T x;
        ar(x);
        flex_vector = std::move(flex_vector).push_back(std::move(x));
    }
}

} // namespace cereal
