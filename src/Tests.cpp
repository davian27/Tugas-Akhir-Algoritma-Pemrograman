#include "../include/Tests.h"
#include "../include/SignalAnalysis.h"
#include "../include/PowerAnalysis.h"
#include "../include/DisturbanceDetector.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

void SelfTests::run() {
    const double pi = 3.14159265358979323846;
    MeasurementDataset data;
    data.sourcePath = "synthetic test";
    data.sampleRateHz = 5000.0;
    const double amplitude = 220.0 * std::sqrt(2.0);
    const double currentAmplitude = 2.0 * std::sqrt(2.0);
    for (int n = 0; n < 1000; ++n) {
        const double t = n / data.sampleRateHz;
        data.samples.push_back({t, amplitude * std::sin(2 * pi * 50 * t), currentAmplitude * std::sin(2 * pi * 50 * t - pi / 3)});
    }
    AnalysisResult result;
    Config config;
    SignalAnalysis::calculateVoltageCurrent(data, result);
    SignalAnalysis::calculateFrequency(data, result);
    PowerAnalysis::calculate(data, result);
    SignalAnalysis::calculateVoltageThd(data, config, result);
    if (std::fabs(result.voltageRms - 220.0) > 0.2) throw std::runtime_error("Uji Vrms gagal");
    if (!result.frequencyDefined || std::fabs(result.frequencyHz - 50.0) > 0.1) throw std::runtime_error("Uji frekuensi gagal");
    if (!result.powerFactorDefined || std::fabs(result.powerFactor - 0.5) > 0.01) throw std::runtime_error("Uji faktor daya gagal");
    if (!result.thdDefined || result.thdPercent > 0.5) throw std::runtime_error("Uji THD sinus gagal");

    AnalysisResult zero;
    zero.voltageRms = 0.0;
    const DisturbanceReport diagnosis = DisturbanceDetector::evaluate(zero, config);
    if (diagnosis.conditions.empty() || diagnosis.conditions[0] != "INTERRUPTION") throw std::runtime_error("Uji interruption gagal");
    std::cout << "Semua self-test lulus (Vrms, frekuensi, faktor daya, THD sinus, interruption).\n";
}
