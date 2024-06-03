#pragma once

namespace immer::persist {

/**
 * Define these traits to connect a type (vector_one<T>) to its pool
 * (output_pool<T>).
 */
template <class T>
struct container_traits
{};

/**
 * User defines these traits to control pools persistence.
 */
template <class T>
struct persist_traits;

} // namespace immer::persist
