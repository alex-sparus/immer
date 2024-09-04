#pragma once

#include <immer/box.hpp>
#include <immer/flex_vector.hpp>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include <immer/table.hpp>
#include <immer/vector.hpp>

#include <boost/hana.hpp>

#include <variant>

namespace immer::persist::util {

namespace detail {

namespace hana = boost::hana;

template <class T>
struct single_type_t
{
    static auto apply()
    {
        return hana::make_tuple(
            hana::make_pair(hana::type_c<T>, BOOST_HANA_STRING("")));
    }
};

template <class T, class = void>
struct get_inner_types_t : single_type_t<T>
{};

template <class T>
struct get_inner_types_t<T, std::enable_if_t<hana::Struct<T>::value>>
{
    static auto apply()
    {
        auto value = T{};
        return hana::transform(hana::keys(value), [&](auto key) {
            const auto& member = hana::at_key(value, key);
            return hana::make_pair(hana::typeid_(member), key);
        });
    }
};

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
struct get_inner_types_t<immer::vector<T, MemoryPolicy, B, BL>>
    : single_type_t<T>
{};

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
struct get_inner_types_t<immer::flex_vector<T, MemoryPolicy, B, BL>>
    : single_type_t<T>
{};

template <typename T, typename MemoryPolicy>
struct get_inner_types_t<immer::box<T, MemoryPolicy>> : single_type_t<T>
{};

template <typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
struct get_inner_types_t<immer::set<T, Hash, Equal, MemoryPolicy, B>>
    : single_type_t<T>
{};

template <typename K,
          typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
struct get_inner_types_t<immer::map<K, T, Hash, Equal, MemoryPolicy, B>>
{
    static auto apply()
    {
        return hana::concat(single_type_t<K>::apply(),
                            single_type_t<T>::apply());
    }
};

template <typename T,
          typename KeyFn,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
struct get_inner_types_t<immer::table<T, KeyFn, Hash, Equal, MemoryPolicy, B>>
    : single_type_t<T>
{};

template <class... Types>
struct get_inner_types_t<std::variant<Types...>>
{
    static auto apply()
    {
        return hana::make_tuple(
            hana::make_pair(hana::type_c<Types>, BOOST_HANA_STRING(""))...);
    }
};

constexpr auto insert_conditionally = [](auto map, auto pair) {
    namespace hana = hana;
    using contains_t =
        decltype(hana::is_just(hana::find(map, hana::first(pair))));
    if constexpr (contains_t::value) {
        const auto empty_string = BOOST_HANA_STRING("");
        using is_empty_t        = decltype(hana::second(pair) == empty_string);
        if constexpr (is_empty_t::value) {
            // Do not replace with empty string
            return map;
        } else {
            return hana::insert(map, pair);
        }
    } else {
        return hana::insert(map, pair);
    }
};

inline auto get_inner_types_map_with_empty_strings(const auto& type)
{
    namespace hana = boost::hana;

    // Returns a tuple of pairs
    constexpr auto get_for_one_type = [](auto type) {
        using T = typename decltype(type)::type;
        return detail::get_inner_types_t<T>::apply();
    };

    constexpr auto process_queue = [get_for_one_type](const auto& result,
                                                      const auto& queue) {
        if constexpr (decltype(hana::is_empty(queue))::value) {
            return hana::make_tuple(result, queue);
        } else {
            const auto& to_expand = hana::front(queue);
            const auto new_queue  = hana::drop_front(queue);
            const auto new_state  = hana::fold_left(
                get_for_one_type(to_expand),
                hana::make_tuple(result, new_queue),
                [](auto state, auto pair) {
                    auto result   = hana::at_c<0>(state);
                    auto queue    = hana::at_c<1>(state);
                    auto new_type = hana::first(pair);
                    auto new_result =
                        detail::insert_conditionally(result, pair);
                    if constexpr (hana::contains(result, new_type)) {
                        // New type has already been seen before
                        // Queue is not changing
                        return hana::make_tuple(new_result, queue);
                    } else {
                        // First time we see this type, add it to the queue
                        return hana::make_tuple(new_result,
                                                hana::append(queue, new_type));
                    }
                });
            return new_state;
        }
    };

    constexpr auto can_expand = [](const auto& result, const auto& queue) {
        return !hana::is_empty(queue);
    };

    const auto initial_state =
        hana::make_tuple(hana::make_map(), hana::make_tuple(type));

    auto expanded_state = hana::while_(
        hana::fuse(can_expand), initial_state, hana::fuse(process_queue));

    return hana::at_c<0>(expanded_state);
}

} // namespace detail

/**
 * Generate a map (type, member_name) for all members of a given type,
 * recursively.
 */
inline auto get_inner_types_map(const auto& type)
{
    namespace hana = boost::hana;

    auto with_empty_strings =
        detail::get_inner_types_map_with_empty_strings(type);

    // Throw away types we don't know names for
    const auto empty_string = BOOST_HANA_STRING("");
    auto result             = hana::filter(hana::to_tuple(with_empty_strings),
                               [empty_string](auto pair) {
                                   return hana::second(pair) != empty_string;
                               });
    return hana::to_map(result);
}

inline auto get_inner_types(const auto& type)
{
    return boost::hana::keys(
        detail::get_inner_types_map_with_empty_strings(type));
}

} // namespace immer::persist::util
