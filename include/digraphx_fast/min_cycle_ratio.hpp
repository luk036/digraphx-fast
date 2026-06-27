/**
 * @file min_cycle_ratio.hpp
 * @brief Minimum cost-to-time cycle ratio solver for CSR graphs
 */

#pragma once

#include <vector>

#include "parametric.hpp"

namespace digraphx_fast {

/**
 * @brief Minimum cost-to-time cycle ratio
 *
 * Find the cycle with minimum ratio:
 * @f[
 *     r^* = \min_{C \subseteq G} \frac{\sum_{e\in C} \mathrm{cost}(e)}{\sum_{e\in C} \mathrm{time}(e)}
 * @f]
 *
 * Uses parametric search with Howard's algorithm.
 *
 * @dot
 *   digraph mcr_fast {
 *     rankdir=LR; bgcolor="transparent";
 *     node [shape=box, style=filled, fillcolor="#d4e6f1"];
 *     r0 [label="Initial r", fillcolor="#a9cce3"];
 *     w_func [label="calc_weight =\ncost - r * time"];
 *     r_func [label="calc_ratio =\ntotal_cost /\ntotal_time"];
 *     param [label="max_parametric\nsearch", fillcolor="#f9e79f"];
 *     cycle [label="Optimal\ncycle", fillcolor="#7fb3d8"];
 *     r0 -> w_func;
 *     w_func -> param;
 *     r_func -> param;
 *     param -> cycle;
 *   }
 * @enddot
 *
 * @tparam Graph CSR graph type
 * @tparam T Numeric type (typically double or Fraction)
 * @tparam Fn1 (edge_idx) -> cost
 * @tparam Fn2 (edge_idx) -> time
 */
template <typename Graph, typename T, typename Fn1, typename Fn2>
auto min_cycle_ratio(const Graph& gra, T& r0, Fn1&& get_cost, Fn2&& get_time,
                     std::vector<T>& dist, size_t max_iters = 1000) -> std::vector<size_t> {
    auto calc_weight = [&](const T& r, size_t e) -> T {
        return static_cast<T>(get_cost(e)) - r * static_cast<T>(get_time(e));
    };

    auto calc_ratio = [&](const std::vector<size_t>& cycle, const Graph& g) -> T {
        T total_cost{};
        T total_time{};
        for (auto e : cycle) {
            total_cost += static_cast<T>(get_cost(e));
            total_time += static_cast<T>(get_time(e));
        }
        return total_cost / total_time;
    };

    return max_parametric(gra, r0, std::move(calc_weight), std::move(calc_ratio), dist,
                          max_iters);
}

}  // namespace digraphx_fast
