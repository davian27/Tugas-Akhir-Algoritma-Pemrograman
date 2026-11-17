#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>

#include "../include/DataLoader.h"
#include "../include/AnalysisDatabase.h"
#include "../include/DisturbanceDetector.h"
#include "../include/PowerAnalysis.h"
#include "../include/ReportManager.h"
#include "../include/SignalAnalysis.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {
const wchar_t* WINDOW_CLASS = L"PowerQualityAnalyzerDashboard";
const int SIDEBAR_WIDTH = 220;
const COLORREF BG = RGB(12, 18, 28);
const COLORREF SIDEBAR = RGB(14, 22, 34);
const COLORREF CARD = RGB(20, 29, 42);
const COLORREF CARD_ALT = RGB(28, 39, 54);
const COLORREF CYAN = RGB(56, 180, 198);
const COLORREF TEXT_MAIN = RGB(226, 232, 240);
const COLORREF TEXT_MUTED = RGB(145, 157, 173);
const COLORREF GRID = RGB(39, 50, 65);
const COLORREF GREEN = RGB(89, 177, 137);
const COLORREF YELLOW = RGB(215, 166, 84);
const COLORREF RED = RGB(211, 112, 112);

enum Page { PAGE_DASHBOARD, PAGE_ANALYSIS, PAGE_HISTORY };
struct HistoryEntry {
    std::int64_t id = 0;
    std::wstring fileName;
    std::wstring timestamp;
    AnalysisResult result;
    std::wstring status;
};

HWND pathInput = nullptr;
HFONT fontRegular = nullptr, fontMedium = nullptr, fontBold = nullptr;
Page activePage = PAGE_DASHBOARD;
bool hasResult = false;
MeasurementDataset lastDataset;
AnalysisResult lastResult;
Config appConfig;
DisturbanceReport lastReport;
std::vector<HistoryEntry> history;
std::vector<std::pair<RECT, std::int64_t>> historyRowRects;
bool databaseReady = false;
RECT browseRect{}, analyzeRect{}, saveRect{}, exportRect{}, dashboardNavRect{}, analysisNavRect{}, historyNavRect{};
int clientWidth = 0, clientHeight = 0;
int uploadBottomY = 252, metricsTopY = 266, bodyTopY = 386, bodyBottomY = 0;

void navigateTo(Page page);

void updateLayoutMetrics() {
    const bool compact = clientHeight < 810;
    uploadBottomY = compact ? 220 : 252;
    metricsTopY = uploadBottomY + 14;
    bodyTopY = metricsTopY + 120;
    bodyBottomY = clientHeight - 22;
}

std::wstring widen(const std::string& utf8) {
    if (utf8.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (size <= 0) return L"(teks tidak dapat ditampilkan)";
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], size);
    result.resize(static_cast<std::size_t>(size - 1));
    return result;
}

void refreshHistory() {
    std::vector<StoredAnalysis> records = AnalysisDatabase::loadRecent(100);
    std::reverse(records.begin(), records.end());
    history.clear();
    history.reserve(records.size());
    for (const StoredAnalysis& record : records) {
        const std::wstring fullPath = widen(record.sourcePath);
        const std::size_t slash = fullPath.find_last_of(L"\\/");
        const std::wstring fileName = slash == std::wstring::npos ? fullPath : fullPath.substr(slash + 1);
        history.push_back(HistoryEntry{record.id, fileName, widen(record.analyzedAt), record.result,
                                       widen(record.primaryStatus)});
    }
}

void addCurrentResultToSessionHistory(std::int64_t id) {
    const std::wstring fullPath = widen(lastDataset.sourcePath);
    const std::size_t slash = fullPath.find_last_of(L"\\/");
    const std::wstring fileName = slash == std::wstring::npos ? fullPath : fullPath.substr(slash + 1);
    const std::wstring status = lastReport.conditions.empty() ? L"Tidak diketahui" : widen(lastReport.conditions.front());
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    std::wostringstream timestamp;
    timestamp << std::put_time(&local, L"%Y-%m-%d %H:%M:%S");
    history.push_back(HistoryEntry{id, fileName, timestamp.str(), lastResult, status});
    if (history.size() > 100) history.erase(history.begin());
}

