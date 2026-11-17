#pragma once

#include "../Model.h"
#include <filesystem>
#include <string>

class ReportManager {
public:
    static void print(const MeasurementDataset&, const AnalysisResult&, const Config&, const DisturbanceReport&);
    static std::string appendLog(const MeasurementDataset&, const AnalysisResult&, const Config&, const DisturbanceReport&);
    static void exportPdf(const MeasurementDataset&, const AnalysisResult&, const Config&,
                          const DisturbanceReport&, const std::filesystem::path& outputPath);
};
