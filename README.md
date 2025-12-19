# Fast_dijkstra



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