#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <sqlite3.h>

namespace storage {

constexpr const char* kTableName = "daily_stats";

bool initialize_database(const std::string& db_path, sqlite3** db);
bool save_keystrokes(sqlite3* db, int keystrokes);
void close_database(sqlite3* db);

} // namespace storage

#endif // STORAGE_H
