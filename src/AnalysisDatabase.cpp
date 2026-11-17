#include "../include/AnalysisDatabase.h"

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include <sqlite3.h>

namespace {
const char* DATABASE_PATH = "output/analysis_history.sqlite3";

using DatabaseHandle = std::unique_ptr<sqlite3, decltype(&sqlite3_close)>;
using StatementHandle = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

[[noreturn]] void databaseError(sqlite3* database, const std::string& action) {
    throw std::runtime_error(action + ": " + (database ? sqlite3_errmsg(database) : "SQLite error"));
}

DatabaseHandle openDatabase() {
    sqlite3* rawDatabase = nullptr;
    const int result = sqlite3_open_v2(DATABASE_PATH, &rawDatabase,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
    DatabaseHandle database(rawDatabase, sqlite3_close);
    if (result != SQLITE_OK) databaseError(rawDatabase, "Tidak dapat membuka database riwayat");
    sqlite3_busy_timeout(rawDatabase, 5000);
    char* errorMessage = nullptr;
    const int pragmaResult = sqlite3_exec(rawDatabase, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errorMessage);
    if (pragmaResult != SQLITE_OK) {
        const std::string detail = errorMessage ? errorMessage : sqlite3_errmsg(rawDatabase);
        sqlite3_free(errorMessage);
        throw std::runtime_error("Tidak dapat mengaktifkan foreign key SQLite: " + detail);
    }
    return database;
}

void execute(sqlite3* database, const char* sql) {
    char* errorMessage = nullptr;
    if (sqlite3_exec(database, sql, nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        const std::string detail = errorMessage ? errorMessage : sqlite3_errmsg(database);
        sqlite3_free(errorMessage);
        throw std::runtime_error("Query database gagal: " + detail);
    }
}

StatementHandle prepare(sqlite3* database, const char* sql) {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK)
        databaseError(database, "Tidak dapat menyiapkan query database");
    return StatementHandle(statement, sqlite3_finalize);
}

void bindText(sqlite3* database, sqlite3_stmt* statement, int index, const std::string& value) {
    if (sqlite3_bind_text(statement, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
        databaseError(database, "Tidak dapat mengisi parameter teks database");
}

void bindDouble(sqlite3* database, sqlite3_stmt* statement, int index, double value) {
    if (sqlite3_bind_double(statement, index, value) != SQLITE_OK)
        databaseError(database, "Tidak dapat mengisi parameter numerik database");
}

void bindInt(sqlite3* database, sqlite3_stmt* statement, int index, int value) {
    if (sqlite3_bind_int(statement, index, value) != SQLITE_OK)
        databaseError(database, "Tidak dapat mengisi parameter integer database");
}

void bindInt64(sqlite3* database, sqlite3_stmt* statement, int index, sqlite3_int64 value) {
    if (sqlite3_bind_int64(statement, index, value) != SQLITE_OK)
        databaseError(database, "Tidak dapat mengisi parameter integer database");
}

void reset(sqlite3* database, sqlite3_stmt* statement) {
    sqlite3_reset(statement);
    if (sqlite3_clear_bindings(statement) != SQLITE_OK)
        databaseError(database, "Tidak dapat mengosongkan parameter query database");
}

void stepDone(sqlite3* database, sqlite3_stmt* statement) {
    if (sqlite3_step(statement) != SQLITE_DONE)
        databaseError(database, "Tidak dapat menyimpan data ke database");
}

StoredAnalysis readRecord(sqlite3_stmt* statement) {
    StoredAnalysis record;
    record.id = sqlite3_column_int64(statement, 0);
    const unsigned char* timestamp = sqlite3_column_text(statement, 1);
    const unsigned char* path = sqlite3_column_text(statement, 2);
    const unsigned char* status = sqlite3_column_text(statement, 3);
    record.analyzedAt = timestamp ? reinterpret_cast<const char*>(timestamp) : "";
    record.sourcePath = path ? reinterpret_cast<const char*>(path) : "";
    record.primaryStatus = status ? reinterpret_cast<const char*>(status) : "";
    record.sampleCount = static_cast<std::size_t>(sqlite3_column_int64(statement, 4));
    record.sampleRateHz = sqlite3_column_double(statement, 5);
    record.result.sampleCount = record.sampleCount;
    record.result.voltageMax = sqlite3_column_double(statement, 6);
    record.result.voltageMin = sqlite3_column_double(statement, 7);
    record.result.voltagePeakToPeak = sqlite3_column_double(statement, 8);
    record.result.voltageRms = sqlite3_column_double(statement, 9);
    record.result.currentRms = sqlite3_column_double(statement, 10);
    record.result.activePowerW = sqlite3_column_double(statement, 11);
    record.result.apparentPowerVa = sqlite3_column_double(statement, 12);
    record.result.powerFactorDefined = sqlite3_column_int(statement, 13) != 0;
    record.result.powerFactor = sqlite3_column_double(statement, 14);
    record.result.frequencyDefined = sqlite3_column_int(statement, 15) != 0;
    record.result.frequencyHz = sqlite3_column_double(statement, 16);
    record.result.thdDefined = sqlite3_column_int(statement, 17) != 0;
    record.result.thdPercent = sqlite3_column_double(statement, 18);
    const unsigned char* thdNote = sqlite3_column_text(statement, 19);
    record.result.thdNote = thdNote ? reinterpret_cast<const char*>(thdNote) : "";
    return record;
}

const char* RECORD_SELECT =
    "SELECT id, analyzed_at, source_path, primary_status, sample_count, sample_rate_hz, "
    "voltage_max, voltage_min, voltage_pp, voltage_rms, current_rms, active_power_w, apparent_power_va, "
    "power_factor_defined, power_factor, frequency_defined, frequency_hz, thd_defined, thd_percent, thd_note "
    ", nominal_voltage_vrms, nominal_frequency_hz, sag_lower_fraction, sag_upper_fraction, interruption_fraction, "
    "swell_fraction, thd_limit_percent, max_harmonic_order "
    "FROM analyses";
}

void AnalysisDatabase::initialize() {
    std::filesystem::create_directories("output");
    DatabaseHandle database = openDatabase();
    execute(database.get(),
        "CREATE TABLE IF NOT EXISTS analyses ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "analyzed_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),"
        "source_path TEXT NOT NULL, primary_status TEXT NOT NULL,"
        "sample_count INTEGER NOT NULL, sample_rate_hz REAL NOT NULL,"
        "voltage_max REAL NOT NULL, voltage_min REAL NOT NULL, voltage_pp REAL NOT NULL,"
        "voltage_rms REAL NOT NULL, current_rms REAL NOT NULL, active_power_w REAL NOT NULL,"
        "apparent_power_va REAL NOT NULL, power_factor_defined INTEGER NOT NULL, power_factor REAL NOT NULL,"
        "frequency_defined INTEGER NOT NULL, frequency_hz REAL NOT NULL,"
        "thd_defined INTEGER NOT NULL, thd_percent REAL NOT NULL, thd_note TEXT NOT NULL,"
        "nominal_voltage_vrms REAL NOT NULL, nominal_frequency_hz REAL NOT NULL,"
        "sag_lower_fraction REAL NOT NULL, sag_upper_fraction REAL NOT NULL,"
        "interruption_fraction REAL NOT NULL, swell_fraction REAL NOT NULL,"
        "thd_limit_percent REAL NOT NULL, max_harmonic_order INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS analysis_conditions ("
        "analysis_id INTEGER NOT NULL, condition_order INTEGER NOT NULL, condition TEXT NOT NULL,"
        "PRIMARY KEY (analysis_id, condition_order),"
        "FOREIGN KEY (analysis_id) REFERENCES analyses(id) ON DELETE CASCADE"
        ");"
        "CREATE TABLE IF NOT EXISTS samples ("
        "analysis_id INTEGER NOT NULL, sample_index INTEGER NOT NULL, time_seconds REAL NOT NULL,"
        "voltage REAL NOT NULL, current REAL NOT NULL,"
        "PRIMARY KEY (analysis_id, sample_index),"
        "FOREIGN KEY (analysis_id) REFERENCES analyses(id) ON DELETE CASCADE"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_analyses_analyzed_at ON analyses(analyzed_at DESC);");
}

std::int64_t AnalysisDatabase::save(const MeasurementDataset& dataset, const AnalysisResult& result,
                                    const Config& config, const DisturbanceReport& report) {
    if (dataset.samples.empty()) throw std::runtime_error("Tidak ada sampel untuk disimpan ke riwayat.");
    initialize();
    DatabaseHandle database = openDatabase();
    execute(database.get(), "BEGIN IMMEDIATE;");
    try {
        StatementHandle insert = prepare(database.get(),
            "INSERT INTO analyses (source_path, primary_status, sample_count, sample_rate_hz, "
            "voltage_max, voltage_min, voltage_pp, voltage_rms, current_rms, active_power_w, apparent_power_va, "
            "power_factor_defined, power_factor, frequency_defined, frequency_hz, thd_defined, thd_percent, thd_note, "
            "nominal_voltage_vrms, nominal_frequency_hz, sag_lower_fraction, sag_upper_fraction, interruption_fraction, "
            "swell_fraction, thd_limit_percent, max_harmonic_order) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
        const std::string primaryStatus = report.conditions.empty() ? "UNKNOWN" : report.conditions.front();
        int parameter = 1;
        bindText(database.get(), insert.get(), parameter++, dataset.sourcePath);
        bindText(database.get(), insert.get(), parameter++, primaryStatus);
        bindInt64(database.get(), insert.get(), parameter++, static_cast<sqlite3_int64>(result.sampleCount));
        bindDouble(database.get(), insert.get(), parameter++, dataset.sampleRateHz);
        bindDouble(database.get(), insert.get(), parameter++, result.voltageMax);
        bindDouble(database.get(), insert.get(), parameter++, result.voltageMin);
        bindDouble(database.get(), insert.get(), parameter++, result.voltagePeakToPeak);
        bindDouble(database.get(), insert.get(), parameter++, result.voltageRms);
        bindDouble(database.get(), insert.get(), parameter++, result.currentRms);
        bindDouble(database.get(), insert.get(), parameter++, result.activePowerW);
        bindDouble(database.get(), insert.get(), parameter++, result.apparentPowerVa);
        bindInt(database.get(), insert.get(), parameter++, result.powerFactorDefined ? 1 : 0);
        bindDouble(database.get(), insert.get(), parameter++, result.powerFactor);
        bindInt(database.get(), insert.get(), parameter++, result.frequencyDefined ? 1 : 0);
        bindDouble(database.get(), insert.get(), parameter++, result.frequencyHz);
        bindInt(database.get(), insert.get(), parameter++, result.thdDefined ? 1 : 0);
        bindDouble(database.get(), insert.get(), parameter++, result.thdPercent);
        bindText(database.get(), insert.get(), parameter++, result.thdNote);
        bindDouble(database.get(), insert.get(), parameter++, config.nominalVoltageVrms);
        bindDouble(database.get(), insert.get(), parameter++, config.nominalFrequencyHz);
        bindDouble(database.get(), insert.get(), parameter++, config.sagLowerFraction);
        bindDouble(database.get(), insert.get(), parameter++, config.sagUpperFraction);
        bindDouble(database.get(), insert.get(), parameter++, config.interruptionFraction);
        bindDouble(database.get(), insert.get(), parameter++, config.swellFraction);
        bindDouble(database.get(), insert.get(), parameter++, config.thdLimitPercent);
        bindInt(database.get(), insert.get(), parameter, config.maxHarmonicOrder);
        stepDone(database.get(), insert.get());
        const sqlite3_int64 id = sqlite3_last_insert_rowid(database.get());
        reset(database.get(), insert.get());

        StatementHandle conditionInsert = prepare(database.get(),
            "INSERT INTO analysis_conditions (analysis_id, condition_order, condition) VALUES (?, ?, ?);");
        for (std::size_t index = 0; index < report.conditions.size(); ++index) {
            bindInt64(database.get(), conditionInsert.get(), 1, id);
            bindInt64(database.get(), conditionInsert.get(), 2, static_cast<sqlite3_int64>(index));
            bindText(database.get(), conditionInsert.get(), 3, report.conditions[index]);
            stepDone(database.get(), conditionInsert.get());
            reset(database.get(), conditionInsert.get());
        }

        StatementHandle sampleInsert = prepare(database.get(),
            "INSERT INTO samples (analysis_id, sample_index, time_seconds, voltage, current) VALUES (?, ?, ?, ?, ?);");
        for (std::size_t index = 0; index < dataset.samples.size(); ++index) {
            const Sample& sample = dataset.samples[index];
            bindInt64(database.get(), sampleInsert.get(), 1, id);
            bindInt64(database.get(), sampleInsert.get(), 2, static_cast<sqlite3_int64>(index));
            bindDouble(database.get(), sampleInsert.get(), 3, sample.timeSeconds);
            bindDouble(database.get(), sampleInsert.get(), 4, sample.voltage);
            bindDouble(database.get(), sampleInsert.get(), 5, sample.current);
            stepDone(database.get(), sampleInsert.get());
            reset(database.get(), sampleInsert.get());
        }
        execute(database.get(), "COMMIT;");
        return static_cast<std::int64_t>(id);
    } catch (...) {
        try { execute(database.get(), "ROLLBACK;"); } catch (...) {}
        throw;
    }
}

std::vector<StoredAnalysis> AnalysisDatabase::loadRecent(std::size_t limit) {
    initialize();
    DatabaseHandle database = openDatabase();
    const std::string sql = std::string(RECORD_SELECT) + " ORDER BY analyzed_at DESC, id DESC LIMIT ?;";
    StatementHandle statement = prepare(database.get(), sql.c_str());
    bindInt64(database.get(), statement.get(), 1, static_cast<sqlite3_int64>(limit));
    std::vector<StoredAnalysis> records;
    int result = SQLITE_ROW;
    while ((result = sqlite3_step(statement.get())) == SQLITE_ROW) records.push_back(readRecord(statement.get()));
    if (result != SQLITE_DONE) databaseError(database.get(), "Tidak dapat membaca riwayat analisis");
    return records;
}

void AnalysisDatabase::load(std::int64_t id, MeasurementDataset& dataset, AnalysisResult& result,
                            Config& config, DisturbanceReport& report) {
    initialize();
    DatabaseHandle database = openDatabase();
    const std::string sql = std::string(RECORD_SELECT) + " WHERE id = ?;";
    StatementHandle recordStatement = prepare(database.get(), sql.c_str());
    bindInt64(database.get(), recordStatement.get(), 1, static_cast<sqlite3_int64>(id));
    if (sqlite3_step(recordStatement.get()) != SQLITE_ROW)
        throw std::runtime_error("Riwayat analisis tidak ditemukan di database.");
    const StoredAnalysis record = readRecord(recordStatement.get());

    dataset = MeasurementDataset{};
    dataset.sourcePath = record.sourcePath;
    dataset.sampleRateHz = record.sampleRateHz;
    result = record.result;
    report = DisturbanceReport{};
    config = Config{};
    config.nominalVoltageVrms = sqlite3_column_double(recordStatement.get(), 20);
    config.nominalFrequencyHz = sqlite3_column_double(recordStatement.get(), 21);
    config.sagLowerFraction = sqlite3_column_double(recordStatement.get(), 22);
    config.sagUpperFraction = sqlite3_column_double(recordStatement.get(), 23);
    config.interruptionFraction = sqlite3_column_double(recordStatement.get(), 24);
    config.swellFraction = sqlite3_column_double(recordStatement.get(), 25);
    config.thdLimitPercent = sqlite3_column_double(recordStatement.get(), 26);
    config.maxHarmonicOrder = sqlite3_column_int(recordStatement.get(), 27);

    StatementHandle sampleStatement = prepare(database.get(),
        "SELECT time_seconds, voltage, current FROM samples WHERE analysis_id = ? ORDER BY sample_index;");
    bindInt64(database.get(), sampleStatement.get(), 1, static_cast<sqlite3_int64>(id));
    int sampleResult = SQLITE_ROW;
    while ((sampleResult = sqlite3_step(sampleStatement.get())) == SQLITE_ROW) {
        dataset.samples.push_back({sqlite3_column_double(sampleStatement.get(), 0),
                                   sqlite3_column_double(sampleStatement.get(), 1),
                                   sqlite3_column_double(sampleStatement.get(), 2)});
    }
    if (sampleResult != SQLITE_DONE) databaseError(database.get(), "Tidak dapat membaca sampel dari riwayat");

    StatementHandle conditionStatement = prepare(database.get(),
        "SELECT condition FROM analysis_conditions WHERE analysis_id = ? ORDER BY condition_order;");
    bindInt64(database.get(), conditionStatement.get(), 1, static_cast<sqlite3_int64>(id));
    int conditionResult = SQLITE_ROW;
    while ((conditionResult = sqlite3_step(conditionStatement.get())) == SQLITE_ROW) {
        const unsigned char* condition = sqlite3_column_text(conditionStatement.get(), 0);
        report.conditions.emplace_back(condition ? reinterpret_cast<const char*>(condition) : "");
    }
    if (conditionResult != SQLITE_DONE) databaseError(database.get(), "Tidak dapat membaca diagnosis dari riwayat");
}
