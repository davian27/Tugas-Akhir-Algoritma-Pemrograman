#pragma once

#include <string>
#include <cstddef>
#include <vector>

struct Sample {
    double timeSeconds = 0.0;
    double voltage = 0.0;
    double current = 0.0;
};

struct MeasurementDataset {
    std::string sourcePath;
    std::vector<Sample> samples;
    double sampleRateHz = 0.0;
};

struct Config {
    double nominalVoltageVrms = 220.0;
    double nominalFrequencyHz = 50.0;
    double sagLowerFraction = 0.10;
    double sagUpperFraction = 0.90;
    double interruptionFraction = 0.10;
    double swellFraction = 1.10;
    double thdLimitPercent = 5.0;
    int maxHarmonicOrder = 10;
};

struct AnalysisResult {
    std::size_t sampleCount = 0;
    double voltageMax = 0.0;
    double voltageMin = 0.0;
    double voltagePeakToPeak = 0.0;
    double voltageRms = 0.0;
    double currentRms = 0.0;
    double activePowerW = 0.0;
    double apparentPowerVa = 0.0;
    bool powerFactorDefined = false;
    double powerFactor = 0.0;
    bool frequencyDefined = false;
    double frequencyHz = 0.0;
    bool thdDefined = false;
    double thdPercent = 0.0;
    std::string thdNote;
};

struct DisturbanceReport {
    std::vector<std::string> conditions;
};
