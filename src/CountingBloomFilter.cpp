#include "CountingBloomFilter.h"
#include "MurmurHash3.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
constexpr uint32_t kSeed1 = 0x9747b28cU;
constexpr uint32_t kSeed2 = 0x85ebca6bU;

std::size_t computeOptimalM(std::size_t n, double p) {
    double m = -(static_cast<double>(n) * std::log(p)) / (std::log(2.0) * std::log(2.0));
    return static_cast<std::size_t>(std::ceil(m));
}

unsigned int computeOptimalK(std::size_t m, std::size_t n) {
    if (n == 0) return 1;
    double k = (static_cast<double>(m) / static_cast<double>(n)) * std::log(2.0);
    return std::max(1u, static_cast<unsigned int>(std::round(k)));
}
} // namespace

CountingBloomFilter::CountingBloomFilter(std::size_t expectedItems, double targetFalsePositiveRate) {
    if (expectedItems == 0) {
        throw std::invalid_argument("expectedItems must be > 0");
    }
    if (targetFalsePositiveRate <= 0.0 || targetFalsePositiveRate >= 1.0) {
        throw std::invalid_argument("targetFalsePositiveRate must be in (0, 1)");
    }
    std::size_t m = computeOptimalM(expectedItems, targetFalsePositiveRate);
    unsigned int k = computeOptimalK(m, expectedItems);
    counters_.assign(m, 0);
    numHashes_ = k;
}

CountingBloomFilter::CountingBloomFilter(std::size_t numCounters, unsigned int numHashes, bool /*explicitTag*/) {
    if (numCounters == 0) {
        throw std::invalid_argument("numCounters must be > 0");
    }
    if (numHashes == 0) {
        throw std::invalid_argument("numHashes must be > 0");
    }
    counters_.assign(numCounters, 0);
    numHashes_ = numHashes;
}

void CountingBloomFilter::hashIndices(const std::string& key, std::vector<std::size_t>& outIndices) const {
    uint32_t h1, h2;
    MurmurHash3_x86_32(key.data(), static_cast<int>(key.size()), kSeed1, &h1);
    MurmurHash3_x86_32(key.data(), static_cast<int>(key.size()), kSeed2, &h2);

    outIndices.clear();
    outIndices.reserve(numHashes_);
    const std::size_t m = counters_.size();
    for (unsigned int i = 0; i < numHashes_; ++i) {
        // Kirsch-Mitzenmacher double hashing: h_i = (h1 + i*h2) mod m
        uint64_t combined = static_cast<uint64_t>(h1) + static_cast<uint64_t>(i) * static_cast<uint64_t>(h2);
        outIndices.push_back(static_cast<std::size_t>(combined % m));
    }
}

void CountingBloomFilter::insert(const std::string& key) {
    std::vector<std::size_t> indices;
    hashIndices(key, indices);
    for (std::size_t idx : indices) {
        if (counters_[idx] < 255) {
            ++counters_[idx];
        }
        // Saturating: if already at 255, leave it (extremely rare in
        // practice unless the filter is wildly overloaded).
    }
    ++itemCount_;
}

void CountingBloomFilter::remove(const std::string& key) {
    std::vector<std::size_t> indices;
    hashIndices(key, indices);
    for (std::size_t idx : indices) {
        if (counters_[idx] > 0) {
            --counters_[idx];
        }
    }
    if (itemCount_ > 0) {
        --itemCount_;
    }
}

bool CountingBloomFilter::contains(const std::string& key) const {
    std::vector<std::size_t> indices;
    hashIndices(key, indices);
    for (std::size_t idx : indices) {
        if (counters_[idx] == 0) {
            return false; // definitely not present
        }
    }
    return true; // probably present
}

double CountingBloomFilter::estimatedFalsePositiveRate() const {
    const double m = static_cast<double>(counters_.size());
    const double n = static_cast<double>(itemCount_);
    const double k = static_cast<double>(numHashes_);
    if (n == 0) return 0.0;
    double inner = 1.0 - std::exp(-k * n / m);
    return std::pow(inner, k);
}
