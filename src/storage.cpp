#include "storage.h"
#include <iostream>
#include <ctime>

namespace storage {

namespace {

bool exec_simple(sqlite3* db, const std::string& sql, const std::string& context) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "storage: error in " << context << ": "
                  << (err_msg != nullptr ? err_msg : "unknown") << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

} // namespace

bool initialize_database(const std::string& db_path, sqlite3** db) {
    if (db == nullptr) {
        std::cerr << "storage: invalid output db pointer" << std::endl;
        return false;
    }

    int rc = sqlite3_open(db_path.c_str(), db);
    if (rc != SQLITE_OK) {
        std::cerr << "storage: could not open database '" << db_path
                   << "': " << sqlite3_errmsg(*db) << std::endl;
        sqlite3_close(*db);
        *db = nullptr;
        return false;
    }

    static const std::string create_table_sql =
        "CREATE TABLE IF NOT EXISTS daily_stats ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "timestamp INTEGER, "
        "keystrokes INTEGER);";

    if (!exec_simple(*db, create_table_sql, "creating daily_stats table")) {
        sqlite3_close(*db);
        *db = nullptr;
        return false;
    }

    return true;
}

bool save_keystrokes(sqlite3* db, int keystrokes) {
    if (db == nullptr) {
        std::cerr << "storage: invalid database handle" << std::endl;
        return false;
    }

    static const char* insert_sql =
        "INSERT INTO daily_stats (timestamp, keystrokes) VALUES (?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "storage: error preparing insert: "
                   << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    const int64_t current_timestamp = static_cast<int64_t>(std::time(nullptr));

    if (sqlite3_bind_int64(stmt, 1, current_timestamp) != SQLITE_OK ||
        sqlite3_bind_int(stmt, 2, keystrokes) != SQLITE_OK) {
        std::cerr << "storage: error binding parameters: "
                   << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "storage: error inserting record: "
                   << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    return true;
}

void close_database(sqlite3* db) {
    if (db) {
        sqlite3_close(db);
    }
}

} // namespace storage