void openSavedAnalysis(HWND window, std::int64_t id) {
    if (id <= 0) {
        MessageBoxW(window, L"Analisis ini hanya tersedia selama sesi saat database tidak dapat digunakan.",
                    L"Riwayat tidak tersimpan", MB_OK | MB_ICONINFORMATION);
        return;
    }
    try {
        MeasurementDataset dataset;
        AnalysisResult result;
        Config config;
        DisturbanceReport report;
        AnalysisDatabase::load(id, dataset, result, config, report);
        lastDataset = std::move(dataset);
        lastResult = result;
        appConfig = config;
        lastReport = std::move(report);
        hasResult = true;
        if (pathInput) SetWindowTextW(pathInput, widen(lastDataset.sourcePath).c_str());
        navigateTo(PAGE_ANALYSIS);
        InvalidateRect(window, nullptr, FALSE);
    } catch (const std::exception& error) {
        MessageBoxW(window, widen(error.what()).c_str(), L"Tidak dapat membuka riwayat", MB_OK | MB_ICONERROR);
    }
}

std::wstring number(double value, int decimals = 2) {
    std::wostringstream out;
    out << std::fixed << std::setprecision(decimals) << value;
    return out.str();
}

void fill(HDC dc, const RECT& rect, COLORREF color, int radius = 12) {
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ old = SelectObject(dc, brush);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    radius = std::min(radius, 6);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, oldPen); SelectObject(dc, old); DeleteObject(pen); DeleteObject(brush);
}

void text(HDC dc, int x, int y, const std::wstring& value, COLORREF color = TEXT_MAIN,
          int size = 14, bool bold = false, int maxWidth = 0, UINT flags = DT_LEFT | DT_VCENTER | DT_SINGLELINE) {
    HFONT font = bold ? fontBold : (size <= 12 ? fontRegular : fontMedium);
    HGDIOBJ old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    int right = maxWidth > 0 ? x + maxWidth : clientWidth - 16;
    if (clientWidth > 0) right = std::min(right, clientWidth - 12);
    RECT rect{x, y, std::max(x + 1, right), y + size + 10};
    if ((flags & DT_SINGLELINE) != 0) flags |= DT_END_ELLIPSIS;
    DrawTextW(dc, value.c_str(), static_cast<int>(value.size()), &rect, flags);
    SelectObject(dc, old);
}

