/// JonssonicDSP - A Modular Realtime C++ Audio DSP Library
/// Routing types for filter output selection
/// SPDX-License-Identifier: MIT

#pragma once

namespace jnsc {

/// Routing enum for filter output selection
enum class Routing { Series, Parallel };

/// Generalized wrapper for routing types
template <typename Filter, Routing RoutingType>
struct RoutingWrapper;

/// Specialization for any filter engine with RoutingType
template <template <typename, typename, typename, Routing> class Filter,
          typename T,
          typename Topology,
          typename Design,
          Routing CurrentRouting>
struct RoutingWrapper<Filter<T, Topology, Design, CurrentRouting>, Routing::Series> {
    using Type = Filter<T, Topology, Design, Routing::Series>;
};

template <template <typename, typename, typename, Routing> class Filter,
          typename T,
          typename Topology,
          typename Design,
          Routing CurrentRouting>
struct RoutingWrapper<Filter<T, Topology, Design, CurrentRouting>, Routing::Parallel> {
    using Type = Filter<T, Topology, Design, Routing::Parallel>;
};

/// Type aliases for Series and Parallel routing
template <typename Filter>
using Series = typename RoutingWrapper<Filter, Routing::Series>::Type;

template <typename Filter>
using Parallel = typename RoutingWrapper<Filter, Routing::Parallel>::Type;

} // namespace jnsc