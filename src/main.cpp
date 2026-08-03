#include "CountingBloomFilter.h"

#include <iomanip>
#include <iostream>

int main() {
    CountingBloomFilter filter(/*expectedItems=*/1000, /*targetFalsePositiveRate=*/0.01);

    std::cout << "Counting Bloom Filter demo\n";
    std::cout << "  counters (m): " << filter.numCounters() << "\n";
    std::cout << "  hash functions (k): " << filter.numHashes() << "\n\n";

    filter.insert("alice");
    filter.insert("bob");
    filter.insert("carol");

    auto check = [&](const std::string& key) {
        std::cout << "  contains(\"" << key << "\") = "
                  << std::boolalpha << filter.contains(key) << "\n";
    };

    std::cout << "After inserting alice, bob, carol:\n";
    check("alice");
    check("bob");
    check("dave"); // never inserted -> should be false (barring a false positive)

    filter.remove("bob");
    std::cout << "\nAfter removing bob:\n";
    check("alice");
    check("bob");

    std::cout << "\nEstimated false-positive rate at current load: "
              << std::fixed << std::setprecision(5)
              << filter.estimatedFalsePositiveRate() << "\n";
    std::cout << "Memory usage: " << filter.memoryUsageBytes() << " bytes\n";

    return 0;
}
