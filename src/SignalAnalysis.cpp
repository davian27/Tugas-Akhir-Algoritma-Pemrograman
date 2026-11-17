#include "../include/SignalAnalysis.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace { const double PI = 3.14159265358979323846; }

void SignalAnalysis::calculateVoltageCurrent(const MeasurementDataset& data, AnalysisResult& result) {
    result.sampleCount = data.samples.size();
    result.voltageMax = -std::numeric_limits<double>::infinity();
    result.voltageMin = std::numeric_limits<double>::infinity();
    double voltageSquares = 0.0, currentSquares = 0.0;
    for (const auto& sample : data.samples) {
        result.voltageMax = std::max(result.voltageMax, sample.voltage);
        result.voltageMin = std::min(result.voltageMin, sample.voltage);
        voltageSquares += sample.voltage * sample.voltage;
        currentSquares += sample.current * sample.current;
    }
    const double n = static_cast<double>(data.samples.size());
    result.voltagePeakToPeak = result.voltageMax - result.voltageMin;
    result.voltageRms = std::sqrt(voltageSquares / n);
    result.currentRms = std::sqrt(currentSquares / n);
}

void SignalAnalysis::calculateFrequency(const MeasurementDataset& data, AnalysisResult& result) {
    std::vector<double> crossings;
    for (std::size_t i = 1; i < data.samples.size(); ++i) {
        const double a = data.samples[i - 1].voltage;
        const double b = data.samples[i].voltage;
        if (a <= 0.0 && b > 0.0) {
            const double fraction = (0.0 - a) / (b - a);
            crossings.push_back(data.samples[i - 1].timeSeconds + fraction *
                (data.samples[i].timeSeconds - data.samples[i - 1].timeSeconds));
        }
    }
    if (crossings.size() < 2) return;
    const double elapsed = crossings.back() - crossings.front();
    if (elapsed <= 0.0) return;
    result.frequencyHz = static_cast<double>(crossings.size() - 1) / elapsed;
    result.frequencyDefined = std::isfinite(result.frequencyHz) && result.frequencyHz > 0.0;
}

void SignalAnalysis::calculateVoltageThd(const MeasurementDataset& data, const Config& config, AnalysisResult& result) {
    if (!result.frequencyDefined) { result.thdNote = "Tidak dihitung: frekuensi fundamental tidak terdeteksi."; return; }
    if (data.samples.size() < 16) { result.thdNote = "Tidak dihitung: jumlah sampel terlalu sedikit."; return; }
    const double fs = data.sampleRateHz;
    const int maxOrder = std::min(config.maxHarmonicOrder, static_cast<int>(std::floor((fs * 0.5) / result.frequencyHz)));
    if (maxOrder < 2) { result.thdNote = "Tidak dihitung: Nyquist tidak mencakup harmonisa orde 2."; return; }

    double mean = 0.0;
    for (const auto& sample : data.samples) mean += sample.voltage;
    mean /= static_cast<double>(data.samples.size());
    std::vector<double> rms(maxOrder + 1, 0.0);
    for (int h = 1; h <= maxOrder; ++h) {
        double cosine = 0.0, sine = 0.0;
        for (const auto& sample : data.samples) {
            const double angle = 2.0 * PI * result.frequencyHz * h * sample.timeSeconds;
            const double centered = sample.voltage - mean;
            cosine += centered * std::cos(angle);
            sine += centered * std::sin(angle);
        }
        // Single-frequency DFT projection; most accurate for integer-cycle windows.
        rms[h] = std::sqrt(2.0) * std::hypot(cosine, sine) / static_cast<double>(data.samples.size());
    }
    if (rms[1] <= 1e-12) { result.thdNote = "Tidak dihitung: komponen fundamental mendekati nol."; return; }
    double harmonicSquares = 0.0;
    for (int h = 2; h <= maxOrder; ++h) harmonicSquares += rms[h] * rms[h];
    result.thdPercent = std::sqrt(harmonicSquares) / rms[1] * 100.0;
    result.thdDefined = std::isfinite(result.thdPercent);
    result.thdNote = "DFT proyeksi sampai orde " + std::to_string(maxOrder) + "; hasil sensitif terhadap kebocoran spektrum dan jendela non-integer-cycle.";
}
