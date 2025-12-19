# Fast_dijkstra

wrapper of c++ Boost Dijkstra https://www.boost.org/doc/libs/latest/libs/graph/doc/dijkstra_shortest_paths.html

## usage
edges must be integers
```py
from fast_dijkstra import directed_dijkstra

edges = [(0, 1), (0, 2), (1, 2), (1, 3), (2, 3)]
weights = [1.0, 4.0, 2.0, 5.0, 1.0]
sources = [0, 1]
num_threads = 4

distances, predecessor = directed_dijkstra(edges, weights, sources, num_threads)
```



```py
from fast_dijkstra import limited_directed_dijkstra

edges = [(0, 1), (0, 2), (1, 2), (1, 3), (2, 3)]
weights = [1.0, 4.0, 2.0, 5.0, 1.0]
sources = [0, 1]
num_threads = 4
limit=1000

distances, predecessor = limited_directed_dijkstra(edges, weights, sources, limit, num_threads)
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
sudo apt-get install -y libboost-all-dev

poetry run python setup.py bdist_wheel

 or 

poetry run python -m build --wheel