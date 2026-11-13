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

TEST(Database, OpeningDirectoryAsDatabaseFails) {
    EXPECT_THROW(Database db(testing::TempDir()), std::runtime_error);
}

TEST(Database, ConstraintFailureDoesNotPoisonConnection) {
    Database db(":memory:");
    db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY, value TEXT NOT NULL)");
    db.execute("INSERT INTO t VALUES (1, 'original')");
    EXPECT_THROW(db.execute("INSERT INTO t VALUES (1, 'duplicate')"), std::runtime_error);
    EXPECT_THROW(db.execute("INSERT INTO t VALUES (2, NULL)"), std::runtime_error);
    EXPECT_EQ(db.query("SELECT value FROM t")[0][0], "original");
    EXPECT_NO_THROW(db.execute("INSERT INTO t VALUES (2, 'valid')"));
}

TEST(Database, NullColumnsAndEmptyResultsAreHandled) {
    Database db(":memory:");
    auto rows = db.query("SELECT NULL, 'present'");
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0], (Database::Row{"", "present"}));
    EXPECT_TRUE(db.query("SELECT 1 WHERE 0").empty());
}

TEST(Database, ConstraintFailureRollsBackEarlierWrites) {
    Database db(":memory:");
    db.execute("CREATE TABLE t (id INTEGER PRIMARY KEY)");
    EXPECT_THROW(db.transaction([&] {
        db.execute("INSERT INTO t VALUES (1)");
        db.execute("INSERT INTO t VALUES (1)");
    }), std::runtime_error);
    EXPECT_TRUE(db.query("SELECT * FROM t").empty());
    EXPECT_NO_THROW(db.transaction([&] { db.execute("INSERT INTO t VALUES (2)"); }));
    EXPECT_EQ(db.query("SELECT id FROM t")[0][0], "2");
}
