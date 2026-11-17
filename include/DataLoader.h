#pragma once

#include "../Model.h"
#include <filesystem>
#include <string>

class DataLoader {
public:
    static MeasurementDataset loadCsv(const std::filesystem::path& path);
};
