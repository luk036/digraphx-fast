/**
 * @file parametric.hpp
 * @brief Maximum parametric network problem solver for CSR graphs
 */

#pragma once

#include <algorithm>
#include <vector>

#include "neg_cycle.hpp"

namespace digraphx_fast {

    /**
     * @brief Maximum parametric network problem
     *
     * Maximize r such that:
     *   dist[v] - dist[u] <= distance(e, r)  for all edges e(u,v)
     *
     * Uses Howard's algorithm with warm-start — distances and
     * predecessor info persist across iterations for faster convergence.
     *
     * @tparam Graph CSR graph type
     * @tparam T Numeric type for parameter r
     * @tparam Fn1 (r, edge_idx) -> weight
     * @tparam Fn2 (cycle_edges, graph_ref) -> new_r
     */
    template <typename Graph, typename T, typename Fn1, typename Fn2>
    auto max_parametric(const Graph& gra, T& r_opt, Fn1&& distance, Fn2&& zero_cancel,
                        std::vector<T>& dist, size_t max_iters = 1000) -> std::vector<size_t> {
        using weight_t = typename Graph::weight_t;

        auto ncf = NegCycleFinder<Graph>(gra);
        std::vector<size_t> c_opt;
        std::vector<weight_t> weights(gra.num_edges);

        // First iteration: howard (fresh start)
        // Subsequent iterations: howard_warm (reuses predecessor info)

        for (auto niter = 0U; niter != max_iters; ++niter) {
            // Compute edge weights for current r
            for (size_t e = 0; e < gra.num_edges; ++e) {
                weights[e] = static_cast<weight_t>(distance(r_opt, e));
            }

            bool found;
            if (niter == 0) {
                found = ncf.howard(
                    dist, weights,
                    [&](const auto& cycle) {
                        auto r_min = zero_cancel(cycle, gra);
                        if (r_min < r_opt) {
                            c_opt = cycle;
                            r_opt = r_min;
                            // Update distances along cycle
                            for (auto e : cycle) {
                                auto u = 0u;
                                // Find source node for this edge
                                // Linear scan over offsets — could be optimized
                                // with a reverse map but fine for typical cycles
                                for (size_t i = 0; i < gra.num_nodes; ++i) {
                                    if (e >= gra.offsets[i] && e < gra.offsets[i + 1]) {
                                        u = static_cast<uint32_t>(i);
                                        break;
                                    }
                                }
                                auto v = gra.targets[e];
                                dist[u] = dist[v] - static_cast<T>(weights[e]);
                            }
                        }
                    },
                    1);
            } else {
                found = ncf.howard_warm(
                    dist, weights,
                    [&](const auto& cycle) {
                        auto r_min = zero_cancel(cycle, gra);
                        if (r_min < r_opt) {
                            c_opt = cycle;
                            r_opt = r_min;
                            for (auto e : cycle) {
                                auto u = 0u;
                                for (size_t i = 0; i < gra.num_nodes; ++i) {
                                    if (e >= gra.offsets[i] && e < gra.offsets[i + 1]) {
                                        u = static_cast<uint32_t>(i);
                                        break;
                                    }
                                }
                                auto v = gra.targets[e];
                                dist[u] = dist[v] - static_cast<T>(weights[e]);
                            }
                        }
                    },
                    1);
            }

            if (!found) break;
        }

        return c_opt;
    }

}  // namespace digraphx_fast
