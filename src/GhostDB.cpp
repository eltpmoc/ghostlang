#include "GhostDB.hpp"
#include "sqlite3.h"

#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace {
std::mutex db_mutex;
std::map<int, sqlite3 *> active_dbs;
int next_handle = 1;

sqlite3 *get_db(int handle) {
  auto it = active_dbs.find(handle);
  if (it == active_dbs.end()) {
    throw std::runtime_error("Handle de banco de dados SQLite invalido: " +
                             std::to_string(handle));
  }
  return it->second;
}
} // namespace

int ghost_db_open(const std::string &path) {
  std::lock_guard<std::mutex> lock(db_mutex);
  sqlite3 *db = nullptr;
  int rc = sqlite3_open(path.c_str(), &db);
  if (rc != SQLITE_OK) {
    std::string err =
        db ? sqlite3_errmsg(db) : "Erro ao alocar memoria para SQLite";
    if (db)
      sqlite3_close(db);
    throw std::runtime_error("Falha ao abrir SQLite (" + path + "): " + err);
  }
  int handle = next_handle++;
  active_dbs[handle] = db;
  return handle;
}

int ghost_db_exec(int handle, const std::string &sql) {
  std::lock_guard<std::mutex> lock(db_mutex);
  sqlite3 *db = get_db(handle);
  char *err_msg = nullptr;
  int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg);
  if (rc != SQLITE_OK) {
    std::string err = err_msg ? err_msg : "Erro desconhecido";
    sqlite3_free(err_msg);
    throw std::runtime_error("Erro de execucao SQLite: " + err);
  }
  return rc;
}

std::string ghost_db_query(int handle, const std::string &sql) {
  std::lock_guard<std::mutex> lock(db_mutex);
  sqlite3 *db = get_db(handle);

  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    throw std::runtime_error("Erro na consulta SQLite: " +
                             std::string(sqlite3_errmsg(db)));
  }

  std::ostringstream json;
  json << "[";
  bool first_row = true;
  int cols = sqlite3_column_count(stmt);

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    if (!first_row)
      json << ", ";
    first_row = false;
    json << "{";
    for (int c = 0; c < cols; ++c) {
      if (c > 0)
        json << ", ";
      const char *col_name = sqlite3_column_name(stmt, c);
      json << "\"" << (col_name ? col_name : "") << "\": ";
      int type = sqlite3_column_type(stmt, c);
      if (type == SQLITE_INTEGER) {
        json << sqlite3_column_int64(stmt, c);
      } else if (type == SQLITE_FLOAT) {
        json << sqlite3_column_double(stmt, c);
      } else if (type == SQLITE_NULL) {
        json << "null";
      } else {
        const unsigned char *text = sqlite3_column_text(stmt, c);
        std::string s = text ? reinterpret_cast<const char *>(text) : "";
        json << "\"";
        for (char ch : s) {
          if (ch == '"')
            json << "\\\"";
          else if (ch == '\\')
            json << "\\\\";
          else if (ch == '\n')
            json << "\\n";
          else if (ch == '\r')
            json << "\\r";
          else if (ch == '\t')
            json << "\\t";
          else
            json << ch;
        }
        json << "\"";
      }
    }
    json << "}";
  }

  sqlite3_finalize(stmt);
  json << "]";
  return json.str();
}

int ghost_db_close(int handle) {
  std::lock_guard<std::mutex> lock(db_mutex);
  auto it = active_dbs.find(handle);
  if (it != active_dbs.end()) {
    int rc = sqlite3_close(it->second);
    active_dbs.erase(it);
    return rc;
  }
  return 0;
}

int ghost_db_last_id(int handle) {
  std::lock_guard<std::mutex> lock(db_mutex);
  sqlite3 *db = get_db(handle);
  return static_cast<int>(sqlite3_last_insert_rowid(db));
}

int ghost_db_changes(int handle) {
  std::lock_guard<std::mutex> lock(db_mutex);
  sqlite3 *db = get_db(handle);
  return sqlite3_changes(db);
}
