# Fast_dijkstra

wrapper of c++ Boost Dijkstra https://www.boost.org/doc/libs/latest/libs/graph/doc/dijkstra_shortest_paths.html

# to deploy

1) change the version number in pyproject.toml under [project]
```toml
[project]
name = "fast_dijkstra"
version = "1.0.2"
```
2) create a new tag starting with "v"

```sh
git tag -a 'v1.0.2' -m 'description'
```

Github action will build wheels for windows and Linux.

3) when Done, upload to Pypi


```sh
./upload v1.0.2
```

# local development build
sudo apt-get install -y libboost-all-dev

poetry run python setup.py bdist_wheel
