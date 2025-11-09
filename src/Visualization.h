#pragma once
#include <map>
#include <string>
#include "tree.h"
#include "hashTable.h"

namespace Visualization {

// Runs the full SFML UI and event loop.
// - Colors the US map by Unemployment_Rate for the selected year
// - Attribute toggle affects the point lookup only (map always uses Unemployment_Rate)
// - Output shows ONLY the numeric value returned by hashData.search(...)
// Returns 0 on normal window close, nonzero on asset/load errors.
    int visualizer(
            Tree tree, hashTable hashData, std::vector<float> stateData
    );

} // namespace Visualization
