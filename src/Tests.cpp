#include "../include/Tests.h"
#include "../include/DataLoader.h"
#include "../include/SignalAnalysis.h"
#include "../include/PowerAnalysis.h"
#include "../include/DisturbanceDetector.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

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

    const auto testCsv = [](const std::string& name, const std::string& contents, bool shouldLoad) {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        const std::filesystem::path path = std::filesystem::temp_directory_path() /
            ("pqa_validation_" + name + "_" + std::to_string(suffix) + ".csv");
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            if (!output) throw std::runtime_error("Tidak dapat membuat file sementara untuk uji validasi CSV.");
            output << contents;
        }
        bool loaded = false;
        try {
            (void)DataLoader::loadCsv(path);
            loaded = true;
        } catch (const std::exception&) {
        }
        std::error_code removeError;
        std::filesystem::remove(path, removeError);
        if (loaded != shouldLoad) throw std::runtime_error("Uji validasi CSV gagal: " + name);
    };

    testCsv("valid_header", "waktu,tegangan,arus\n0,0,1\n0.0002,10,2\n", true);
    testCsv("valid_quoted_header", "\"Time (s)\",\"Voltage (V)\",\"Current (A)\"\n0,0,1\n0.0002,10,2\n", true);
    testCsv("malformed_header", "foo,bar,baz\n0,0,1\n0.0002,10,2\n", false);
    testCsv("malformed_first_data", "bad,0,1\n0.0002,10,2\n", false);
    testCsv("missing_column", "0,0\n0.0002,10\n", false);
    testCsv("wrong_column_count", "0,0,1,extra\n0.0002,10,2,extra\n", false);
    testCsv("non_finite", "0,0,1\n0.0002,NaN,2\n", false);
    testCsv("hex_number", "0,0,1\n0.0002,0x1p4,2\n", false);
    testCsv("duplicate_time", "0,0,1\n0,10,2\n", false);
    testCsv("non_uniform_time", "0,0,1\n0.0002,10,2\n0.0005,20,3\n", false);
    testCsv("non_uniform_tiny_interval", "0,0,1\n1e-15,10,2\n5e-13,20,3\n", false);
    testCsv("empty_file", "", false);
    testCsv("overflow_interval", "-1e308,0,1\n1e308,10,2\n", false);

    std::cout << "Semua self-test lulus (perhitungan dasar dan validasi CSV).\n";
}
