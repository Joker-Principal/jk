#include <array>
#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <jk/wrap/sqlite.h>

using namespace JK::Wrap::Sqlite;
using JK::Wrap::Result;
using namespace std::literals;

struct Row
{
	long long id = 0;
	std::string name;
	double score = 0.0;
	bool flag = false;
	std::vector<std::byte> data;
};

constexpr std::byte blobBytes[] = { std::byte{ 1 }, std::byte{ 2 }, std::byte{ 3 } };

std::error_code sqliteError(int code)
{
	return { code, category() };
}

int main()
{
	static_assert(typeText(SQLITE_INTEGER) == "INTEGER");
	static_assert(typeText(SQLITE3_TEXT) == "TEXT");

	// Result 本身
	{
		Result<int> value = 42;
		assert(value && value.value() == 42);

		Result<int> failure = JK::Wrap::unexpected(sqliteError(SQLITE_ERROR));
		assert(!failure);
		assert(failure.error() == sqliteError(SQLITE_ERROR));
	}

	auto conn = Conn::open(":memory:");
	assert(conn);
	auto& db = conn.value();

	assert(db.exec(R"(
		CREATE TABLE t(
			id INTEGER PRIMARY KEY,
			name TEXT,
			score REAL,
			flag INTEGER,
			data BLOB
		)
	)"));
	assert(db.exec("CREATE INDEX idx_t_name ON t(name)"));

	// 插入：binds 覆盖字符串/浮点/bool/blob
	{
		auto stmt = db.prepare("INSERT INTO t(name, score, flag, data) VALUES (?, ?, ?, ?)");
		assert(stmt);

		auto bound = stmt->binds(
			std::string_view("alpha"), 1.5, true, std::span<const std::byte>(blobBytes));
		assert(bound);

		auto stepped = stmt->step();
		assert(stepped);
		assert(!stepped.value());
	}

	// 再插一行 NULL blob，验证 nullable bool 与 nullptr 绑定
	{
		auto stmt = db.prepare("INSERT INTO t(name, score, flag, data) VALUES (?, ?, ?, ?)");
		assert(stmt);
		assert(stmt->binds("beta"sv, 2.5, false, nullptr));
		assert(stmt->step());
	}

	// 查询：columns / fetch
	{
		auto stmt = db.prepare("SELECT id, name, score, flag, data FROM t ORDER BY id");
		assert(stmt);

		Row row;
		auto fetched = stmt->fetch(row.id, row.name, row.score, row.flag, row.data);
		assert(fetched);
		assert(fetched.value());
		assert(row.id == 1);
		assert(row.name == "alpha");
		assert(row.score == 1.5);
		assert(row.flag);
		assert(row.data.size() == 3);
		assert(row.data[0] == std::byte{ 1 });

		Row second;
		bool nullable = true;
		std::nullptr_t ignore = nullptr;
		fetched = stmt->fetch(second.id, second.name, second.score, second.flag, ignore);
		assert(fetched);
		assert(fetched.value());
		assert(second.id == 2);
		assert(second.name == "beta");
		assert(!second.flag);

		fetched = stmt->step();
		assert(fetched);
		assert(!fetched.value());
	}

	// 单列取值：string_view 是 sqlite 缓冲区里的视图
	{
		auto stmt = db.prepare("SELECT name, data FROM t WHERE id = ?");
		assert(stmt);
		assert(stmt->bind(1, 1));
		assert(stmt->step());

		std::string_view name;
		assert(stmt->column(0, name));
		assert(name == "alpha");

		std::array<std::byte, 3> fixed{};
		assert(stmt->column(1, std::span<std::byte, 3>(fixed)));
		assert(fixed[2] == std::byte{ 3 });
	}

	// 类型不匹配：返回 SQLITE_MISMATCH 而不是崩溃
	{
		auto stmt = db.prepare("SELECT name FROM t WHERE id = 1");
		assert(stmt);
		assert(stmt->step());

		int wrongType = 0;
		auto res = stmt->column(0, wrongType);
		assert(!res);
		assert(res.error() == sqliteError(SQLITE_MISMATCH));
	}

	// 错误路径：坏 SQL / 缺表
	{
		auto bad = db.prepare("SELECT * FROM missing_table");
		assert(!bad);
		assert(bad.error().category() == category());

		assert(!db.exec("NOT SQL"));
		assert(!db.execWithError("NOT SQL").empty());
		assert(db.lastError().value() != SQLITE_OK);
	}

	// 只读打开不存在的文件应失败
	{
		auto missing = Conn::open("jk-sqlite-test-missing.db", Conn::readonly);
		assert(!missing);
	}

	// Blob：读 + 写
	{
		auto blob = db.openBlob("t", "data", 1);
		assert(blob);
		assert(blob.value().bytes() == 3);

		std::byte buffer[3]{};
		assert(blob.value().read(0, buffer));
		assert(buffer[1] == std::byte{ 2 });
		assert(blob.value().close());
	}
	{
		constexpr std::byte replacement[] = { std::byte{ 7 }, std::byte{ 8 }, std::byte{ 9 } };
		auto blob = db.openBlob("t", "data", 1, false);
		assert(blob);
		assert(blob.value().write(0, std::span<const std::byte>(replacement)));
		assert(blob.value().close());

		std::vector<std::byte> data;
		auto stmt = db.prepare("SELECT data FROM t WHERE id = 1");
		assert(stmt);
		assert(stmt->step());
		assert(stmt->column(0, data));
		assert(data.size() == 3);
		assert(data[0] == std::byte{ 7 });
	}

	// move 语义：被 move 的对象析构时不会重复释放
	{
		auto stmt = db.prepare("SELECT 1");
		assert(stmt);
		auto moved = std::move(stmt).value();
		assert(stmt.value().stmt == nullptr);
		assert(moved.stmt != nullptr);
	}

	db.close();
	assert(db.db == nullptr);
}
