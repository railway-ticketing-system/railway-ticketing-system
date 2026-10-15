#pragma once
#include <sqlite3.h>

#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace rtrs {

// Thin RAII wrapper over a SQLite connection. All access is serialised with a
// mutex so the multi-threaded HTTP server can share one connection safely.
class Database {
public:
    using Row = std::vector<std::string>;
    using Param = std::string;

    explicit Database(const std::string& path);  // ":memory:" for tests
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Runs a statement with bound text parameters. Throws std::runtime_error on failure.
    void execute(const std::string& sql, const std::vector<Param>& params = {});

    // Runs a query and returns every row as text columns (NULL becomes "").
    std::vector<Row> query(const std::string& sql, const std::vector<Param>& params = {});

    long long lastInsertId();

    // Runs fn inside BEGIN/COMMIT; rolls back and rethrows if fn throws.
    void transaction(const std::function<void()>& fn);

private:
    sqlite3* db_ = nullptr;
    std::recursive_mutex mutex_;
};

}  // namespace rtrs