void wrappedText(HDC dc, const RECT& rect, const std::wstring& value, COLORREF color, int size = 10) {
    HFONT font = CreateFontW(-size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HGDIOBJ old = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    RECT bounds = rect;
    DrawTextW(dc, value.c_str(), static_cast<int>(value.size()), &bounds, DT_LEFT | DT_TOP | DT_WORDBREAK);
    SelectObject(dc, old);
    DeleteObject(font);
}

void line(HDC dc, int x1, int y1, int x2, int y2, COLORREF color, int thickness = 1) {
    HPEN pen = CreatePen(PS_SOLID, thickness, color);
    HGDIOBJ old = SelectObject(dc, pen);
    MoveToEx(dc, x1, y1, nullptr); LineTo(dc, x2, y2);
    SelectObject(dc, old); DeleteObject(pen);
}

RECT makeRect(int left, int top, int right, int bottom) { return RECT{left, top, right, bottom}; }
bool hit(const RECT& rect, POINT p) { return PtInRect(&rect, p) != FALSE; }

void drawButton(HDC dc, const RECT& rect, const std::wstring& label, bool primary, bool enabled = true) {
    fill(dc, rect, !enabled ? RGB(37, 45, 57) : (primary ? CYAN : CARD_ALT), 5);
    text(dc, rect.left + 10, rect.top + 3, label, !enabled ? TEXT_MUTED : (primary ? BG : TEXT_MAIN),
         13, true, rect.right - rect.left - 20, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void layoutPathInput() {
    if (!pathInput) return;
    int left = SIDEBAR_WIDTH + 24;
    int right = clientWidth - 24;
    int browseWidth = 126;
    int analyzeWidth = 110;
    int saveWidth = 148;
    int exportWidth = 112;
    int gap = 10;
    int available = right - left - 16 - browseWidth - analyzeWidth - saveWidth - exportWidth - gap * 4;
    int inputWidth = std::max(150, available);
    int controlY = clientHeight < 810 ? 146 : 208;
    MoveWindow(pathInput, left + 16, controlY, inputWidth, 34, TRUE);
    browseRect = makeRect(left + 16 + inputWidth + gap, controlY,
                         left + 16 + inputWidth + gap + browseWidth, controlY + 34);
    analyzeRect = makeRect(browseRect.right + gap, controlY,
                          browseRect.right + gap + analyzeWidth, controlY + 34);
    saveRect = makeRect(analyzeRect.right + gap, controlY,
                       analyzeRect.right + gap + saveWidth, controlY + 34);
    exportRect = makeRect(saveRect.right + gap, controlY,
                         saveRect.right + gap + exportWidth, controlY + 34);
}

void drawSidebar(HDC dc) {
    RECT side = makeRect(0, 0, SIDEBAR_WIDTH, clientHeight);
    HBRUSH brush = CreateSolidBrush(SIDEBAR); FillRect(dc, &side, brush); DeleteObject(brush);
    text(dc, 23, 25, L"PQA", CYAN, 17, true);
    text(dc, 23, 49, L"POWER QUALITY ANALYZER", TEXT_MUTED, 9, true);
    line(dc, 20, 82, SIDEBAR_WIDTH - 20, 82, GRID);
    dashboardNavRect = makeRect(12, 104, SIDEBAR_WIDTH - 12, 148);
    analysisNavRect = makeRect(12, 158, SIDEBAR_WIDTH - 12, 202);
    historyNavRect = makeRect(12, 212, SIDEBAR_WIDTH - 12, 256);
    struct Nav { RECT rect; const wchar_t* label; Page page; } items[] = {
        {dashboardNavRect, L"Dashboard", PAGE_DASHBOARD},
        {analysisNavRect, L"Analisis", PAGE_ANALYSIS},
        {historyNavRect, L"Riwayat", PAGE_HISTORY}
    };
    for (const auto& item : items) {
        if (activePage == item.page) fill(dc, makeRect(item.rect.left, item.rect.top + 4, item.rect.right, item.rect.bottom - 4), CARD_ALT, 4);
        COLORREF color = activePage == item.page ? TEXT_MAIN : TEXT_MUTED;
        text(dc, item.rect.left + 14, item.rect.top + 5, item.label, color, 13, activePage == item.page);
        if (activePage == item.page) fill(dc, makeRect(item.rect.left, item.rect.top + 13, item.rect.left + 2, item.rect.bottom - 13), CYAN, 1);
    }
    text(dc, 22, clientHeight - 56, L"C++ Power Analyzer", TEXT_MUTED, 11);
    text(dc, 22, clientHeight - 37, L"Analisis berbasis CSV", TEXT_MUTED, 11);
}

void drawHeader(HDC dc) {
    const int titleX = SIDEBAR_WIDTH + 26;
    const int titleWidth = std::max(120, clientWidth - titleX - 214);
    text(dc, titleX, 22, activePage == PAGE_DASHBOARD ? L"Dashboard" :
         (activePage == PAGE_ANALYSIS ? L"Analisis Data" : L"Riwayat Analisis"), TEXT_MAIN, 23, true, titleWidth);
    text(dc, titleX, 53, activePage == PAGE_DASHBOARD ? L"Ringkasan kualitas daya dari data pengukuran Anda." :
         (activePage == PAGE_ANALYSIS ? L"Impor, analisis, dan simpan laporan pengukuran." : L"Daftar analisis tersimpan yang dapat dibuka kembali."), TEXT_MUTED, 12, false, titleWidth);
    fill(dc, makeRect(clientWidth - 177, 34, clientWidth - 169, 42), hasResult ? GREEN : TEXT_MUTED, 4);
    text(dc, clientWidth - 160, 28, hasResult ? L"Analisis tersedia" : L"Belum dianalisis", TEXT_MUTED, 11);
}

void drawUploadCard(HDC dc) {
    int left = SIDEBAR_WIDTH + 24, right = clientWidth - 24;
    RECT card = makeRect(left, 94, right, uploadBottomY);
    fill(dc, card, CARD);
    text(dc, card.left + 16, card.top + 12, L"DATA PENGUKURAN", TEXT_MUTED, 10, true);
    text(dc, card.left + 16, card.top + 30, L"CSV · waktu, tegangan, arus", TEXT_MAIN, 13, true);
    layoutPathInput();
    drawButton(dc, browseRect, L"Pilih CSV...", false);
    drawButton(dc, analyzeRect, L"Analisis", true);
    drawButton(dc, saveRect, L"Simpan laporan", false, hasResult);
    drawButton(dc, exportRect, L"Ekspor PDF", false, hasResult);
    text(dc, card.left + 16, card.bottom - 29, hasResult ? L"Analisis berhasil. Hasil dan grafik menggunakan sampel dari file yang dipilih." : L"Belum ada hasil. Setelah memilih CSV, tekan Analisis.",
         hasResult ? GREEN : TEXT_MUTED, 10);
}

void drawMetricCard(HDC dc, const RECT& rect, const wchar_t* title, const std::wstring& value,
                    const wchar_t* unit, COLORREF accent) {
    fill(dc, rect, CARD, 5);
    text(dc, rect.left + 16, rect.top + 12, title, TEXT_MUTED, 10, true);
    text(dc, rect.left + 16, rect.top + 34, value, TEXT_MAIN, 22, true);
    text(dc, rect.left + 16, rect.bottom - 25, unit, accent, 10, false);
}

std::wstring metricValue(double value, bool available = true, int decimals = 2) {
    return available ? number(value, decimals) : L"—";
}

void drawMetrics(HDC dc, int top) {
    const int left = SIDEBAR_WIDTH + 24, right = clientWidth - 24, gap = 12;
    const int width = (right - left - gap * 3) / 4;
    const int bottom = top + 106;
    const wchar_t* titles[] = {L"TEGANGAN RMS", L"FREKUENSI", L"ARUS RMS", L"FAKTOR DAYA"};
    std::wstring values[] = {
        metricValue(lastResult.voltageRms, hasResult),
        metricValue(lastResult.frequencyHz, hasResult && lastResult.frequencyDefined),
        metricValue(lastResult.currentRms, hasResult),
        metricValue(lastResult.powerFactor, hasResult && lastResult.powerFactorDefined, 3)
    };
    const wchar_t* units[] = {L"Volt", L"Hz", L"Ampere", L"PF"};
    COLORREF colors[] = {CYAN, GREEN, CYAN, YELLOW};
    for (int i = 0; i < 4; ++i) {
        int x = left + i * (width + gap);
        drawMetricCard(dc, makeRect(x, top, x + width, bottom), titles[i], values[i], units[i], colors[i]);
    }
}

void drawWaveChart(HDC dc, const RECT& card) {
    fill(dc, card, CARD);
    text(dc, card.left + 18, card.top + 14, L"Bentuk gelombang", TEXT_MAIN, 14, true);
    text(dc, card.left + 18, card.top + 36, L"Tegangan (V) · arus (A)", TEXT_MUTED, 10);
    fill(dc, makeRect(card.right - 183, card.top + 18, card.right - 173, card.top + 28), CYAN, 3);
    text(dc, card.right - 166, card.top + 14, L"Tegangan", TEXT_MUTED, 10);
    fill(dc, makeRect(card.right - 98, card.top + 18, card.right - 88, card.top + 28), YELLOW, 3);
    text(dc, card.right - 81, card.top + 14, L"Arus", TEXT_MUTED, 10);
    RECT plot = makeRect(card.left + 54, card.top + 70, card.right - 44, card.bottom - 36);
    if (!hasResult || lastDataset.samples.size() < 2) {
        fill(dc, plot, RGB(12, 29, 51), 6);
        text(dc, plot.left + 20, (plot.top + plot.bottom) / 2 - 9, L"Pilih dan analisis CSV untuk melihat bentuk gelombang.", TEXT_MUTED, 12, false, plot.right - plot.left - 40);
        return;
    }
    for (int i = 0; i <= 4; ++i) {
        int y = plot.top + (plot.bottom - plot.top) * i / 4;
        line(dc, plot.left, y, plot.right, y, GRID, 1);
    }
    const double maxV = std::max(1e-9, std::max(std::fabs(lastResult.voltageMax), std::fabs(lastResult.voltageMin)) * 1.10);
    double maxI = 0.0;
    for (const auto& s : lastDataset.samples) maxI = std::max(maxI, std::fabs(s.current));
    maxI = std::max(1e-9, maxI * 1.10);
    const int centerY = (plot.top + plot.bottom) / 2;
    const int halfH = (plot.bottom - plot.top) / 2 - 2;
    const std::size_t count = lastDataset.samples.size();
    const std::size_t maxPoints = static_cast<std::size_t>(std::max(2, static_cast<int>(plot.right - plot.left)));
    const std::size_t stride = std::max<std::size_t>(1, (count + maxPoints - 1) / maxPoints);
    std::vector<POINT> voltage, current;
    voltage.reserve(maxPoints + 2); current.reserve(maxPoints + 2);
    for (std::size_t i = 0; i < count; i += stride) {
        const int x = plot.left + static_cast<int>((static_cast<double>(i) / (count - 1)) * (plot.right - plot.left));
        const double v = lastDataset.samples[i].voltage;
        const double a = lastDataset.samples[i].current;
        voltage.push_back(POINT{x, centerY - static_cast<int>((v / maxV) * halfH)});
        current.push_back(POINT{x, centerY - static_cast<int>((a / maxI) * halfH)});
    }
    if ((count - 1) % stride != 0) {
        const auto& s = lastDataset.samples.back();
        const int x = plot.right;
        voltage.push_back(POINT{x, centerY - static_cast<int>((s.voltage / maxV) * halfH)});
        current.push_back(POINT{x, centerY - static_cast<int>((s.current / maxI) * halfH)});
    }
    HPEN vPen = CreatePen(PS_SOLID, 2, CYAN); HGDIOBJ old = SelectObject(dc, vPen);
    if (voltage.size() > 1) Polyline(dc, voltage.data(), static_cast<int>(voltage.size()));
    SelectObject(dc, old); DeleteObject(vPen);
    HPEN iPen = CreatePen(PS_SOLID, 2, YELLOW); old = SelectObject(dc, iPen);
    if (current.size() > 1) Polyline(dc, current.data(), static_cast<int>(current.size()));
    SelectObject(dc, old); DeleteObject(iPen);
    text(dc, card.left + 18, card.bottom - 27, L"Waktu: " + number(lastDataset.samples.front().timeSeconds, 4) + L" s", TEXT_MUTED, 10);
    text(dc, card.right - 100, card.bottom - 27, number(lastDataset.samples.back().timeSeconds, 4) + L" s", TEXT_MUTED, 10);
}

void drawDiagnosis(HDC dc, const RECT& card) {
    fill(dc, card, CARD);
    text(dc, card.left + 18, card.top + 14, L"Diagnosis Kualitas Daya", TEXT_MAIN, 15, true);
    if (!hasResult) {
        text(dc, card.left + 18, card.top + 58, L"Hasil diagnosis tampil setelah CSV dianalisis.", TEXT_MUTED, 12, false, card.right - card.left - 36, DT_LEFT | DT_WORDBREAK);
        return;
    }
    bool normal = !lastReport.conditions.empty() && lastReport.conditions[0].find("NORMAL") == 0;
    COLORREF stateColor = normal ? GREEN : YELLOW;
    if (!normal) {
        for (const auto& condition : lastReport.conditions)
            if (condition.find("INTERRUPTION") != std::string::npos) stateColor = RED;
    }
    const std::wstring headline = normal ? L"Normal" : widen(lastReport.conditions.front());
    text(dc, card.left + 18, card.top + 49, headline, stateColor, 16, true, card.right - card.left - 36);
    line(dc, card.left + 18, card.top + 78, card.right - 18, card.top + 78, GRID);
    int y = card.top + 88;
    for (const auto& condition : lastReport.conditions) {
        if (condition == "MULTIPLE DISTURBANCE" || condition.find("NORMAL") == 0) continue;
        fill(dc, makeRect(card.left + 19, y + 7, card.left + 26, y + 14), stateColor, 4);
        text(dc, card.left + 36, y, widen(condition), TEXT_MUTED, 11, false, card.right - card.left - 54);
        y += 27;
    }
    if (card.bottom - card.top >= 185)
        text(dc, card.left + 18, card.bottom - 25, L"Klasifikasi agregat seluruh file", TEXT_MUTED, 9);
}

void drawThd(HDC dc, const RECT& card) {
    fill(dc, card, CARD);
    text(dc, card.left + 18, card.top + 14, L"Distorsi harmonisa", TEXT_MAIN, 14, true);
    text(dc, card.left + 18, card.top + 43, L"THD tegangan", TEXT_MUTED, 10);
    const bool thdWarning = hasResult && lastResult.thdDefined && lastResult.thdPercent > appConfig.thdLimitPercent;
    if (hasResult && lastResult.thdDefined) {
        text(dc, card.left + 18, card.top + 62, number(lastResult.thdPercent) + L" %",
             thdWarning ? YELLOW : TEXT_MAIN, 23, true);
    } else {
        text(dc, card.left + 18, card.top + 66, hasResult ? L"Belum tersedia" : L"—",
             TEXT_MUTED, hasResult ? 14 : 23, true);
    }
    line(dc, card.left + 18, card.top + 105, card.right - 18, card.top + 105, GRID);
    text(dc, card.left + 18, card.top + 114, L"Ambang konfigurasi", TEXT_MUTED, 9);
    text(dc, card.right - 70, card.top + 111, number(appConfig.thdLimitPercent, 1) + L" %", TEXT_MAIN, 10, true);
    if (hasResult && !lastResult.thdDefined && !lastResult.thdNote.empty()) {
        std::string note = lastResult.thdNote;
        const std::size_t colon = note.find(": ");
        if (colon != std::string::npos) note.erase(0, colon + 2);
        wrappedText(dc, makeRect(card.left + 18, card.top + 137, card.right - 18, card.bottom - 5), widen(note), TEXT_MUTED, 9);
    }
}

void drawHistoryTable(HDC dc, const RECT& card, std::size_t maxRows) {
    fill(dc, card, CARD);
    text(dc, card.left + 18, card.top + 14, L"Riwayat Analisis", TEXT_MAIN, 15, true);
    const int headerY = card.top + 49;
    const int innerLeft = card.left + 18;
    const int innerRight = card.right - 18;
    const int innerWidth = std::max(1, innerRight - innerLeft);
    const int fileX = innerLeft;
    const int rmsX = innerLeft + innerWidth * 48 / 100;
    const int frequencyX = innerLeft + innerWidth * 66 / 100;
    const int statusX = innerLeft + innerWidth * 84 / 100;
    line(dc, card.left + 16, headerY + 24, card.right - 16, headerY + 24, GRID);
    text(dc, fileX, headerY, L"FILE", TEXT_MUTED, 10, true, rmsX - fileX - 10);
    text(dc, rmsX, headerY, L"Vrms", TEXT_MUTED, 10, true, frequencyX - rmsX - 10);
    text(dc, frequencyX, headerY, L"Frekuensi", TEXT_MUTED, 10, true, statusX - frequencyX - 10);
    text(dc, statusX, headerY, L"Status", TEXT_MUTED, 10, true, innerRight - statusX);
    if (history.empty()) {
        text(dc, card.left + 18, headerY + 39, L"Belum ada riwayat. Jalankan analisis untuk menambahkan data.", TEXT_MUTED, 11, false, card.right - card.left - 36);
        return;
    }
    int y = headerY + 33;
    const std::size_t visibleRows = card.bottom > headerY + 43
        ? static_cast<std::size_t>((card.bottom - headerY - 43) / 27) : 0;
    std::size_t rows = std::min(std::min(maxRows, history.size()), visibleRows);
    for (std::size_t n = 0; n < rows; ++n) {
        const auto& item = history[history.size() - 1 - n];
        std::wstring name = item.fileName;
        if (name.size() > 34) name = L"..." + name.substr(name.size() - 31);
        text(dc, fileX, y, name, TEXT_MAIN, 11, false, rmsX - fileX - 10);
        text(dc, rmsX, y, number(item.result.voltageRms) + L" V", TEXT_MUTED, 11, false, frequencyX - rmsX - 10);
        text(dc, frequencyX, y,
             item.result.frequencyDefined ? number(item.result.frequencyHz) + L" Hz" : L"N/A", TEXT_MUTED, 11, false, statusX - frequencyX - 10);
        const bool isNormal = item.status.find(L"NORMAL") == 0;
        text(dc, statusX, y, item.status, isNormal ? GREEN : YELLOW, 10, true, innerRight - statusX);
        historyRowRects.emplace_back(makeRect(card.left + 8, y - 3, card.right - 8, y + 23), item.id);
        y += 27;
        if (n + 1 < rows) line(dc, card.left + 18, y - 5, card.right - 18, y - 5, GRID);
    }
}

void drawDashboard(HDC dc) {
    int left = SIDEBAR_WIDTH + 24, right = clientWidth - 24;
    drawUploadCard(dc);
    drawMetrics(dc, metricsTopY);
    int gap = 14;
    int available = right - left;
    int chartW = static_cast<int>((available - gap) * 0.64);
    const int lowerPanelHeight = 165;
    int chartBottom = std::max(bodyTopY + 130, bodyBottomY - lowerPanelHeight - gap);
    RECT chart = makeRect(left, bodyTopY, left + chartW, chartBottom);
    RECT diagnosis = makeRect(chart.right + gap, bodyTopY, right, chart.bottom);
    drawWaveChart(dc, chart);
    drawDiagnosis(dc, diagnosis);
    int bottomTop = chart.bottom + 14;
    RECT thd = makeRect(left, bottomTop, left + chartW, bodyBottomY);
    RECT hist = makeRect(thd.right + gap, bottomTop, right, bodyBottomY);
    drawThd(dc, thd);
    drawHistoryTable(dc, hist, 3);
}

void drawAnalysisPage(HDC dc) {
    int left = SIDEBAR_WIDTH + 24, right = clientWidth - 24;
    drawUploadCard(dc);
    drawMetrics(dc, metricsTopY);
    const int gap = 14;
    const int availableWidth = right - left;
    const int chartW = static_cast<int>((availableWidth - gap) * 0.64);
    const int top = bodyTopY;
    const int bottom = bodyBottomY;
    RECT chart = makeRect(left, top, left + chartW, bottom);
    RECT detail = makeRect(chart.right + gap, top, right, bottom);
    drawWaveChart(dc, chart);
    int half = (detail.bottom - detail.top - gap) / 2;
    RECT diagnosis = makeRect(detail.left, detail.top, detail.right, detail.top + half);
    RECT thd = makeRect(detail.left, diagnosis.bottom + gap, detail.right, detail.bottom);
    drawDiagnosis(dc, diagnosis);
    drawThd(dc, thd);
}

void drawHistoryPage(HDC dc) {
    int left = SIDEBAR_WIDTH + 24, right = clientWidth - 24;
    RECT card = makeRect(left, 94, right, clientHeight - 24);
    drawHistoryTable(dc, card, 100);
    if (!history.empty()) text(dc, left + 18, card.bottom - 29,
        L"Tersimpan lokal. Klik baris untuk membuka kembali analisis.", TEXT_MUTED, 10);
}

void paintDashboard(HWND window) {
    PAINTSTRUCT ps{};
    HDC screen = BeginPaint(window, &ps);
    historyRowRects.clear();
    RECT client{}; GetClientRect(window, &client);
    clientWidth = client.right; clientHeight = client.bottom;
    updateLayoutMetrics();
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, std::max(1, clientWidth), std::max(1, clientHeight));
    HGDIOBJ oldBitmap = SelectObject(memory, bitmap);
    HBRUSH background = CreateSolidBrush(BG); FillRect(memory, &client, background); DeleteObject(background);
    drawSidebar(memory);
    drawHeader(memory);
    if (activePage == PAGE_DASHBOARD) drawDashboard(memory);
    else if (activePage == PAGE_ANALYSIS) drawAnalysisPage(memory);
    else drawHistoryPage(memory);
    BitBlt(screen, 0, 0, clientWidth, clientHeight, memory, 0, 0, SRCCOPY);
    SelectObject(memory, oldBitmap); DeleteObject(bitmap); DeleteDC(memory);
    EndPaint(window, &ps);
}

void browseFile(HWND window) {
    wchar_t path[32768] = L"";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window;
    dialog.lpstrFilter = L"CSV files (*.csv)\0*.csv\0All files (*.*)\0*.*\0\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = 32768;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    dialog.lpstrTitle = L"Pilih file pengukuran CSV";
    if (GetOpenFileNameW(&dialog)) {
        SetWindowTextW(pathInput, path);
        InvalidateRect(window, nullptr, FALSE);
    }
}

void runAnalysis(HWND window) {
    int length = GetWindowTextLengthW(pathInput);
    if (length == 0) {
        MessageBoxW(window, L"Pilih file CSV terlebih dahulu.", L"File belum dipilih", MB_OK | MB_ICONINFORMATION);
        return;
    }
    std::wstring path(static_cast<std::size_t>(length) + 1, L'\0');
    GetWindowTextW(pathInput, &path[0], length + 1);
    path.resize(static_cast<std::size_t>(length));
    try {
        lastDataset = DataLoader::loadCsv(std::filesystem::path(path));
        lastResult = AnalysisResult{};
        SignalAnalysis::calculateVoltageCurrent(lastDataset, lastResult);
        SignalAnalysis::calculateFrequency(lastDataset, lastResult);
        PowerAnalysis::calculate(lastDataset, lastResult);
        SignalAnalysis::calculateVoltageThd(lastDataset, appConfig, lastResult);
        lastReport = DisturbanceDetector::evaluate(lastResult, appConfig);
        hasResult = true;
        if (databaseReady) {
            std::int64_t savedId = 0;
            try {
                savedId = AnalysisDatabase::save(lastDataset, lastResult, appConfig, lastReport);
                refreshHistory();
            } catch (const std::exception& databaseError) {
                addCurrentResultToSessionHistory(savedId);
                MessageBoxW(window,
                    (L"Analisis berhasil, tetapi riwayat tidak dapat disimpan ke database.\r\n" +
                     widen(databaseError.what())).c_str(),
                    L"Riwayat database gagal", MB_OK | MB_ICONWARNING);
            }
        } else {
            addCurrentResultToSessionHistory(0);
        }
        InvalidateRect(window, nullptr, FALSE);
    } catch (const std::exception& error) {
        hasResult = false;
        MessageBoxW(window, widen(error.what()).c_str(), L"Tidak dapat menganalisis CSV", MB_OK | MB_ICONERROR);
        InvalidateRect(window, nullptr, FALSE);
    }
}

void saveReport(HWND window) {
    if (!hasResult) return;
    try {
        const std::string location = ReportManager::appendLog(lastDataset, lastResult, appConfig, lastReport);
        MessageBoxW(window, (L"Laporan ditambahkan ke:\r\n" + widen(location)).c_str(), L"Laporan tersimpan", MB_OK | MB_ICONINFORMATION);
    } catch (const std::exception& error) {
        MessageBoxW(window, widen(error.what()).c_str(), L"Gagal menyimpan laporan", MB_OK | MB_ICONERROR);
    }
}

void exportPdf(HWND window) {
    if (!hasResult) return;
    wchar_t path[32768] = L"power_quality_report.pdf";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window;
    dialog.lpstrFilter = L"PDF document (*.pdf)\0*.pdf\0\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = 32768;
    dialog.lpstrDefExt = L"pdf";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    dialog.lpstrTitle = L"Simpan laporan Power Quality Analyzer";
    if (!GetSaveFileNameW(&dialog)) return;
    try {
        ReportManager::exportPdf(lastDataset, lastResult, appConfig, lastReport,
                                 std::filesystem::path(path));
        MessageBoxW(window, (L"Laporan PDF berhasil disimpan:\r\n" + std::wstring(path)).c_str(),
                    L"Ekspor PDF selesai", MB_OK | MB_ICONINFORMATION);
    } catch (const std::exception& error) {
        MessageBoxW(window, widen(error.what()).c_str(), L"Gagal mengekspor PDF", MB_OK | MB_ICONERROR);
    }
}

void resizePathInput() {
    if (pathInput) layoutPathInput();
}

void navigateTo(Page page) {
    activePage = page;
    if (pathInput) ShowWindow(pathInput, page == PAGE_HISTORY ? SW_HIDE : SW_SHOWNA);
}

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        fontRegular = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        fontMedium = CreateFontW(-14, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        fontBold = CreateFontW(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        pathInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 100, 30, window, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
        SendMessageW(pathInput, WM_SETFONT, reinterpret_cast<WPARAM>(fontRegular), TRUE);
        resizePathInput();
        try {
            AnalysisDatabase::initialize();
            refreshHistory();
            databaseReady = true;
        } catch (const std::exception& error) {
            databaseReady = false;
            MessageBoxW(window,
                (L"Database riwayat tidak dapat dibuka. Analisis tetap dapat digunakan, tetapi riwayat sesi ini tidak akan tersimpan.\r\n" +
                 widen(error.what())).c_str(),
                L"Database tidak tersedia", MB_OK | MB_ICONWARNING);
        }
        return 0;
    case WM_SIZE:
        clientWidth = LOWORD(lParam); clientHeight = HIWORD(lParam);
        updateLayoutMetrics();
        resizePathInput(); InvalidateRect(window, nullptr, FALSE); return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = 1000; info->ptMinTrackSize.y = 760;
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: paintDashboard(window); return 0;
    case WM_LBUTTONDOWN: {
        POINT p{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        if (hit(dashboardNavRect, p)) navigateTo(PAGE_DASHBOARD);
        else if (hit(analysisNavRect, p)) navigateTo(PAGE_ANALYSIS);
        else if (hit(historyNavRect, p)) navigateTo(PAGE_HISTORY);
        else if (activePage != PAGE_HISTORY && hit(browseRect, p)) { browseFile(window); return 0; }
        else if (activePage != PAGE_HISTORY && hit(analyzeRect, p)) { runAnalysis(window); return 0; }
        else if (activePage != PAGE_HISTORY && hit(saveRect, p)) { saveReport(window); return 0; }
        else if (activePage != PAGE_HISTORY && hit(exportRect, p)) { exportPdf(window); return 0; }
        else if (activePage == PAGE_HISTORY || activePage == PAGE_DASHBOARD) {
            for (auto row = historyRowRects.rbegin(); row != historyRowRects.rend(); ++row) {
                if (hit(row->first, p)) {
                    openSavedAnalysis(window, row->second);
                    return 0;
                }
            }
        }
        InvalidateRect(window, nullptr, FALSE); return 0;
    }
    case WM_DESTROY:
        if (fontRegular) DeleteObject(fontRegular);
        if (fontMedium) DeleteObject(fontMedium);
        if (fontBold) DeleteObject(fontBold);
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = WINDOW_CLASS;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    if (!RegisterClassW(&windowClass)) return 1;
    HWND window = CreateWindowExW(0, WINDOW_CLASS, L"Power Quality Analyzer",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 1360, 900,
        nullptr, nullptr, instance, nullptr);
    if (!window) return 1;
    ShowWindow(window, showCommand); UpdateWindow(window);
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message); DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
