#include "../include/DisturbanceDetector.h"

DisturbanceReport DisturbanceDetector::evaluate(const AnalysisResult& result, const Config& config) {
    DisturbanceReport report;
    const double v = result.voltageRms;
    const double nominal = config.nominalVoltageVrms;
    if (v < config.interruptionFraction * nominal) report.conditions.push_back("INTERRUPTION");
    else if (v >= config.sagLowerFraction * nominal && v < config.sagUpperFraction * nominal) report.conditions.push_back("VOLTAGE SAG");
    if (v > config.swellFraction * nominal) report.conditions.push_back("VOLTAGE SWELL");
    if (result.thdDefined && result.thdPercent > config.thdLimitPercent) report.conditions.push_back("HARMONIC DISTORTION");
    if (report.conditions.empty()) report.conditions.push_back("NORMAL (berdasarkan aturan konfigurasi)");
    else if (report.conditions.size() > 1) report.conditions.push_back("MULTIPLE DISTURBANCE");
    return report;
}
