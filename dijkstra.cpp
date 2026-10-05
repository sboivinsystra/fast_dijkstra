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
    const int* targets,
    double* output_dist,
    int* pred,
    int num_targets,
    double cutoff
) {

    using Item = std::pair<double, int>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> heap;

    // Full graph search state
    std::vector<double> dist(g.n, INF);

    dist[source] = 0.0;
    heap.emplace(0.0, source);
    int remaining_targets = num_targets;

    while (!heap.empty() && remaining_targets > 0) {
        auto [current_dist, current_node] = heap.top();
        heap.pop();
        if (current_dist > dist[current_node]) continue;
        
        //only store distance if node in the targets
        int target_idx = targets[current_node];
        if (target_idx >= 0) {
            output_dist[target_idx] = current_dist;
            --remaining_targets;
        }

         // No need to explore beyond cutoff
        if (current_dist >= cutoff) continue;

        // check neighbors
        for (int j = g.indptr[current_node]; j < g.indptr[current_node + 1]; ++j) {
            int next_node = g.indices[j];
            double next_dist = current_dist + g.weights[j];
            if ((next_dist <= cutoff) && (next_dist < dist[next_node])){
                dist[next_node] = next_dist;
                if (pred != nullptr)  pred[next_node] = current_node; //  add to pred 
                heap.emplace(next_dist, next_node);
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
    py::object targets = py::none(),
    py::object cutoff =  py::float_(INF),
    bool return_predecessors=true,
    int num_threads = -1
) {

    if (num_threads > 0) omp_set_num_threads(num_threads);

    const int num_nodes = indptr.size() - 1;
    const int num_sources = sources.size();
    const int* src_ptr = sources.data();

    // ---- Build graph (once) ----
    CSRGraph g;
    g.n = num_nodes;
    g.indptr = indptr.data();
    g.indices = indices.data();
    g.weights = weights.data();

    // cutoff

    std::vector<double> source_cutoffs;
    if (py::isinstance<py::float_>(cutoff) ||
        py::isinstance<py::int_>(cutoff)) {
        source_cutoffs.assign(num_sources, cutoff.cast<double>());
    } else {
        py::array_t<double> cutoff_array = cutoff.cast<py::array_t<double>>();
        if (cutoff_array.size() != num_sources) throw std::runtime_error("cutoff array must have one value per source" );
        auto cutoff_ptr = cutoff_array.data();
        source_cutoffs.assign(cutoff_ptr,cutoff_ptr + num_sources);
    }


    // targets. if none uses all nodes. else create a lut to remap nodes to the targets
    
    std::vector<int> target_index(num_nodes, -1);  
    const int* target_index_ptr = target_index.data();  
    int num_targets;

    if (targets.is_none()) {
        num_targets = num_nodes;
        for (int i = 0; i < num_nodes; ++i) target_index[i] = i;   
    } else {
        auto targets_array = targets.cast<py::array_t<int>>();
        num_targets = targets_array.size();
        const int* targets_ptr = targets_array.data();
        for (int i = 0; i < num_targets; ++i) target_index[targets_ptr[i]] = i;
    }


    // ---- Allocate outputs ----
    // create a matrix (num_source,num_targets) full of inf
    py::array_t<double> distances_out({num_sources, num_targets});
    auto* distances_ptr = distances_out.mutable_data();
    std::fill_n(distances_ptr, num_sources * num_targets, INF);

    // if return predecessor. create a full matrix with all nodes for it
    py::array_t<int> predecessors_out;
    if (return_predecessors) {
        predecessors_out = py::array_t<int>({num_sources, num_nodes});
        auto* predecessors_ptr = predecessors_out.mutable_data();
        std::fill_n(predecessors_ptr, num_sources * num_nodes, -9999);
    }


   
    #pragma omp parallel for schedule(dynamic)
    for (int si = 0; si < num_sources; ++si) {
        double source_cutoff = source_cutoffs[si];
        double *dist_row = distances_ptr + si * num_targets;
        int* pred_row = nullptr;
        if (return_predecessors) pred_row = predecessors_out.mutable_data() + si * num_nodes;

        
        single_source_dijkstra(
            g,
            src_ptr[si],
            target_index_ptr,
            dist_row,
            pred_row,
            num_targets,
            source_cutoff
        );
    }

    return {distances_out, predecessors_out};
}

/* ---------------- pybind11 module ---------------- */

PYBIND11_MODULE(fast_dijkstra, m) {
    m.doc() = "Fast multi-source Dijkstra yolo";

    m.def("dijkstra", &multi_source_dijkstra,
        py::arg("indptr"),
        py::arg("indices"),
        py::arg("weights"),
        py::arg("sources"),
        py::arg("targets")= py::none(),
        py::arg("cutoff") = INF,
        py::arg("return_predecessors") = true,
        py::arg("num_threads") = -1,
        R"pbdoc(
        Compute shortest paths using Dijkstra's algorithm. on a csr_graph

        Parameters
        ----------
        indptr : list[int]
        indices : list[int]
        weights : Sequence[float]
        sources : list[int]
        targets: OPTIONAL list[int]  if None uses all nodes
        cutoff: OPTIONAL list[float] | float (default = np.inf). can pass a limit per sources
        return_predecessors: OPTIONAL bool
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
