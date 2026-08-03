#ifndef COUNTING_BLOOM_FILTER_H
#define COUNTING_BLOOM_FILTER_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

// A Counting Bloom Filter supporting insertion, deletion, and
// probabilistic membership queries.
//
// Standard Bloom filters only support insertion and lookup; once a bit
// is set it can never be safely cleared, because multiple keys may hash
// to the same bit. A Counting Bloom Filter replaces each bit with a
// small saturating counter, so a slot can be decremented on deletion
// without corrupting the filter's state for other keys still present.
//
// Hash functions: rather than computing k independent hash functions
// (expensive), this uses Kirsch-Mitzenmacher double hashing:
//
//     h_i(x) = (h1(x) + i * h2(x)) mod m,   for i = 0 .. k-1
//
// where h1 and h2 come from two differently-seeded MurmurHash3 calls.
// This gives k effectively-independent indices from only 2 hash
// computations per operation.
class CountingBloomFilter {
public:
    // Construct a filter sized for `expectedItems` insertions at a
    // target false-positive rate `targetFalsePositiveRate` (e.g. 0.01
    // for 1%). Optimal m (counter array size) and k (hash count) are
    // derived using the standard Bloom filter formulas:
    //
    //     m = -(n * ln(p)) / (ln(2))^2
    //     k = (m / n) * ln(2)
    CountingBloomFilter(std::size_t expectedItems, double targetFalsePositiveRate);

    // Construct a filter with explicit counter-array size and hash count.
    CountingBloomFilter(std::size_t numCounters, unsigned int numHashes, bool /*explicitTag*/);

    void insert(const std::string& key);

    // Removes a key. Only call this on a key that was actually inserted;
    // deleting a key that was never inserted (including one that merely
    // triggered a false positive) can corrupt the filter for other keys.
    void remove(const std::string& key);

    // Probabilistic membership test.
    //   - false  => key is DEFINITELY NOT in the set.
    //   - true   => key is PROBABLY in the set (may be a false positive).
    bool contains(const std::string& key) const;

    std::size_t numCounters() const { return counters_.size(); }
    unsigned int numHashes() const { return numHashes_; }
    std::size_t itemCount() const { return itemCount_; }

    // Theoretical false-positive rate given current fill level:
    //     (1 - e^(-k*n/m))^k
    double estimatedFalsePositiveRate() const;

    // Approximate memory footprint in bytes (counter array only).
    std::size_t memoryUsageBytes() const { return counters_.size() * sizeof(counters_[0]); }

private:
    std::vector<uint8_t> counters_;   // saturating counters, capped at 255
    unsigned int numHashes_;
    std::size_t itemCount_ = 0;

    void hashIndices(const std::string& key, std::vector<std::size_t>& outIndices) const;
};

#endif // COUNTING_BLOOM_FILTER_H
