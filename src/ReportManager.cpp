#include "../include/ReportManager.h"

#include <ctime>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
std::string formatNumber(double value, int decimals = 2) {
    if (std::fabs(value) < 0.5 * std::pow(10.0, -decimals)) value = 0.0;
    std::ostringstream formatted;
    formatted.imbue(std::locale::classic());
    formatted << std::fixed << std::setprecision(decimals) << value;
    std::string result = formatted.str();
    const std::size_t point = result.find('.');
    if (point != std::string::npos) {
        while (!result.empty() && result.back() == '0') result.pop_back();
        if (!result.empty() && result.back() == '.') result.pop_back();
    }
    if (result == "-0" || result.empty()) return "0";
    return result;
}

void writeReport(std::ostream& out, const MeasurementDataset& data, const AnalysisResult& r,
                 const Config& c, const DisturbanceReport& d) {
    out << "POWER QUALITY ANALYZER\n" << std::string(40, '-') << "\n"
        << "File              : " << data.sourcePath << "\n"
        << "Jumlah sampel     : " << r.sampleCount << "\n"
        << "Sampling          : " << formatNumber(data.sampleRateHz) << " Hz (seragam, toleransi 0,1%)\n\n"
        << "TEGANGAN DAN ARUS\n"
        << "Vmax              : " << formatNumber(r.voltageMax) << " V\n"
        << "Vmin              : " << formatNumber(r.voltageMin) << " V\n"
        << "Vpp               : " << formatNumber(r.voltagePeakToPeak) << " V\n"
        << "Vrms              : " << formatNumber(r.voltageRms) << " V\n"
        << "Irms              : " << formatNumber(r.currentRms) << " A\n"
        << "Frekuensi         : ";
    if (r.frequencyDefined) out << formatNumber(r.frequencyHz) << " Hz\n"; else out << "tidak tersedia\n";
    out << "Daya aktif        : " << formatNumber(r.activePowerW) << " W\n"
        << "Daya semu         : " << formatNumber(r.apparentPowerVa) << " VA\n"
        << "Faktor daya       : ";
    if (r.powerFactorDefined) out << formatNumber(r.powerFactor, 3) << "\n"; else out << "tidak terdefinisi\n";
    out << "THD tegangan      : ";
    if (r.thdDefined) out << formatNumber(r.thdPercent) << " %\n"; else out << "tidak tersedia\n";
    if (!r.thdNote.empty()) out << "Catatan THD       : " << r.thdNote << "\n";
    out << "\nDIAGNOSIS\n";
    for (const auto& condition : d.conditions) out << "- " << condition << "\n";
    out << "\nKonfigurasi       : Vnom=" << formatNumber(c.nominalVoltageVrms) << " V, fnom=" << formatNumber(c.nominalFrequencyHz)
        << " Hz, batas THD=" << formatNumber(c.thdLimitPercent) << "%\n"
        << "Metode            : agregat seluruh file; bukan deteksi durasi kejadian.\n"
        << std::string(40, '-') << "\n";
}

std::string pdfText(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (unsigned char c : value) {
        if (c == '\n' || c == '\r' || c == '\t') result += ' ';
        else if (c >= 32 && c <= 126) {
            if (c == '(' || c == ')' || c == '\\') result += '\\';
            result += static_cast<char>(c);
        } else if ((c & 0xC0) != 0x80) result += '?';
    }
    return result;
}

void addPdfLine(std::ostringstream& content, const std::string& value, double x, double y,
                int fontSize = 10, const char* font = "F1",
                const char* color = "0.12 0.18 0.24") {
    content << color << " rg\nBT /" << font << ' ' << fontSize << " Tf " << x << ' ' << y
            << " Td (" << pdfText(value) << ") Tj ET\n";
}

