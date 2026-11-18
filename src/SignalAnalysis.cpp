#include "../include/SignalAnalysis.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace { const double PI = 3.14159265358979323846; }

void SignalAnalysis::calculateVoltageCurrent(const MeasurementDataset& data, AnalysisResult& result) {
    if (data.samples.empty()) throw std::runtime_error("Tidak ada sampel untuk dihitung.");
    result.sampleCount = data.samples.size();
    result.voltageMax = -std::numeric_limits<double>::infinity();
    result.voltageMin = std::numeric_limits<double>::infinity();
    double voltageScale = 0.0, currentScale = 0.0;
    double voltageScaledSquares = 0.0, currentScaledSquares = 0.0;
    for (const auto& sample : data.samples) {
        if (!std::isfinite(sample.timeSeconds) || !std::isfinite(sample.voltage) || !std::isfinite(sample.current))
            throw std::runtime_error("Data sampel mengandung nilai bukan angka finite.");
        result.voltageMax = std::max(result.voltageMax, sample.voltage);
        result.voltageMin = std::min(result.voltageMin, sample.voltage);
        const double absVoltage = std::fabs(sample.voltage);
        if (absVoltage > voltageScale) {
            const double ratio = voltageScale == 0.0 ? 0.0 : voltageScale / absVoltage;
            voltageScaledSquares = 1.0 + voltageScaledSquares * ratio * ratio;
            voltageScale = absVoltage;
        } else if (voltageScale > 0.0) {
            const double ratio = absVoltage / voltageScale;
            voltageScaledSquares += ratio * ratio;
        }
        const double absCurrent = std::fabs(sample.current);
        if (absCurrent > currentScale) {
            const double ratio = currentScale == 0.0 ? 0.0 : currentScale / absCurrent;
            currentScaledSquares = 1.0 + currentScaledSquares * ratio * ratio;
            currentScale = absCurrent;
        } else if (currentScale > 0.0) {
            const double ratio = absCurrent / currentScale;
            currentScaledSquares += ratio * ratio;
        }
    }
    const double n = static_cast<double>(data.samples.size());
    result.voltagePeakToPeak = result.voltageMax - result.voltageMin;
    result.voltageRms = voltageScale * std::sqrt(voltageScaledSquares / n);
    result.currentRms = currentScale * std::sqrt(currentScaledSquares / n);
    if (!std::isfinite(result.voltagePeakToPeak) || !std::isfinite(result.voltageRms) ||
        !std::isfinite(result.currentRms))
        throw std::runtime_error("Nilai tegangan atau arus terlalu besar untuk dihitung dengan aman.");
}

void SignalAnalysis::calculateFrequency(const MeasurementDataset& data, AnalysisResult& result) {
    result.frequencyDefined = false;
    result.frequencyHz = 0.0;
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
    result.thdDefined = false;
    result.thdPercent = 0.0;
    result.thdNote.clear();
    if (!result.frequencyDefined) { result.thdNote = "Tidak dihitung: frekuensi fundamental tidak terdeteksi."; return; }
    if (data.samples.size() < 16) { result.thdNote = "Tidak dihitung: jumlah sampel terlalu sedikit."; return; }
    const double fs = data.sampleRateHz;
    if (!std::isfinite(fs) || fs <= 0.0 || !std::isfinite(result.frequencyHz) || result.frequencyHz <= 0.0) {
        result.thdNote = "Tidak dihitung: frekuensi sampling atau fundamental tidak valid.";
        return;
    }
    const double nyquistOrder = std::floor((fs * 0.5) / result.frequencyHz);
    const int maxOrder = nyquistOrder >= static_cast<double>(config.maxHarmonicOrder)
        ? config.maxHarmonicOrder : static_cast<int>(nyquistOrder);
    if (maxOrder < 2) { result.thdNote = "Tidak dihitung: Nyquist tidak mencakup harmonisa orde 2."; return; }

    double mean = 0.0;
    const double sampleCount = static_cast<double>(data.samples.size());
    for (const auto& sample : data.samples) mean += sample.voltage / sampleCount;
    const double firstTime = data.samples.front().timeSeconds;
    std::vector<double> rms(maxOrder + 1, 0.0);
    for (int h = 1; h <= maxOrder; ++h) {
        double cosine = 0.0, sine = 0.0;
        double scale = 0.0;
        for (const auto& sample : data.samples) {
            const double centered = sample.voltage - mean;
            if (!std::isfinite(centered)) {
                result.thdNote = "Tidak dihitung: rentang tegangan terlalu besar.";
                return;
            }
            scale = std::max(scale, std::fabs(centered));
        }
        if (scale == 0.0) {
            rms[h] = 0.0;
            continue;
        }
        for (const auto& sample : data.samples) {
            const double angle = 2.0 * PI * result.frequencyHz * h * (sample.timeSeconds - firstTime);
            if (!std::isfinite(angle)) {
                result.thdNote = "Tidak dihitung: rentang waktu terlalu besar untuk analisis harmonisa.";
                return;
            }
            const double normalized = (sample.voltage - mean) / scale;
            cosine += normalized * std::cos(angle);
            sine += normalized * std::sin(angle);
        }
        // Single-frequency DFT projection; most accurate for integer-cycle windows.
        rms[h] = scale * std::sqrt(2.0) * std::hypot(cosine, sine) / sampleCount;
        if (!std::isfinite(rms[h])) {
            result.thdNote = "Tidak dihitung: nilai harmonisa terlalu besar untuk dihitung.";
            return;
        }
    }
    if (rms[1] <= 1e-12) { result.thdNote = "Tidak dihitung: komponen fundamental mendekati nol."; return; }
    double harmonicRms = 0.0;
    for (int h = 2; h <= maxOrder; ++h) harmonicRms = std::hypot(harmonicRms, rms[h]);
    result.thdPercent = harmonicRms / rms[1] * 100.0;
    result.thdDefined = std::isfinite(result.thdPercent);
    if (!result.thdDefined) {
        result.thdPercent = 0.0;
        result.thdNote = "Tidak dihitung: hasil THD berada di luar rentang numerik.";
        return;
    }
    result.thdNote = "DFT proyeksi sampai orde " + std::to_string(maxOrder) + "; hasil sensitif terhadap kebocoran spektrum dan jendela non-integer-cycle.";
}
