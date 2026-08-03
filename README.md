# Counting Bloom Filter

A C++17 implementation of a **Counting Bloom Filter** — a probabilistic set
membership structure that supports insertion, **deletion**, and lookup with
a tunable false-positive rate.

Unlike a standard Bloom filter (bits only, insert-and-lookup), each slot
here is a small saturating counter, so an item can be removed later
without corrupting the filter's state for other items that hash to the
same slot.

## Design

- **Hashing:** Two independent 32-bit hashes are computed per key using
  [MurmurHash3](https://github.com/aappleby/smhasher) with different
  seeds. The `k` slot indices are then derived with
  [Kirsch–Mitzenmacher double hashing](https://www.eecs.harvard.edu/~michaelm/postscripts/rsa2008.pdf):

  ```
  h_i(x) = (h1(x) + i * h2(x)) mod m,   i = 0 .. k-1
  ```

  This gives `k` effectively-independent slot indices from only 2 hash
  computations instead of `k`, which is the standard technique for
  making Bloom filters fast in practice.

- **Sizing:** Given an expected item count `n` and a target false-positive
  rate `p`, the counter array size `m` and hash count `k` are derived
  from the standard optimal Bloom filter formulas:

  ```
  m = -(n * ln(p)) / (ln 2)^2
  k = (m / n) * ln 2
  ```

- **Counters:** 1 byte per slot, saturating at 255 (chosen for simplicity;
  a production system handling extreme skew might pack 4-bit counters
  instead to halve memory, at the cost of a lower saturation ceiling).

## Project structure

```
bloomfilter/
├── include/
│   ├── CountingBloomFilter.h
│   └── MurmurHash3.h
├── src/
│   ├── CountingBloomFilter.cpp
│   ├── MurmurHash3.cpp
│   └── main.cpp          # small interactive demo
├── benchmark/
│   └── benchmark.cpp     # perf comparison vs std::map
└── CMakeLists.txt
```

## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
./demo
./benchmark
```

(Or without CMake: `g++ -std=c++17 -O2 -Iinclude -o demo src/*.cpp` etc.)

## Benchmark results

Measured on 200,000 random 12-character string keys, target false-positive
rate 1%, comparing against `std::map<std::string, bool>`:

| Metric | Counting Bloom Filter | std::map |
|---|---|---|
| Insert (200k keys) | 19.7 ms | 90.4 ms |
| Lookup (200k queries) | 17.4 ms | 88.1 ms |
| Memory | 1.83 MB | ~14.7 MB (estimated) |
| Observed false-positive rate | 0.986% | 0% (exact) |

The Bloom filter is roughly **5x faster** on both insertion and lookup and
uses about **1/8th the memory** of `std::map`, at the cost of accepting a
small, tunable false-positive rate and giving up exact membership /
iteration. The observed false-positive rate (0.986%) closely matches the
1% target, confirming the `m`/`k` sizing formulas are working as intended.

`std::map`'s memory figure is an estimate (red-black tree node overhead +
string storage), since C++ doesn't expose exact per-container memory
accounting — but it's directionally accurate and matches what you'd expect
from a pointer-heavy tree of heap-allocated strings versus a flat byte
array.

## API

```cpp
CountingBloomFilter filter(/*expectedItems=*/1000, /*targetFalsePositiveRate=*/0.01);

filter.insert("alice");
filter.contains("alice");   // true
filter.remove("alice");
filter.contains("alice");   // false (assuming no false positive)

filter.estimatedFalsePositiveRate();
filter.memoryUsageBytes();
```

## Caveats

- `remove()` should only be called on keys that were actually inserted.
  Removing a key that was never inserted (including one that merely
  triggered a false positive) can decrement counters shared with other
  real members and cause false negatives.
- This is a fixed-capacity filter — it doesn't resize. Sizing is based on
  the expected item count given at construction.
