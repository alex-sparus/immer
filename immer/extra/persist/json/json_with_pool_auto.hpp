#pragma once

#include <immer/extra/persist/json/json_immer.hpp>
#include <immer/extra/persist/json/json_with_pool.hpp>
#include <immer/extra/persist/json/persistable.hpp>

#include <immer/extra/persist/box/pool.hpp>
#include <immer/extra/persist/champ/traits.hpp>
#include <immer/extra/persist/rbts/traits.hpp>

#include <immer/extra/persist/common/type_traverse.hpp>

namespace immer::persist {

template <class T>
struct is_auto_ignored_type : boost::hana::false_
{};

template <class T>
struct is_auto_ignored_type<immer::map<node_id, values_save<T>>>
    : boost::hana::true_
{};

template <class T>
struct is_auto_ignored_type<immer::map<node_id, values_load<T>>>
    : boost::hana::true_
{};

template <>
struct is_auto_ignored_type<immer::map<node_id, rbts::inner_node>>
    : boost::hana::true_
{};

template <>
struct is_auto_ignored_type<immer::vector<node_id>> : boost::hana::true_
{};

template <>
struct is_auto_ignored_type<immer::vector<rbts::rbts_info>> : boost::hana::true_
{};

/**
 * This wrapper is used to load a given container via persistable.
 */
template <class Container>
struct persistable_loader_wrapper
{
    Container& value;

    template <class Archive>
    typename container_traits<Container>::container_id::rep_t
    save_minimal(const Archive&) const
    {
        throw std::logic_error{
            "Should never be called. persistable_loader_wrapper::save_minimal"};
    }

