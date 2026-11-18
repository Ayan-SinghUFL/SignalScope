#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <cfloat>
#include <string>
#include "CellNode.h"
#include "KDTree.h"
#include "MathUtils.h"

using namespace std;
using namespace std::chrono;

// Florida bounding box (approx) -- keeps generated towers on/around the state.
const double FL_LAT_MIN = 24.5,  FL_LAT_MAX = 31.0;
const double FL_LON_MIN = -87.6, FL_LON_MAX = -80.0;

// Baseline "dumb" search: scan every tower and keep the closest.
// Guaranteed correct, but O(n) per query -- this is what the KD-Tree beats.
const CellNode* bruteForceNearest(const vector<CellNode>& pts,
                                  double lat, double lon) {
    const CellNode* best = nullptr;
    double bestDist = DBL_MAX;
    for (const CellNode& p : pts) {
        double d = MathUtils::haversineDistance(lat, lon, p.lat, p.lon);
        if (d < bestDist) {
            bestDist = d;
            best = &p;
        }
    }
    return best;
}

int main(int argc, char** argv) {
    // Dataset size + query count are configurable:  benchmark <N> <M>
    int N = (argc > 1) ? stoi(argv[1]) : 100000; // number of towers
    int M = (argc > 2) ? stoi(argv[2]) : 1000;   // number of queries

    // Fixed seed => same random data every run => fair, repeatable timings.
    mt19937 rng(42);
    uniform_real_distribution<double> latDist(FL_LAT_MIN, FL_LAT_MAX);
    uniform_real_distribution<double> lonDist(FL_LON_MIN, FL_LON_MAX);

    // --- Generate the tower dataset ---
    vector<CellNode> towers;
    towers.reserve(N);
    for (int i = 0; i < N; i++) {
        towers.push_back({"tower_" + to_string(i), "LTE",
                          latDist(rng), lonDist(rng)});
    }

    // --- Generate a fixed set of random query points ---
    vector<pair<double, double>> queries;
    queries.reserve(M);
    for (int i = 0; i < M; i++) {
        queries.push_back({latDist(rng), lonDist(rng)});
    }

    cout << "=== KD-Tree Benchmark ===\n";
    cout << "Dataset: " << N << " towers, " << M << " queries "
         << "(Florida bounding box)\n\n";

    // --- Time the tree BUILD (inserting all N towers) ---
    KDTree tree;
    auto buildStart = high_resolution_clock::now();
    for (const CellNode& t : towers) {
        tree.insert(t);
    }
    auto buildEnd = high_resolution_clock::now();
    long long buildUs = duration_cast<microseconds>(buildEnd - buildStart).count();

    // --- Time KD-Tree SEARCH over all queries ---
    // Accumulate a checksum so the compiler can't optimize the calls away.
    double sink = 0.0;
    auto kdStart = high_resolution_clock::now();
    for (const auto& q : queries) {
        const CellNode* r = tree.nearestNeighbor(q.first, q.second);
        sink += r->lat;
    }
    auto kdEnd = high_resolution_clock::now();
    long long kdNs = duration_cast<nanoseconds>(kdEnd - kdStart).count();

    // --- Time BRUTE-FORCE SEARCH over the same queries ---
    auto bfStart = high_resolution_clock::now();
    for (const auto& q : queries) {
        const CellNode* r = bruteForceNearest(towers, q.first, q.second);
        sink += r->lat;
    }
    auto bfEnd = high_resolution_clock::now();
    long long bfNs = duration_cast<nanoseconds>(bfEnd - bfStart).count();

    // --- Correctness: does the KD-Tree agree with brute force? ---
    int correct = 0;
    for (const auto& q : queries) {
        const CellNode* kd = tree.nearestNeighbor(q.first, q.second);
        const CellNode* bf = bruteForceNearest(towers, q.first, q.second);
        double kdDist = MathUtils::haversineDistance(q.first, q.second, kd->lat, kd->lon);
        double bfDist = MathUtils::haversineDistance(q.first, q.second, bf->lat, bf->lon);
        if (fabs(kdDist - bfDist) < 1e-9) correct++;
    }

    // --- Report (times in microseconds, per the task spec) ---
    double kdAvgUs = (kdNs / 1000.0) / M;
    double bfAvgUs = (bfNs / 1000.0) / M;

    cout << "Build KD-Tree:       " << buildUs << " us total\n\n";
    cout << "KD-Tree search:      " << (kdNs / 1000.0) << " us total, "
         << kdAvgUs << " us/query\n";
    cout << "Brute-force search:  " << (bfNs / 1000.0) << " us total, "
         << bfAvgUs << " us/query\n\n";

    if (kdAvgUs > 0.0) {
        cout << "Speedup:             " << (bfAvgUs / kdAvgUs)
             << "x faster than brute force\n";
    }
    cout << "Correctness:         " << correct << "/" << M
         << " queries matched brute force\n";

    // Print the checksum so 'sink' is genuinely used (defeats optimizer).
    cout << "\n(checksum: " << sink << ")\n";
    return (correct == M) ? 0 : 1;
}
