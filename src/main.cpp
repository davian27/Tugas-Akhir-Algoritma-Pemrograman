#include "../include/Config.h"
#include "../include/DataLoader.h"
#include "../include/DisturbanceDetector.h"
#include "../include/PowerAnalysis.h"
#include "../include/ReportManager.h"
#include "../include/SignalAnalysis.h"
#include "../include/Tests.h"

#include <iostream>
#include <string>

int main() {
    Config config;
    while (true) {
        std::cout << "\nPOWER QUALITY ANALYZER\n"
                  << "1) Analisis CSV\n2) Pengujian otomatis\n3) Pengaturan\n4) Keluar\nPilihan: ";
        std::string choice;
        if (!std::getline(std::cin, choice)) break;
        if (choice == "1") {
            std::cout << "Path file CSV: ";
            std::string path;
            std::getline(std::cin, path);
            try {
                MeasurementDataset data = DataLoader::loadCsv(path);
                AnalysisResult result;
                SignalAnalysis::calculateVoltageCurrent(data, result);
                SignalAnalysis::calculateFrequency(data, result);
                PowerAnalysis::calculate(data, result);
                SignalAnalysis::calculateVoltageThd(data, config, result);
                DisturbanceReport report = DisturbanceDetector::evaluate(result, config);
                ReportManager::print(data, result, config, report);
                std::cout << "Simpan hasil ke log? (y/N): ";
                std::string save;
                std::getline(std::cin, save);
                if (save == "y" || save == "Y") std::cout << "Tersimpan di " << ReportManager::appendLog(data, result, config, report) << "\n";
            } catch (const std::exception& error) { std::cout << "Gagal menganalisis: " << error.what() << "\n"; }
        } else if (choice == "2") {
            try { SelfTests::run(); }
            catch (const std::exception& error) { std::cout << "Self-test gagal: " << error.what() << "\n"; }
        } else if (choice == "3") ConfigManager::edit(config);
        else if (choice == "4") break;
        else std::cout << "Pilihan tidak dikenal.\n";
    }
    return 0;
}
