#pragma once

#include "../Model.h"

class SignalAnalysis {
public:
    static void calculateVoltageCurrent(const MeasurementDataset&, AnalysisResult&);
    static void calculateFrequency(const MeasurementDataset&, AnalysisResult&);
    static void calculateVoltageThd(const MeasurementDataset&, const Config&, AnalysisResult&);
};
