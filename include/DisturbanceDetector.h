#pragma once

#include "../Model.h"

class DisturbanceDetector {
public:
    static DisturbanceReport evaluate(const AnalysisResult&, const Config&);
};
