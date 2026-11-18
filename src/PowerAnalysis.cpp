#include "../include/PowerAnalysis.h"

#include <cmath>
#include <stdexcept>

void PowerAnalysis::calculate(const MeasurementDataset& data, AnalysisResult& result) {
    if (data.samples.empty()) throw std::runtime_error("Tidak ada sampel untuk menghitung daya.");
    const double sampleCount = static_cast<double>(data.samples.size());
    result.activePowerW = 0.0;
    for (const auto& sample : data.samples) {
        const double instantaneousPower = sample.voltage * sample.current;
        if (!std::isfinite(instantaneousPower))
            throw std::runtime_error("Hasil perkalian tegangan dan arus terlalu besar untuk dihitung.");
        result.activePowerW += instantaneousPower / sampleCount;
        if (!std::isfinite(result.activePowerW))
            throw std::runtime_error("Daya aktif berada di luar rentang numerik.");
    }
    result.apparentPowerVa = result.voltageRms * result.currentRms;
    if (!std::isfinite(result.apparentPowerVa))
        throw std::runtime_error("Daya semu berada di luar rentang numerik.");
    result.powerFactorDefined = result.apparentPowerVa > 0.0;
    result.powerFactor = 0.0;
    if (result.powerFactorDefined) {
        result.powerFactor = result.activePowerW / result.apparentPowerVa;
        if (!std::isfinite(result.powerFactor))
            throw std::runtime_error("Faktor daya berada di luar rentang numerik.");
    }
}
