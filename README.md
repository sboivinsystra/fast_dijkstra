# Fast_dijkstra

Fast directed Dijkstra written in C++ with parallelization build in


# Function definition

```toml
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

```

## usage
* The graph is passed as a csr_matrix just like scipy. you can reuse scipy to create the graph from edges.
* edges must be integers. starting at 0
* Graph is directed

```py
from scipy.sparse import csr_matrix
from fast_dijkstra import dijkstra, dijkstra_with_targets, dijkstra_on_multi_targets

edges = [(0, 1), (0, 2), (1, 2), (1, 3), (2, 3)]
weights = [1.0, 4.0, 2.0, 5.0, 1.0]
sources = [0, 1]

row = [e[0] for e in edges]
col = [e[1] for e in edges]

nodelist = sorted({e[0] for e in edges}.union({e[1] for e in edges}))
nlen = len(nodelist)

sparse = csr_matrix((weights, (row, col)), shape=(nlen, nlen))
indptr = sparse.indptr  # [0, 2, 4, 5, 5]
indices = sparse.indices  # [1, 2, 2, 3, 3]

# basic usage
distances, predecessor = dijkstra(indptr=indptr, indices=indices, weights=weights, sources=sources)

distances   # [
            #   [ 0.,  1.,  3.,  4.],
            #   [inf,  0.,  2.,  3.]
            # ]

predecessor # [
            #   [-9999,     0,     1,     2],
            #   [-9999, -9999,     1,     2]
            # ]


# with limits, and threads
distances, predecessor = dijkstra(
    indptr=indptr, indices=indices, weights=weights, sources=sources, cutoff=10, num_threads=2
)

# with targets (return smaller predecessor/distances)
distances, predecessor = dijkstra_with_targets(
    indptr=indptr, indices=indices, weights=weights, sources=sources, targets=[3]
)

# with limits, and threads
distances, predecessor = dijkstra_with_targets(
    indptr=indptr, indices=indices, weights=weights, sources=sources, targets=[3]
)

# with target per source.

targets = [(0, 3), (1, 2), (1, 3)]
row = [e[0] for e in targets]
col = [e[1] for e in targets]
target_mat = csr_matrix(([1, 1, 1], (row, col)), shape=(nlen, nlen))


distances, predecessor = dijkstra_on_multi_targets(
    indptr=indptr,
    indices=indices,
    weights=weights,
    sources=sources,
    target_indptr=target_mat.indptr,
    target_indices=target_mat.indices,
)
```


# to deploy

1) change the version number in pyproject.toml under [project]
```toml
[project]
name = "fast_dijkstra"
version = "1.0.2"
```

* the poetry config is only used for local dev. to ignore.

2) create a new tag starting with "v"

```sh
git tag -a 'v1.0.2' -m 'description'
```

```sh
git push origin v1.0.2
```

Github action will build wheels for windows and Linux.

3) when Done, upload to Pypi


```sh
./upload v1.0.2
```

# local development build

poetry run python setup.py bdist_wheel

 or 

poetry run python -m build --wheel