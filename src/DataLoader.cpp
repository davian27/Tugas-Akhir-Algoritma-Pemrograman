#include "../include/DataLoader.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool parseNumber(const std::string& text, double& value) {
    const std::string cleaned = trim(text);
    if (cleaned.empty()) return false;
    std::size_t index = 0;
    if (cleaned[index] == '+' || cleaned[index] == '-') ++index;
    std::size_t digits = 0;
    while (index < cleaned.size() && std::isdigit(static_cast<unsigned char>(cleaned[index]))) {
        ++index;
        ++digits;
    }
    if (index < cleaned.size() && cleaned[index] == '.') {
        ++index;
        while (index < cleaned.size() && std::isdigit(static_cast<unsigned char>(cleaned[index]))) {
            ++index;
            ++digits;
        }
    }
    if (digits == 0) return false;
    if (index < cleaned.size() && (cleaned[index] == 'e' || cleaned[index] == 'E')) {
        ++index;
        if (index < cleaned.size() && (cleaned[index] == '+' || cleaned[index] == '-')) ++index;
        const std::size_t exponentStart = index;
        while (index < cleaned.size() && std::isdigit(static_cast<unsigned char>(cleaned[index]))) ++index;
        if (index == exponentStart) return false;
    }
    if (index != cleaned.size()) return false;
    char* end = nullptr;
    value = std::strtod(cleaned.c_str(), &end);
    return end == cleaned.c_str() + cleaned.size() && std::isfinite(value);
}

std::string normalizeHeaderCell(std::string value) {
    value = trim(value);
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
        value = value.substr(1, value.size() - 2);
    std::string normalized;
    for (unsigned char character : value) {
        if (std::isspace(character) || character == '_' || character == '(' || character == ')' || character == '/') continue;
        normalized.push_back(static_cast<char>(std::tolower(character)));
    }
    return normalized;
}

bool isKnownHeader(const std::string (&fields)[4]) {
    const std::string time = normalizeHeaderCell(fields[0]);
    const std::string voltage = normalizeHeaderCell(fields[1]);
    const std::string current = normalizeHeaderCell(fields[2]);
    const bool timeColumn = time == "waktu" || time == "time" || time == "timestamp" ||
                            time == "times" || time == "timesec" || time == "timestampsec";
    const bool voltageColumn = voltage == "tegangan" || voltage == "voltage" || voltage == "v" ||
                               voltage == "voltagev" || voltage == "teganganv";
    const bool currentColumn = current == "arus" || current == "current" || current == "i" ||
                               current == "currenta" || current == "arusa";
    return timeColumn && voltageColumn && currentColumn;
}
}

MeasurementDataset DataLoader::loadCsv(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Tidak dapat membuka file: " + path.u8string());

    MeasurementDataset dataset;
    dataset.sourcePath = path.u8string();
    std::string line;
    std::size_t lineNumber = 0;
    bool firstNonEmpty = true;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (trim(line).empty()) continue;
        const bool isFirstRecord = firstNonEmpty;
        if (firstNonEmpty && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) line.erase(0, 3);
        firstNonEmpty = false;

        std::stringstream row(line);
        std::string fields[4];
        int count = 0;
        while (count < 4 && std::getline(row, fields[count], ',')) ++count;
        const std::size_t firstComma = line.find(',');
        const std::size_t secondComma = firstComma == std::string::npos
            ? std::string::npos : line.find(',', firstComma + 1);
        const std::size_t extraComma = secondComma == std::string::npos
            ? std::string::npos : line.find(',', secondComma + 1);
        if (count != 3 || firstComma == std::string::npos ||
            secondComma == std::string::npos || extraComma != std::string::npos) {
            throw std::runtime_error("Baris " + std::to_string(lineNumber) + ": harus memiliki tepat 3 kolom dipisahkan koma.");
        }

        Sample sample;
        if (!parseNumber(fields[0], sample.timeSeconds) ||
            !parseNumber(fields[1], sample.voltage) ||
            !parseNumber(fields[2], sample.current)) {
            if (dataset.samples.empty() && isFirstRecord && isKnownHeader(fields)) continue;
            throw std::runtime_error("Baris " + std::to_string(lineNumber) + ": nilai waktu/tegangan/arus harus numerik dan finite.");
        }
        if (!dataset.samples.empty() && sample.timeSeconds <= dataset.samples.back().timeSeconds) {
            throw std::runtime_error("Baris " + std::to_string(lineNumber) + ": waktu harus meningkat dan tidak boleh duplikat.");
        }
        dataset.samples.push_back(sample);
    }
    if (dataset.samples.size() < 2) throw std::runtime_error("CSV harus berisi minimal 2 sampel numerik.");

    const double firstInterval = dataset.samples[1].timeSeconds - dataset.samples[0].timeSeconds;
    if (!std::isfinite(firstInterval) || firstInterval <= 0.0)
        throw std::runtime_error("Interval waktu pertama tidak valid atau terlalu besar untuk dihitung.");
    double sumIntervals = 0.0;
    for (std::size_t i = 1; i < dataset.samples.size(); ++i) {
        const double interval = dataset.samples[i].timeSeconds - dataset.samples[i - 1].timeSeconds;
        if (!std::isfinite(interval) || interval <= 0.0)
            throw std::runtime_error("Interval waktu antar-sampel tidak valid.");
        sumIntervals += interval;
        if (!std::isfinite(sumIntervals))
            throw std::runtime_error("Rentang waktu terlalu besar untuk dihitung dengan aman.");
        const double timeScale = std::max(std::fabs(dataset.samples[i].timeSeconds),
                                          std::fabs(dataset.samples[i - 1].timeSeconds));
        const double tolerance = std::max(firstInterval * 0.001,
            std::numeric_limits<double>::epsilon() * timeScale * 8.0);
        if (std::fabs(interval - firstInterval) > tolerance) {
            throw std::runtime_error("Interval waktu tidak seragam (toleransi 0,1%); analisis frekuensi/THD memerlukan sampling seragam.");
        }
    }
    dataset.sampleRateHz = static_cast<double>(dataset.samples.size() - 1) / sumIntervals;
    if (!std::isfinite(dataset.sampleRateHz) || dataset.sampleRateHz <= 0.0)
        throw std::runtime_error("Frekuensi sampling tidak valid; periksa jarak waktu pada CSV.");
    return dataset;
}
