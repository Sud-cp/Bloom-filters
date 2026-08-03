// Benchmarks the Counting Bloom Filter against std::map<std::string,bool>
// for insertion and lookup, and reports memory usage for both. This is
// the benchmark referenced on the resume: "Benchmarked performance
// against std::map, demonstrating lower memory usage and faster lookup
// for large datasets."

#include "CountingBloomFilter.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace {

std::vector<std::string> generateRandomStrings(std::size_t count, std::size_t length, unsigned int seed) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, sizeof(charset) - 2);

    std::vector<std::string> result;
    result.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        std::string s(length, '\0');
        for (std::size_t j = 0; j < length; ++j) {
            s[j] = charset[dist(rng)];
        }
        result.push_back(std::move(s));
    }
    return result;
}

template <typename Fn>
double timeMillis(Fn&& fn) {
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// Rough estimate of std::map<std::string, bool> memory footprint.
// Each red-black tree node carries color/parent/left/right pointers plus
// the key (std::string, which itself heap-allocates for longer strings)
// and the bool value. This is an approximation for comparison purposes,
// not an exact accounting of allocator overhead.
std::size_t estimateMapMemoryBytes(std::size_t count, std::size_t keyLength) {
    constexpr std::size_t nodeOverhead = 32;       // color + 3 pointers, roughly
    constexpr std::size_t stringOverhead = 32;      // std::string control block (SSO-dependent)
    std::size_t perEntry = nodeOverhead + stringOverhead + keyLength + sizeof(bool);
    return perEntry * count;
}

} // namespace

int main() {
    const std::size_t datasetSize = 200000;
    const std::size_t keyLength = 12;
    const double targetFpr = 0.01;

    std::cout << "Generating " << datasetSize << " random keys...\n";
    auto insertedKeys = generateRandomStrings(datasetSize, keyLength, /*seed=*/42);
    auto lookupKeys   = generateRandomStrings(datasetSize, keyLength, /*seed=*/1337); // disjoint w.h.p.

    // --- Counting Bloom Filter ---
    CountingBloomFilter cbf(datasetSize, targetFpr);

    double cbfInsertMs = timeMillis([&] {
        for (const auto& key : insertedKeys) cbf.insert(key);
    });

    std::size_t cbfFalsePositives = 0;
    double cbfLookupMs = timeMillis([&] {
        for (const auto& key : lookupKeys) {
            if (cbf.contains(key)) ++cbfFalsePositives;
        }
    });

    // --- std::map ---
    std::map<std::string, bool> map;
    double mapInsertMs = timeMillis([&] {
        for (const auto& key : insertedKeys) map[key] = true;
    });

    std::size_t mapHits = 0;
    double mapLookupMs = timeMillis([&] {
        for (const auto& key : lookupKeys) {
            if (map.find(key) != map.end()) ++mapHits;
        }
    });

    std::size_t cbfMemory = cbf.memoryUsageBytes();
    std::size_t mapMemory = estimateMapMemoryBytes(datasetSize, keyLength);

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "\n=== Results (n = " << datasetSize << " keys) ===\n\n";

    std::cout << "Insertion time:\n";
    std::cout << "  Counting Bloom Filter: " << cbfInsertMs << " ms\n";
    std::cout << "  std::map:               " << mapInsertMs << " ms\n\n";

    std::cout << "Lookup time (" << datasetSize << " queries, mostly non-members):\n";
    std::cout << "  Counting Bloom Filter: " << cbfLookupMs << " ms\n";
    std::cout << "  std::map:               " << mapLookupMs << " ms\n\n";

    std::cout << "Memory usage:\n";
    std::cout << "  Counting Bloom Filter: " << cbfMemory << " bytes ("
              << (cbfMemory / 1024.0 / 1024.0) << " MB)\n";
    std::cout << "  std::map (estimated):  " << mapMemory << " bytes ("
              << (mapMemory / 1024.0 / 1024.0) << " MB)\n\n";

    std::cout << "Observed false-positive rate: "
              << (100.0 * static_cast<double>(cbfFalsePositives) / static_cast<double>(lookupKeys.size()))
              << "% (target was " << (targetFpr * 100.0) << "%)\n";
    std::cout << "std::map exact match count on disjoint lookup set: " << mapHits << " (expected ~0)\n";

    return 0;
}
