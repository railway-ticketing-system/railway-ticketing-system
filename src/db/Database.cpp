#include "db/Database.h"

#include <stdexcept>

namespace rtrs {

namespace {
struct Stmt {
    sqlite3_stmt* s = nullptr;
    ~Stmt() { sqlite3_finalize(s); }
};
}  // namespace

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
        sqlite3_close(db_);
        throw std::runtime_error("cannot open database: " + msg);
    }
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
}

Database::~Database() { sqlite3_close(db_); }

std::vector<Database::Row> Database::query(const std::string& sql, const std::vector<Param>& params) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    Stmt st;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &st.s, nullptr) != SQLITE_OK)
        throw std::runtime_error(std::string("sql prepare failed: ") + sqlite3_errmsg(db_));
    for (size_t i = 0; i < params.size(); ++i)
        sqlite3_bind_text(st.s, static_cast<int>(i) + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);

    std::vector<Row> rows;
    int rc;
    while ((rc = sqlite3_step(st.s)) == SQLITE_ROW) {
        Row row;
        int cols = sqlite3_column_count(st.s);
        for (int c = 0; c < cols; ++c) {
            const unsigned char* t = sqlite3_column_text(st.s, c);
            row.emplace_back(t ? reinterpret_cast<const char*>(t) : "");
        }
        rows.push_back(std::move(row));
    }
    if (rc != SQLITE_DONE) throw std::runtime_error(std::string("sql step failed: ") + sqlite3_errmsg(db_));
    return rows;
}

void Database::execute(const std::string& sql, const std::vector<Param>& params) { query(sql, params); }

long long Database::lastInsertId() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return sqlite3_last_insert_rowid(db_);
}

void Database::transaction(const std::function<void()>& fn) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    execute("BEGIN");
    try {
        fn();
        execute("COMMIT");
    } catch (...) {
        execute("ROLLBACK");
        throw;
    }
}

}  // namespace rtrs