std::string makePdf(const MeasurementDataset& data, const AnalysisResult& r,
                    const Config& config, const DisturbanceReport& report) {
    std::ostringstream body;
    body << "q 0.09 0.16 0.25 rg 0 790 595 52 re f Q\n"
         << "q 0.08 0.78 0.83 rg 42 786 3 38 re f Q\n";
    addPdfLine(body, "POWER QUALITY ANALYZER", 56, 809, 17, "F2", "1 1 1");
    addPdfLine(body, "Laporan analisis kualitas daya", 56, 768, 9, "F1", "0.30 0.39 0.48");
    std::string sourcePath = pdfText(data.sourcePath);
    if (sourcePath.size() > 84) sourcePath = "..." + sourcePath.substr(sourcePath.size() - 81);
    addPdfLine(body, "File: " + sourcePath, 42, 742, 9);
    addPdfLine(body, "Jumlah sampel: " + std::to_string(r.sampleCount) +
              "    Sampling: " + formatNumber(data.sampleRateHz) + " Hz", 42, 725, 9);

    auto section = [&](const std::string& title, double y) {
        body << "q 0.07 0.15 0.24 rg 42 " << y - 7 << " 511 19 re f Q\n";
        addPdfLine(body, title, 52, y, 9, "F2", "1 1 1");
    };
    auto row = [&](const std::string& label, const std::string& value, double y) {
        addPdfLine(body, label, 54, y, 9);
        addPdfLine(body, value, 330, y, 9, "F2");
        body << "q 0.82 0.85 0.88 RG 42 " << y - 5 << " m 553 " << y - 5 << " l S Q\n";
    };

    section("TEGANGAN DAN ARUS", 697);
    row("Vmax", formatNumber(r.voltageMax) + " V", 676);
    row("Vmin", formatNumber(r.voltageMin) + " V", 658);
    row("Vpp", formatNumber(r.voltagePeakToPeak) + " V", 640);
    row("Vrms", formatNumber(r.voltageRms) + " V", 622);
    row("Irms", formatNumber(r.currentRms) + " A", 604);
    row("Frekuensi", r.frequencyDefined ? formatNumber(r.frequencyHz) + " Hz" : "tidak tersedia", 586);

    section("DAYA", 552);
    row("Daya aktif", formatNumber(r.activePowerW) + " W", 531);
    row("Daya semu", formatNumber(r.apparentPowerVa) + " VA", 513);
    row("Faktor daya", r.powerFactorDefined ? formatNumber(r.powerFactor, 3) : "tidak terdefinisi", 495);
    row("THD tegangan", r.thdDefined ? formatNumber(r.thdPercent) + " %" : "tidak tersedia", 477);
    if (!r.thdDefined && !r.thdNote.empty()) {
        std::string note = pdfText(r.thdNote);
        if (note.size() > 48) note.resize(45), note += "...";
        row("Catatan THD", note, 459);
    }

    section("DIAGNOSIS", 425);
    double y = 404;
    for (const auto& condition : report.conditions) {
        addPdfLine(body, "- " + condition, 54, y, 9, "F1", "0.12 0.18 0.24");
        y -= 17;
    }
    section("KONFIGURASI DAN METODE", y - 8);
    y -= 30;
    addPdfLine(body, "Vnom: " + formatNumber(config.nominalVoltageVrms) + " Vrms    fnom: " +
              formatNumber(config.nominalFrequencyHz) + " Hz    Batas THD: " +
              formatNumber(config.thdLimitPercent) + "%", 54, y, 8);
    y -= 16;
    addPdfLine(body, "Analisis memakai agregat seluruh file; laporan ini bukan sertifikasi standar.", 54, y, 8);
    body << "q 0.08 0.78 0.83 RG 42 35 m 553 35 l S Q\n";
    addPdfLine(body, "Power Quality Analyzer  |  Laporan dibuat dari data CSV yang dipilih", 42, 22, 7,
               "F1", "0.35 0.42 0.49");

    const std::string stream = body.str();
    std::vector<std::string> objects(6);
    objects[1] = "<< /Type /Catalog /Pages 2 0 R >>";
    objects[2] = "<< /Type /Pages /Kids [3 0 R] /Count 1 >>";
    objects[3] = "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Resources << /Font << /F1 4 0 R /F2 5 0 R >> >> /Contents 6 0 R >>";
    objects[4] = "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>";
    objects[5] = "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>";
    objects.push_back("<< /Length " + std::to_string(stream.size()) + " >>\nstream\n" + stream + "endstream");

    std::ostringstream pdf;
    pdf << "%PDF-1.4\n%PQA\n";
    std::vector<std::size_t> offsets(objects.size(), 0);
    for (std::size_t i = 1; i < objects.size(); ++i) {
        offsets[i] = static_cast<std::size_t>(pdf.tellp());
        pdf << i << " 0 obj\n" << objects[i] << "\nendobj\n";
    }
    const std::size_t xrefOffset = static_cast<std::size_t>(pdf.tellp());
    pdf << "xref\n0 " << objects.size() << "\n0000000000 65535 f \n";
    for (std::size_t i = 1; i < objects.size(); ++i) {
        pdf << std::setw(10) << std::setfill('0') << offsets[i] << " 00000 n \n";
    }
    pdf << "trailer\n<< /Size " << objects.size() << " /Root 1 0 R >>\nstartxref\n"
        << xrefOffset << "\n%%EOF\n";
    return pdf.str();
}
}

void ReportManager::print(const MeasurementDataset& data, const AnalysisResult& result,
                          const Config& config, const DisturbanceReport& report) {
    writeReport(std::cout, data, result, config, report);
}

std::string ReportManager::appendLog(const MeasurementDataset& data, const AnalysisResult& result,
                                     const Config& config, const DisturbanceReport& report) {
    std::filesystem::create_directories("output");
    std::ofstream out("output/analysis.log", std::ios::app);
    if (!out) throw std::runtime_error("Tidak dapat menulis output/analysis.log");
    const std::time_t now = std::time(nullptr);
    out << "\nWaktu analisis    : " << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << "\n";
    writeReport(out, data, result, config, report);
    return "output/analysis.log";
}

void ReportManager::exportPdf(const MeasurementDataset& data, const AnalysisResult& result,
                              const Config& config, const DisturbanceReport& report,
                              const std::filesystem::path& outputPath) {
    if (outputPath.empty()) throw std::runtime_error("Lokasi file PDF belum ditentukan.");
    const std::string pdf = makePdf(data, result, config, report);
    std::ofstream out(outputPath, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Tidak dapat membuat file PDF pada lokasi yang dipilih.");
    out.write(pdf.data(), static_cast<std::streamsize>(pdf.size()));
    if (!out) throw std::runtime_error("Gagal menulis seluruh isi file PDF.");
}
