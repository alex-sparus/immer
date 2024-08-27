//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cereal/cereal.hpp>
#include <immer/box.hpp>
#include <immer/vector.hpp>
#include <type_traits>

// This code has mostly been adapted from <cereal/types/vector.hpp>
// We don't deal for now with data that could be potentially serialized
// directly in binary format.

namespace cereal {

template <typename Archive,
          typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
void CEREAL_LOAD_FUNCTION_NAME(Archive& ar,
                               immer::vector<T, MemoryPolicy, B, BL>& vector)
{
    size_type size;
    ar(make_size_tag(size));

    for (auto i = size_type{}; i < size; ++i) {
        T x;
        ar(x);
        vector = std::move(vector).push_back(std::move(x));
    }
}

template <typename Archive,
          typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
void CEREAL_SAVE_FUNCTION_NAME(
    Archive& ar, const immer::vector<T, MemoryPolicy, B, BL>& vector)
{
    ar(make_size_tag(static_cast<size_type>(vector.size())));
    for (auto&& v : vector)
        ar(v);
}

template <typename Archive,
          typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
void CEREAL_SAVE_FUNCTION_NAME(
    Archive& ar,
    const immer::vector<immer::box<T, MemoryPolicy>, MemoryPolicy, B, BL>&
        vector)
{
    ar(make_size_tag(static_cast<size_type>(vector.size())));
    for (auto&& v : vector)
        ar(*v);
}

template <typename Archive,
          typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
void CEREAL_LOAD_FUNCTION_NAME(
    Archive& ar,
    immer::vector<immer::box<T, MemoryPolicy>, MemoryPolicy, B, BL>& vector)
{
    size_type size;
    ar(make_size_tag(size));

    for (auto i = size_type{}; i < size; ++i) {
        T x;
        ar(x);
        vector = std::move(vector).push_back(std::move(x));
    }
}

} // namespace cereal
