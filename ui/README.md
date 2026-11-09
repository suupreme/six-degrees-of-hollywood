# ui/

Header-only code used by `src/main.cpp`.

- `console.h`: finds actors by name or nm id, lets the user pick between actors with the same name, runs the searches and prints the results.
- `path_export.h`: saves a search result as JSON for `viz/index.html`.
- `server.h`: a small local web server for `--serve`. It serves `viz/` and answers `/api/info`, `/api/actors?q=` and `/api/search?from=&to=`.

## Running

```
six_degrees [dataDir] [options]
  --from NAME --to NAME       run one search instead of asking
  --algo bfs|bidir|both       which search to run (default both)
  --export FILE               save each search as JSON for viz/index.html
  --benchmark PAIRS OUT.csv   PAIRS is a number of random pairs, or a file of nm id pairs
  --serve [PORT]              run the web UI at http://localhost:PORT (default 8080)
  --web DIR                   folder with the web UI files (default: the repo's viz/)
```

Example:

```
six_degrees data/sample --from "Ava Brooks" --to "Fay Garner" --export viz/path.json
```

## Web UI

```
six_degrees data/small --serve
```

Then open http://localhost:8080. Type two actors (suggestions appear as you type), and the page shows the shortest chain, each step, and how BFS and bidirectional BFS compare. If a name matches several actors, it asks which one you meant.

Without the server, `viz/index.html` shows the last path saved with `--export viz/path.json`. To view the exported path, run `python -m http.server --directory viz 8000` and open http://localhost:8000, or open `viz/index.html` and choose the JSON file.
