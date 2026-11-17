#pragma once

#include "../Model.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct StoredAnalysis {
    std::int64_t id = 0;
    std::string analyzedAt;
    std::string sourcePath;
    std::string primaryStatus;
    std::size_t sampleCount = 0;
    double sampleRateHz = 0.0;
    AnalysisResult result;
};

class AnalysisDatabase {
public:
    static void initialize();

    static std::int64_t save(const MeasurementDataset&, const AnalysisResult&,
                             const Config&, const DisturbanceReport&);
    static std::vector<StoredAnalysis> loadRecent(std::size_t limit);
    static void load(std::int64_t id, MeasurementDataset&, AnalysisResult&,
                     Config&, DisturbanceReport&);
};
