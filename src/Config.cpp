#include "../include/Config.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace {
double readPositive(const char* prompt, double current) {
    std::cout << prompt << " [" << current << "]: ";
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) return current;
    try {
        std::size_t used = 0;
        double value = std::stod(line, &used);
        if (used != line.size() || value <= 0.0 || !std::isfinite(value)) throw 1;
        return value;
    } catch (...) { std::cout << "Nilai tidak valid; konfigurasi tidak diubah.\n"; return current; }
}
}

void ConfigManager::edit(Config& config) {
    std::cout << "Pengaturan berlaku selama aplikasi berjalan. Tekan Enter untuk mempertahankan nilai.\n";
    Config candidate = config;
    candidate.nominalVoltageVrms = readPositive("Tegangan nominal RMS (V)", config.nominalVoltageVrms);
    candidate.nominalFrequencyHz = readPositive("Frekuensi nominal (Hz)", config.nominalFrequencyHz);
    const double lowerBoundary = readPositive("Batas interruption/bawah sag (%)", config.sagLowerFraction * 100.0) / 100.0;
    candidate.sagLowerFraction = lowerBoundary;
    candidate.interruptionFraction = lowerBoundary;
    candidate.sagUpperFraction = readPositive("Batas atas sag (%)", config.sagUpperFraction * 100.0) / 100.0;
    candidate.swellFraction = readPositive("Batas swell (%)", config.swellFraction * 100.0) / 100.0;
    candidate.thdLimitPercent = readPositive("Batas THD (%)", config.thdLimitPercent);
    if (candidate.sagLowerFraction >= candidate.sagUpperFraction ||
        candidate.sagUpperFraction >= 1.0 || candidate.swellFraction <= 1.0) {
        std::cout << "Konfigurasi ambang tidak konsisten; seluruh perubahan dibatalkan.\n";
        return;
    }
    config = candidate;
    std::cout << "Pengaturan diperbarui.\n";
}
