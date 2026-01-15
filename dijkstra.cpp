#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <vector>
#include <queue>
#include <limits>
#include <omp.h>

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
        auto [current_dist, current_node] = heap.top();
        heap.pop();

        if (current_dist > dist[current_node]) continue;
        // check neighbors
        for (int j = g.indptr[current_node]; j < g.indptr[current_node + 1]; ++j) {
            int next_node = g.indices[j];
            double next_dist = current_dist + g.weights[j];
            if (next_dist <= cutoff){
                if (next_dist < dist[next_node]) {
                    dist[next_node] = next_dist;
                    pred[next_node] = current_node;
                    heap.emplace(next_dist, next_node);
                }
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
    double cutoff = std::numeric_limits<double>::infinity(),
    int num_threads = -1
) {

    if (num_threads > 0)
        omp_set_num_threads(num_threads);

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
   

    #pragma omp parallel for schedule(dynamic)
    for (int si = 0; si < num_sources; ++si) {
        double *dist_row = distances_ptr + si * num_nodes;
        int *pred_row = predecessors_ptr + si * num_nodes;

        for (int i = 0; i < num_nodes; ++i) {
            dist_row[i] = INF;
            pred_row[i] = -9999;
        }


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
    m.doc() = "Fast multi-source Dijkstra";

    m.def("dijkstra", &multi_source_dijkstra,
        py::arg("indptr"),
        py::arg("indices"),
        py::arg("weights"),
        py::arg("sources"),
        py::arg("cutoff") = INF,
        py::arg("num_threads") = -1,
        R"pbdoc(
        Compute shortest paths using Dijkstra's algorithm. on a csr_graph

        Parameters
        ----------
        indptr : List[int]
        indices : List[int]
        weights : Sequence[float]
        sources : List[int]
        cutoff: OPTIONAL  float (default = np.inf)
        num_threads: OPTIONAL int (default = -1)
            -1: maximum allowed threads

        Returns
        -------
        distances : numpy.ndarray
            Array of shape (num_sources, num_nodes) with shortest distances
        predecessors : numpy.ndarray
            Array of shape (num_sources, num_nodes) with predecessor indices
        )pbdoc"
    );
}
