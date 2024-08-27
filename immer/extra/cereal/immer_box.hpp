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
#include <type_traits>

namespace cereal {

template <typename Archive, typename T, typename MP>
void CEREAL_LOAD_FUNCTION_NAME(Archive& ar, immer::box<T, MP>& b)
{
    T x;
    ar(x);
    b = std::move(b).update([&](auto&& v) { return std::move(x); });
}

template <typename Archive, typename T, typename MP>
void CEREAL_SAVE_FUNCTION_NAME(Archive& ar, const immer::box<T, MP>& b)
{
    ar(b.get());
}

} // namespace cereal
