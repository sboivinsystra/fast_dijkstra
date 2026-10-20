## [4.0.0] (2026-10-20)
## Features
* New dijkstra_on_multi_targets. provide targets per sources (as CRS matrix).

## [3.1.0] (2026-10-13)
## changes
* add back version 2.1.1 of dijkstra as it may be faster for simple tasks
* rename new version with targets, cutoff (list) and  return_predecessors as dijkstra_with_targets

## [3.0.0] (2026-10-05)
## Features
* targets: list[int] or None. will return only the distance for the specify targets.
* return_predecessors: bool. will return an empty matrix if False
* cutoff: list[float] | float. can use a cutoff per source or a global one.

## [2.1.1] (2026-01-05)
## changes
* remove limit early exit (more like scipy)
* rename variables

## [2.1.0] (2026-01-05)
## changes
* add num_threads as argument (-1 is auto)

## [2.0.2] (2025-12-23)
## Bug fixes
* add macOS wheel

## [2.0.1] (2025-12-19)
## Bug fixes
* fix windows compilation (c++17)

## [2.0.0] (2025-12-19)
## changes
* remove Boost do a minHeap dijkstra with std::priority_queue

## [2.0.0] (2025-12-19)
## changes
* remove Boost do a minHeap dijkstra with std::priority_queue

## [1.1.2] (2025-12-19)
## changes
* smaller linux build size

## [1.1.1] (2025-12-17)
## changes
* add example in the readme.
* rename repo
* add changelog

## [1.1.0] (2025-12-17)
## changes
* predecessor filled with -9999 (like scipy)