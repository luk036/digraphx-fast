/**
 * @file neg_cycle.hpp
 * @brief Howard's policy iteration for negative cycle detection (CSR-optimized)
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <vector>

#include "csr_graph.hpp"

namespace digraphx_fast {

/**
 * @brief Howard's policy iteration for negative cycle detection
 *
 * Uses pre-allocated per-node arrays (no hash maps), epoch-based
 * visited tracking (no clear()), and direct CSR iteration for
 * cache-friendly memory access.
 *
 * Supports max_cycles to stop early and warm-start for reuse
 * across parametric iterations.
 */
template <typename Graph> class NegCycleFinder {
    using node_t = typename Graph::node_t;
    using weight_t = typename Graph::weight_t;

    static_assert(std::is_same_v<node_t, uint32_t>,
                  "NegCycleFinder requires CSRGraph with uint32_t node_t");

    const Graph& _graph;

    // Pre-allocated per-node arrays
    std::vector<node_t> _pred_node;  // node_t(-1) = sentinel (no predecessor)
    std::vector<size_t> _pred_edge;  // edge index in CSR arrays
    std::vector<node_t> _visited;    // tracks originating start vertex, node_t(-1) = unvisited

    // Scratch buffer for cycle extraction (reused across calls)
    std::vector<size_t> _cycle;

    /**
      * @brief One relaxation pass over all CSR edges
      *
      * @f[
      *     d_v \gets \min(d_v,\; d_u + w(u,v)), \quad \forall (u,v) \in E
      * @f]
      *
      * @dot
      *   digraph relax {
      *     rankdir=LR; bgcolor="transparent";
      *     node [shape=box, style=filled, fillcolor="#d4e6f1"];
      *     scan [label="For each\nu in V", fillcolor="#a9cce3"];
      *     edges [label="For each edge\n(u,v) in E"];
      *     relax [label="d[v] = min(d[v],\nd[u] + w)", shape=diamond, fillcolor="#f9e79f"];
      *     changed [label="Record\npredecessor\nu -> v", fillcolor="#d5f5e3"];
      *     scan -> edges;
      *     edges -> relax;
      *     relax -> changed [label="improved", color="#27ae60"];
      *     relax -> edges [style=dashed, label="skip", color="#888"];
      *   }
      * @enddot
      *
      * Linear scan of targets/weights arrays — cache-friendly.
      * Updates dist[v] = min(dist[v], dist[u] + w) and records predecessor.
      */
    auto _relax(std::vector<weight_t>& dist, const std::vector<weight_t>& weights) -> bool {
        bool changed = false;
        const auto& offsets = _graph.offsets;
        const auto& targets = _graph.targets;
        const auto V = _graph.num_nodes;

        for (node_t u = 0; u < V; ++u) {
            auto du = dist[u];
            for (size_t e = offsets[u]; e < offsets[u + 1]; ++e) {
                auto v = targets[e];
                auto d = du + weights[e];
                if (dist[v] > d) {
                    dist[v] = d;
                    _pred_node[v] = u;
                    _pred_edge[v] = e;
                    changed = true;
                }
            }
        }
        return changed;
    }

    /**
     * @brief Find a cycle in the predecessor graph
     *
     * Uses _visited array to track which start-vertex set each entry.
     * node_t(-1) sentinel = unvisited. Stores the originating start vertex
     * to avoid false cycles across different search paths.
     * O(V) time, no heap allocation.
     */
    auto _find_cycle() -> std::optional<node_t> {
        const auto V = _graph.num_nodes;
        std::fill(_visited.begin(), _visited.end(), node_t(-1));
        for (node_t v = 0; v < V; ++v) {
            if (_visited[v] != node_t(-1))
                continue;
            auto u = v;
            _visited[u] = v;
            while (_pred_node[u] != node_t(-1)) {
                u = _pred_node[u];
                if (_visited[u] == v)
                    return u;
                if (_visited[u] != node_t(-1))
                    break;
                _visited[u] = v;
            }
        }
        return {};
    }

    /**
     * @brief Verify that the cycle starting at handle is indeed negative
     */
    auto _is_negative(node_t handle, const std::vector<weight_t>& weights,
                      const std::vector<weight_t>& dist) const -> bool {
        auto v = handle;
        do {
            auto u = _pred_node[v];
            auto e = _pred_edge[v];
            if (dist[v] > dist[u] + weights[e])
                return true;
            v = u;
        } while (v != handle);
        return false;
    }

    /**
     * @brief Extract cycle edge indices starting from handle
     *
     * Returns reference to internal buffer (avoids allocation per call).
     */
    auto _cycle_list(node_t handle) -> const std::vector<size_t>& {
        _cycle.clear();
        auto v = handle;
        do {
            _cycle.push_back(_pred_edge[v]);
            v = _pred_node[v];
        } while (v != handle);
        return _cycle;
    }

  public:
    explicit NegCycleFinder(const Graph& graph)
        : _graph(graph), _pred_node(graph.num_nodes, node_t(-1)),
          _pred_edge(graph.num_nodes, 0), _visited(graph.num_nodes, 0) {}

    /**
     * @brief Howard's algorithm — find negative cycles with callback
     *
     * @f[
     *     \text{relax } d_v \gets \min(d_v, d_u + w_{uv}) \to \text{ find cycle } \to \text{ verify negativity}
     * @f]
     *
     * @dot
     *   digraph howard_algo {
     *     rankdir=TB; bgcolor="transparent";
     *     node [shape=box, style=filled, fillcolor="#d4e6f1"];
     *     reset [label="Reset\npredecessor map", fillcolor="#a9cce3"];
     *     relax [label="Relax all edges\n(one pass)"];
     *     check_relax [label="Changed?", shape=diamond, fillcolor="#f9e79f"];
     *     find_cycle [label="Find cycle in\npredecessor graph"];
     *     check_neg [label="Cycle\nnegative?", shape=diamond, fillcolor="#f9e79f"];
     *     yield [label="Yield cycle\nvia callback", fillcolor="#d5f5e3"];
     *     done [label="Return\nfound > 0", fillcolor="#7fb3d8"];
     *     reset -> relax;
     *     relax -> check_relax;
     *     check_relax -> find_cycle [label="Yes", color="#27ae60"];
     *     check_relax -> done [label="No", color="#e74c3c"];
     *     find_cycle -> check_neg;
     *     check_neg -> yield [label="Yes", color="#27ae60"];
     *     check_neg -> relax [label="No", style=dashed, color="#888"];
     *     yield -> relax [style=dashed, label="more?", color="#888"];
     *   }
     * @enddot
     *
     * @tparam Fn Callable(const std::vector&lt;size_t&gt;&amp;) — receives cycle edge indices
     * @param[in,out] dist Distance vector (size V), updated during relaxation
     * @param[in] weights Edge weight vector (size E)
     * @param[out] yield_cycle Callback invoked for each negative cycle found
     * @param[in] max_cycles Stop after finding this many cycles (default: 1)
     * @return true if at least one negative cycle was found
     */
    template <typename Fn>
    auto howard(std::vector<weight_t>& dist, const std::vector<weight_t>& weights,
                Fn&& yield_cycle, size_t max_cycles = 1) -> bool {
        // Reset predecessor map
        std::fill(_pred_node.begin(), _pred_node.end(), node_t(-1));

        size_t found = 0;
        while (found < max_cycles) {
            if (!_relax(dist, weights))
                break;
            auto handle = _find_cycle();
            if (handle && _is_negative(*handle, weights, dist)) {
                yield_cycle(_cycle_list(*handle));
                ++found;
            }
        }
        return found > 0;
    }

    /**
     * @brief Warm-start Howard's algorithm
     *
     * @f[
     *     \pi^{(0)}(v) \gets \pi^{(\text{prev})}(v), \quad \text{reuse predecessor policy across parametric iterations}
     * @f]
     *
     * @dot
     *   digraph howard_warm {
     *     rankdir=TB; bgcolor="transparent";
     *     node [shape=box, style=filled, fillcolor="#d4e6f1"];
     *     keep [label="Keep predecessor\nmap from prev call", fillcolor="#a9cce3"];
     *     relax [label="Relax all edges\n(one pass)"];
     *     check_relax [label="Changed?", shape=diamond, fillcolor="#f9e79f"];
     *     find_cycle [label="Find cycle"];
     *     check_neg [label="Negative?", shape=diamond, fillcolor="#f9e79f"];
     *     yield [label="Yield cycle", fillcolor="#d5f5e3"];
     *     done [label="Return\nfound > 0", fillcolor="#7fb3d8"];
     *     keep -> relax;
     *     relax -> check_relax;
     *     check_relax -> find_cycle [label="Yes", color="#27ae60"];
     *     check_relax -> done [label="No", color="#e74c3c"];
     *     find_cycle -> check_neg;
     *     check_neg -> yield [label="Yes", color="#27ae60"];
     *     check_neg -> relax [label="No", style=dashed, color="#888"];
     *     yield -> relax [style=dashed, label="more?", color="#888"];
     *   }
     * @enddot
     *
     * Unlike howard(), this does NOT clear the predecessor map,
     * allowing reuse from a previous call (e.g., in parametric search
     * where distances change only slightly).
     */
    template <typename Fn>
    auto howard_warm(std::vector<weight_t>& dist, const std::vector<weight_t>& weights,
                     Fn&& yield_cycle, size_t max_cycles = 1) -> bool {
        size_t found = 0;
        while (found < max_cycles) {
            if (!_relax(dist, weights))
                break;
            auto handle = _find_cycle();
            if (handle && _is_negative(*handle, weights, dist)) {
                yield_cycle(_cycle_list(*handle));
                ++found;
            }
        }
        return found > 0;
    }
};

} // namespace digraphx_fast
