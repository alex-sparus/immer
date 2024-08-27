//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>

#include <cereal/cereal.hpp>

#include <immer/table.hpp>

namespace cereal {

template <typename Archive,
          typename T,
          typename KF,
          typename H,
          typename E,
          typename MP,
          std::uint32_t B>
void CEREAL_LOAD_FUNCTION_NAME(Archive& ar, immer::table<T, KF, H, E, MP, B>& m)
{
    size_type size;
    ar(make_size_tag(size));

    for (auto i = size_type{}; i < size; ++i) {
        T x;
        ar(x);
        m = std::move(m).insert(std::move(x));
    }
    if (size != m.size())
        IMMER_THROW(std::runtime_error{"duplicate ids?"});
}

template <typename Archive,
          typename T,
          typename KF,
          typename H,
          typename E,
          typename MP,
          std::uint32_t B>
void CEREAL_SAVE_FUNCTION_NAME(Archive& ar,
                               const immer::table<T, KF, H, E, MP, B>& m)
{
    ar(make_size_tag(static_cast<size_type>(m.size())));
    for (auto&& v : m)
        ar(v);
}

} // namespace cereal
