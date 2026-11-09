# ui/

Header-only code used by `src/main.cpp`.

- `console.h`: finds actors by name or nm id, lets the user pick between actors with the same name, runs the searches and prints the results.
- `path_export.h`: saves a search result as JSON for `viz/index.html`.

## Running

```
six_degrees [dataDir] [options]
  --from NAME --to NAME       run one search instead of asking
  --algo bfs|bidir|both       which search to run (default both)
  --export FILE               save each search as JSON for viz/index.html
  --benchmark PAIRS OUT.csv   PAIRS is a number of random pairs, or a file of nm id pairs
```

Example:

```
six_degrees data/sample --from "Ava Brooks" --to "Fay Garner" --export viz/path.json
```

To view the exported path, run `python -m http.server --directory viz 8000` and open http://localhost:8000, or open `viz/index.html` and choose the JSON file.
