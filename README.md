## Gacher
Gacher-cli — a utility for serendipitous reading of articles from interesting categories. Crawl first, then discover random things from your chosen topics.

## Setup
Before starting, you need to create a config.json file with the following fields:

- `db-name` - database name created by the CLI;

- `max-depth` - depth of category crawl (lower depth = articles closer to the original category);

- `categories` - array of categories of interest, e.g. "Category:Physics";

- `user-agent` - custom User-Agent string for HTTP requests;

Example in [config.example.json](https://github.com/vakunization/gacher-cli/blob/main/config.example.json).

## Prerequisites
[C++17](https://gcc.gnu.org/gcc-15/), [libcurl](https://curl.se/download.html), [SQLite3](https://sqlite.org/download.html), [nlohmann/json](https://github.com/nlohmann/json/blob/develop/single_include/nlohmann/json.hpp), [sqlite_modern_cpp](https://github.com/SqliteModernCpp/sqlite_modern_cpp/tree/master/hdr)

**nlohmann/json** and **sqlite_modern_cpp** libraries must be placed in the `lib/` folder (relative to CLI root).

## Usage
### Installation
```bash
git clone ...
cd gacher-cli
make
```
### Collect articles from categories
```bash
./gacher --crawl
```
### Get a random article
```bash
./gacher --get
```

## Future Improvements
- Add arXiv.org to the list of sources
- Implement semantic search to find relevant articles
- Deploy as a web service

