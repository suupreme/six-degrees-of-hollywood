# Sample dataset

A small, hand-written dataset in the same format as the preprocessed IMDb files,
for testing. The people and movies are made up, and the `nm`/`tt` IDs are fake
(`nm9000000`, `tt9100000`, ...) so they can't be mistaken for real IMDb entries.

- `actors.tsv`: 10 actors (indices 0-9)
- `movies.tsv`: 6 movies (indices 0-5)
- `edges.tsv`: 15 actor-movie credits

## Cast of each movie

| Movie | Cast (actor indices) |
|---|---|
| 0 Midnight Harbor (1998) | 0 Ava Brooks, 1 Ben Carter |
| 1 Glass Orchard (2003) | 1 Ben Carter, 2 Cara Diaz |
| 2 The Long Static (2008) | 2 Cara Diaz, 3 Alex Morgan (1975), 4 Dana Ellis |
| 3 Paper Lanterns (2012) | 4 Dana Ellis, 5 Eli Foster, 7 Gus Hill |
| 4 Northbound (2016) | 5 Eli Foster, 6 Fay Garner |
| 5 Second Chorus (2020) | 0 Ava Brooks, 7 Gus Hill, 8 Alex Morgan (1990) |

## Edge cases it covers

- **Duplicate name:** actors 3 and 8 are both "Alex Morgan" (born 1975 and 1990).
  `findActor("Alex Morgan")` should return `{3, 8}`.
- **No connections:** actor 9, Hana Ito, appears in no movie.
- **Missing birth year:** actor 7, Gus Hill, has `\N` (IMDb's missing-value marker).
- **More than one route:** Ava Brooks reaches Dana Ellis in 2 movies through
  Gus Hill (Second Chorus, then Paper Lanterns), and in 3 through Ben Carter and Cara Diaz.

## Expected shortest degrees

Degrees = number of movies on the shortest path; -1 means no connection.
`tests/test_main.cpp` checks these values.

| From | To | Degrees | One shortest path |
|---|---|---|---|
| 0 Ava Brooks | 0 Ava Brooks | 0 | (same actor) |
| 0 Ava Brooks | 1 Ben Carter | 1 | Midnight Harbor |
| 0 Ava Brooks | 8 Alex Morgan (1990) | 1 | Second Chorus |
| 0 Ava Brooks | 4 Dana Ellis | 2 | Second Chorus → Gus Hill → Paper Lanterns |
| 0 Ava Brooks | 3 Alex Morgan (1975) | 3 | Midnight Harbor → Ben Carter → Glass Orchard → Cara Diaz → The Long Static |
| 0 Ava Brooks | 6 Fay Garner | 3 | Second Chorus → Gus Hill → Paper Lanterns → Eli Foster → Northbound |
| 3 Alex Morgan (1975) | 8 Alex Morgan (1990) | 3 | The Long Static → Dana Ellis → Paper Lanterns → Gus Hill → Second Chorus |
| 1 Ben Carter | 6 Fay Garner | 4 | Glass Orchard → Cara Diaz → The Long Static → Dana Ellis → Paper Lanterns → Eli Foster → Northbound |
| 9 Hana Ito | 0 Ava Brooks | -1 | (no connection) |
| 9 Hana Ito | 9 Hana Ito | 0 | (same actor) |
