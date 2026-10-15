#include <gtest/gtest.h>

#include "db/Database.h"

using rtrs::Database;

TEST(Database, InsertAndQueryWithBoundParameters) {
    Database db(":memory:");
    db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT)");
    db.execute("INSERT INTO t (name) VALUES (?)", {"o'brien; DROP TABLE t;--"});
    auto rows = db.query("SELECT name FROM t WHERE id = ?", {"1"});
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0][0], "o'brien; DROP TABLE t;--");
    EXPECT_EQ(db.lastInsertId(), 1);
}

TEST(Database, InvalidSqlThrows) {
    Database db(":memory:");
    EXPECT_THROW(db.execute("NOT SQL"), std::runtime_error);
}

TEST(Database, TransactionRollsBackOnException) {
    Database db(":memory:");
    db.execute("CREATE TABLE t (id INTEGER)");
    EXPECT_THROW(db.transaction([&] {
        db.execute("INSERT INTO t VALUES (1)");
        throw std::runtime_error("boom");
    }), std::runtime_error);
    EXPECT_TRUE(db.query("SELECT * FROM t").empty());
}

TEST(Database, TransactionCommits) {
    Database db(":memory:");
    db.execute("CREATE TABLE t (id INTEGER)");
    db.transaction([&] { db.execute("INSERT INTO t VALUES (1)"); });
    EXPECT_EQ(db.query("SELECT * FROM t").size(), 1u);
}