    template <class Archive>
    void load_minimal(
        const Archive& ar,
        const typename container_traits<Container>::container_id::rep_t&
            container_id)
    {
        persistable<Container> arch;
        immer::persist::load_minimal(ar, arch, container_id);
        value = std::move(arch).container;
    }
};

constexpr auto is_persistable = boost::hana::is_valid(
    [](auto&& obj) ->
    typename container_traits<std::decay_t<decltype(obj)>>::output_pool_t {});

constexpr auto is_auto_ignored = [](const auto& value) {
    return is_auto_ignored_type<std::decay_t<decltype(value)>>{};
};

/**
 * Make a function that operates conditionally on its single argument, based on
 * the given predicate. If the predicate is not satisfied, the function forwards
 * its argument unchanged.
 */
constexpr auto make_conditional_func = [](auto pred, auto func) {
    return [pred, func](auto&& value) -> decltype(auto) {
        return boost::hana::if_(pred(value), func, boost::hana::id)(
            std::forward<decltype(value)>(value));
    };
};

// We must not try to persist types that are actually the pool itself,
// for example, `immer::map<node_id, values_save<T>> leaves` etc.
constexpr auto exclude_internal_pool_types = [](auto wrap) {
    namespace hana = boost::hana;
    return make_conditional_func(hana::compose(hana::not_, is_auto_ignored),
                                 wrap);
};

constexpr auto to_persistable = [](const auto& x) {
    return persistable<std::decay_t<decltype(x)>>(x);
};

constexpr auto to_persistable_loader = [](auto& value) {
    using V = std::decay_t<decltype(value)>;
    return persistable_loader_wrapper<V>{value};
};

/**
 * This function will wrap a value in persistable if possible or will return a
 * reference to its argument.
 */
constexpr auto wrap_for_saving = exclude_internal_pool_types(
    make_conditional_func(is_persistable, to_persistable));

constexpr auto wrap_for_loading = exclude_internal_pool_types(
    make_conditional_func(is_persistable, to_persistable_loader));

/**
 * Generate a hana map of persistable members for the given type, recursively.
 * Example:
 * [(type_c<immer::map<K, V>>, "tracks")]
 */
auto get_pools_for_type(auto type)
{
    namespace hana     = boost::hana;
    auto all_types_map = util::get_inner_types(type);
    auto persistable =
        hana::filter(hana::to_tuple(all_types_map), [&](auto pair) {
            using T = typename decltype(+hana::first(pair))::type;
            return is_persistable(T{});
        });
    return hana::to_map(persistable);
}

template <typename T,
          class PoolsTypes,
          class WrapF = std::decay_t<decltype(wrap_for_saving)>>
auto to_json_with_auto_pool(const T& serializable,
                            const PoolsTypes& pools_types,
                            const WrapF& wrap = wrap_for_saving)
{
    // In the future, wrap function may ignore certain user-provided types that
    // should not be persisted.
    static_assert(
        std::is_same_v<decltype(wrap_for_saving(
                           std::declval<const std::string&>())),
                       const std::string&>,
        "wrap must return a reference when it's not wrapping the type");
    static_assert(
        std::is_same_v<decltype(wrap_for_saving(immer::vector<int>{})),
                       persistable<immer::vector<int>>>,
        "and a value when it's wrapping");

    auto os = std::ostringstream{};
    {
        auto pools  = detail::generate_output_pools(pools_types);
        using Pools = std::decay_t<decltype(pools)>;
        auto ar     = json_immer_output_archive<cereal::JSONOutputArchive,
                                            Pools,
                                            decltype(wrap)>{pools, wrap, os};
        // value0 because that's now cereal saves the unnamed object by default,
        // maybe change later.
        ar(cereal::make_nvp("value0", serializable));
    }
    return os.str();
}

// Same as to_json_with_auto_pool but we don't generate any JSON.
template <typename T,
          class PoolsTypes,
          class WrapF = std::decay_t<decltype(wrap_for_saving)>>
auto get_auto_pool(const T& serializable,
                   const PoolsTypes& pools_types,
                   const WrapF& wrap = wrap_for_saving)
{
    // In the future, wrap function may ignore certain user-provided types that
    // should not be persisted.
    static_assert(
        std::is_same_v<decltype(wrap_for_saving(
                           std::declval<const std::string&>())),
                       const std::string&>,
        "wrap must return a reference when it's not wrapping the type");
    static_assert(
        std::is_same_v<decltype(wrap_for_saving(immer::vector<int>{})),
                       persistable<immer::vector<int>>>,
        "and a value when it's wrapping");

    auto pools  = detail::generate_output_pools(pools_types);
    using Pools = std::decay_t<decltype(pools)>;

    {
        auto ar = json_immer_output_archive<detail::blackhole_output_archive,
                                            Pools,
                                            decltype(wrap)>{pools, wrap};
        // value0 because that's now cereal saves the unnamed object by default,
        // maybe change later.
        ar(cereal::make_nvp("value0", serializable));
        ar.finalize();
        pools = std::move(ar).get_output_pools();
    }
    return pools;
}

constexpr auto reload_pool_auto = [](auto wrap) {
    return [wrap](std::istream& is, auto pools, bool ignore_pool_exceptions) {
        using Pools                  = std::decay_t<decltype(pools)>;
        auto restore                 = immer::util::istream_snapshot{is};
        const auto original_pools    = pools;
        pools.ignore_pool_exceptions = ignore_pool_exceptions;
        auto ar = json_immer_input_archive<cereal::JSONInputArchive,
                                           Pools,
                                           decltype(wrap)>{
            std::move(pools), wrap, is};
        /**
         * NOTE: Critical to clear the pools before loading into it
         * again. I hit a bug when pools contained a vector and every
         * load would append to it, instead of replacing the contents.
         */
        pools = {};
        ar(CEREAL_NVP(pools));
        pools.merge_previous(original_pools);
        return pools;
    };
};

template <typename T, class PoolsTypes>
T from_json_with_auto_pool(std::istream& is, const PoolsTypes& pools_types)
{
    namespace hana      = boost::hana;
    constexpr auto wrap = wrap_for_loading;

    using Pools =
        std::decay_t<decltype(detail::generate_input_pools(pools_types))>;

    auto pools = load_pools<Pools>(is, reload_pool_auto(wrap));

    auto ar =
        json_immer_input_archive<cereal::JSONInputArchive,
                                 Pools,
                                 decltype(wrap)>{std::move(pools), wrap, is};
    // value0 because that's now cereal saves the unnamed object by default,
    // maybe change later.
    auto value0 = T{};
    ar(CEREAL_NVP(value0));
    return value0;
}

template <typename T, class PoolsTypes>
T from_json_with_auto_pool(const std::string& input,
                           const PoolsTypes& pools_types)
{
    auto is = std::istringstream{input};
    return from_json_with_auto_pool<T>(is, pools_types);
}

template <typename T,
          typename OldType,
          typename ConversionsMap,
          class PoolsTypes>
T from_json_with_auto_pool_with_conversion(std::istream& is,
                                           const ConversionsMap& map,
                                           const PoolsTypes& pools_types)
{
    constexpr auto wrap = wrap_for_loading;

    // Load the pools part for the old type
    using OldPools =
        std::decay_t<decltype(detail::generate_input_pools(pools_types))>;
    auto pools_old = load_pools<OldPools>(is, reload_pool_auto(wrap));

    auto pools  = pools_old.transform(map);
    using Pools = decltype(pools);

    auto ar =
        json_immer_input_archive<cereal::JSONInputArchive,
                                 Pools,
                                 decltype(wrap)>{std::move(pools), wrap, is};
    auto r = T{};
    ar(r);
    return r;
}

template <typename T,
          typename OldType,
          typename ConversionsMap,
          class PoolsTypes>
T from_json_with_auto_pool_with_conversion(const std::string& input,
                                           const ConversionsMap& map,
                                           const PoolsTypes& pools_types)
{
    auto is = std::istringstream{input};
    return from_json_with_auto_pool_with_conversion<T, OldType>(
        is, map, pools_types);
}

} // namespace immer::persist
