#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <vector>
#include <queue>
#include <limits>

namespace py = pybind11;


struct CSRGraph {
    int n;
    const int* indptr;   // pointer to row pointer array
    const int* indices;  // pointer to column indices
    const double* weights; // pointer to edge weights
};

const double INF = std::numeric_limits<double>::infinity();

/* ---------------- Single-source Dijkstra ---------------- */

void single_source_dijkstra(
    const CSRGraph& g,
    int source,
    double* dist,
    int* pred,
    double cutoff
) {

    using Item = std::pair<double, int>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> heap;

    dist[source] = 0.0;
    heap.emplace(0.0, source);

    while (!heap.empty()) {
        auto [d, u] = heap.top();
        heap.pop();

        if (d > dist[u]) continue;
        if (d > cutoff) break;
        // check neighbors
        for (int ei = g.indptr[u]; ei < g.indptr[u + 1]; ++ei) {
            int v = g.indices[ei];
            double nd = d + g.weights[ei];

            if (nd < dist[v] && nd <= cutoff) {
                dist[v] = nd;
                pred[v] = u;
                heap.emplace(nd, v);
            }
        }
    }
}

/* ---------------- Python-exposed function ---------------- */

std::tuple<py::array_t<double>, py::array_t<int>>
multi_source_dijkstra(
    py::array_t<int> indptr,
    py::array_t<int> indices,
    py::array_t<double> weights,
    py::array_t<int> sources,
    double cutoff = std::numeric_limits<double>::infinity()
) {
    // ---- Validate shapes ----
    // if (indices.size() != weights.size()) {
    //     throw std::runtime_error("indices and weights must have same length");
    // }

    int num_nodes = indptr.size() - 1;
    int num_sources = sources.size();

    // ---- Build graph (once) ----
    CSRGraph g;
    g.n = num_nodes;
    g.indptr = indptr.data();
    g.indices = indices.data();
    g.weights = weights.data();

    const int* src_ptr = sources.data();

    // ---- Allocate outputs ----
    py::array_t<double> distances_out({num_sources, num_nodes});
    py::array_t<int> predecessors_out({num_sources, num_nodes});
    auto distances_ptr = distances_out.mutable_data();
    auto predecessors_ptr = predecessors_out.mutable_data();
    for (int i = 0; i < num_sources * num_nodes; ++i) {
        distances_ptr[i] = INF;
        predecessors_ptr[i] = -9999;
    }

   

    #pragma omp parallel for schedule(dynamic)
    for (int si = 0; si < num_sources; ++si) {
        double *dist_row = distances_ptr + si * num_nodes;
        int *pred_row = predecessors_ptr + si * num_nodes;
        single_source_dijkstra(
            g,
            src_ptr[si],
            dist_row,
            pred_row,
            cutoff
        );
    }

    return {distances_out, predecessors_out};
}

/* ---------------- pybind11 module ---------------- */

PYBIND11_MODULE(fast_dijkstra, m) {
    m.doc() = "Fast CSR-based multi-source Dijkstra";

    m.def("dijkstra", &multi_source_dijkstra,
        py::arg("indptr"),
        py::arg("indices"),
        py::arg("weights"),
        py::arg("sources"),
        py::arg("cutoff") = INF
    );
}
