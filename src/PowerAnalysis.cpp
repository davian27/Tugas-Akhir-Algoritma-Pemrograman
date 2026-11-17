#include "../include/PowerAnalysis.h"

void PowerAnalysis::calculate(const MeasurementDataset& data, AnalysisResult& result) {
    double instantaneousPowerSum = 0.0;
    for (const auto& sample : data.samples) instantaneousPowerSum += sample.voltage * sample.current;
    result.activePowerW = instantaneousPowerSum / static_cast<double>(data.samples.size());
    result.apparentPowerVa = result.voltageRms * result.currentRms;
    result.powerFactorDefined = result.apparentPowerVa > 0.0;
    if (result.powerFactorDefined) result.powerFactor = result.activePowerW / result.apparentPowerVa;
}
