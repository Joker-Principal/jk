#pragma once
#include <sqlite3.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>
#include <jk/wrap/expected.h>
#include <jk/meta/type-list.h>

namespace JK::Wrap::Details::Sqlite
{
using JK::Wrap::Result;
using JK::Wrap::unexpected;

class ErrorCategory final : public std::error_category
{
public:
	const char* name() const noexcept override { return "sqlite3"; }
	std::string message(int ev) const override { return ::sqlite3_errstr(ev); }
};

inline const std::error_category& category() noexcept
{
	static const ErrorCategory instance;
	return instance;
}

inline std::error_code makeCode(int code) noexcept
{
	return { code, category() };
}

inline unexpected<std::error_code> makeError(int code) noexcept
{
	return unexpected(makeCode(code));
}

// SQLITE_OK -> 成功，其它 -> 失败
inline Result<void> check(int code) noexcept
{
	if(code == SQLITE_OK) return {};
	return makeError(code);
}

constexpr std::string_view typeText(int typeCode) noexcept
{
	switch(typeCode)
	{
	case SQLITE_INTEGER: return "INTEGER";
	case SQLITE_FLOAT:   return "REAL";
	case SQLITE3_TEXT:   return "TEXT";
	case SQLITE_BLOB:    return "BLOB";
	case SQLITE_NULL:    return "NULL";
	default:             return "UNKNOWN";
	}
}

struct Stmt
{
public:
	Stmt(::sqlite3_stmt* stmt) noexcept :
		stmt(stmt)
	{}

	Stmt(const Stmt&) = delete;
	Stmt& operator=(const Stmt&) = delete;
	Stmt(Stmt&& other) noexcept :
		stmt(std::exchange(other.stmt, nullptr))
	{}
	Stmt& operator=(Stmt&& other) & noexcept
	{
		stmt = std::exchange(other.stmt, nullptr);
		return *this;
	}
	~Stmt() noexcept { ::sqlite3_finalize(stmt); }

public:
	std::error_code lastError() const noexcept
	{
		if(!stmt) return makeCode(SQLITE_MISUSE);
		return makeCode(::sqlite3_extended_errcode(::sqlite3_db_handle(stmt)));
	}

public:
	// SQLITE_STATIC: the bound view must outlive the next step / finalize.
	template<typename C>
	Result<void> bind(int index, std::basic_string_view<C> value) noexcept
	{
		int rc = [&] {
			if constexpr (std::same_as<C, char> || std::same_as<C, char8_t>)
				return ::sqlite3_bind_text64(
					stmt, index, reinterpret_cast<const char*>(value.data()),
					static_cast<sqlite3_uint64>(value.size()), SQLITE_STATIC, SQLITE_UTF8);
			else if constexpr (std::same_as<C, char16_t>)
				return ::sqlite3_bind_text16(
					stmt, index, value.data(),
					static_cast<int>(value.size() * sizeof(char16_t)), SQLITE_STATIC);
#if defined(_WIN32)
			else if constexpr (std::same_as<C, wchar_t>)
				return ::sqlite3_bind_text16(
					stmt, index, value.data(),
					static_cast<int>(value.size() * sizeof(wchar_t)), SQLITE_STATIC);
#endif
			else
				static_assert(!std::same_as<C, C>, "unsupported character type for binding");
		}();

		return check(rc);
	}

	template<typename C, typename T, typename A>
	Result<void> bind(int index, const std::basic_string<C, T, A>& value) noexcept
	{
		return bind(index, std::basic_string_view<C>(value));
	}

	template<std::integral T>
	Result<void> bind(int index, T value) noexcept
	{
		int rc = [&] {
			if constexpr (sizeof(T) <= sizeof(int))
				return ::sqlite3_bind_int(stmt, index, static_cast<int>(value));
			else
				return ::sqlite3_bind_int64(stmt, index, static_cast<sqlite3_int64>(value));
		}();

		return check(rc);
	}

	template<std::floating_point T>
	Result<void> bind(int index, T value) noexcept
	{
		int rc = ::sqlite3_bind_double(stmt, index, static_cast<double>(value));
		return check(rc);
	}

	Result<void> bind(int index, std::span<const std::byte> value) noexcept
	{
		int rc = ::sqlite3_bind_blob64(
			stmt, index, value.data(), static_cast<sqlite3_uint64>(value.size()), SQLITE_STATIC);
		return check(rc);
	}

	Result<void> bind(int index, std::nullptr_t) noexcept
	{
		int rc = ::sqlite3_bind_null(stmt, index);
		return check(rc);
	}

	Result<void> bind(int index, bool value) noexcept
	{
		int rc = ::sqlite3_bind_int(stmt, index, value ? 1 : 0);
		return check(rc);
	}

	template<typename Arg, typename... Args>
	Result<void> bindsImpl(int index, Arg&& arg, Args&&... args) noexcept
	{
		auto res = bind(index, std::forward<Arg>(arg));
		if(!res) return unexpected(res.error());
		return bindsImpl(index + 1, std::forward<Args>(args)...);
	}

	constexpr Result<void> bindsImpl(int) noexcept { return {}; }

	template<typename... Args>
		requires (sizeof...(Args) >= 1)
	Result<void> binds(Args&&... args) noexcept
	{
		return bindsImpl(1, std::forward<Args>(args)...);
	}

public:
	Result<bool> step() noexcept
	{
		int rc = ::sqlite3_step(stmt);
		if(rc == SQLITE_ROW) return true;
		if(rc == SQLITE_DONE) return false;
		return makeError(rc);
	}

public:
	template<typename C>
	Result<void> column(int index, std::basic_string_view<C>& out) noexcept
	{
		if(int t = ::sqlite3_column_type(stmt, index); t != SQLITE3_TEXT)
			return makeError(SQLITE_MISMATCH);

		if constexpr (std::same_as<C, char> || std::same_as<C, char8_t>)
		{
			auto text = ::sqlite3_column_text(stmt, index);
			std::size_t size = static_cast<std::size_t>(::sqlite3_column_bytes(stmt, index));
			if(!text) return makeError(SQLITE_NOMEM);
			out = { reinterpret_cast<const C*>(text), size };
		}
		else if constexpr (std::same_as<C, char16_t>)
		{
			auto text = ::sqlite3_column_text16(stmt, index);
			std::size_t size = static_cast<std::size_t>(::sqlite3_column_bytes16(stmt, index)) / sizeof(char16_t);
			if(!text) return makeError(SQLITE_NOMEM);
			out = { reinterpret_cast<const C*>(text), size };
		}
#if defined(_WIN32)
		else if constexpr (std::same_as<C, wchar_t>)
		{
			auto text = ::sqlite3_column_text16(stmt, index);
			std::size_t size = static_cast<std::size_t>(::sqlite3_column_bytes16(stmt, index)) / sizeof(wchar_t);
			if(!text) return makeError(SQLITE_NOMEM);
			out = { reinterpret_cast<const C*>(text), size };
		}
#endif
		else
			static_assert(!std::same_as<C, C>, "unsupported character type for column retrieval");

		return {};
	}

	template<typename C, typename T, typename A>
	Result<void> column(int index, std::basic_string<C, T, A>& out)
	{
		std::basic_string_view<C> view;
		auto res = column(index, view);
		if(!res) return unexpected(res.error());
		out.assign(view);
		return {};
	}

	Result<void> column(int index, std::span<const std::byte>& out) noexcept
	{
		if(int t = ::sqlite3_column_type(stmt, index); t != SQLITE_BLOB)
			return makeError(SQLITE_MISMATCH);

		const void* blob = ::sqlite3_column_blob(stmt, index);
		std::size_t size = static_cast<std::size_t>(::sqlite3_column_bytes(stmt, index));
		if(!blob && size > 0) return makeError(SQLITE_NOMEM);

		out = { static_cast<const std::byte*>(blob), size };
		return {};
	}

	template<std::size_t N>
		requires (N != std::dynamic_extent)
	Result<void> column(int index, std::span<std::byte, N> out) noexcept
	{
		std::span<const std::byte> blob;
		auto res = column(index, blob);
		if(!res) return unexpected(res.error());
		if(blob.size() != N) return makeError(SQLITE_MISMATCH);

		std::memcpy(out.data(), blob.data(), N);
		return {};
	}

	Result<void> column(int index, std::vector<std::byte>& out)
	{
		std::span<const std::byte> blob;
		auto res = column(index, blob);
		if(!res) return unexpected(res.error());

		out.resize(blob.size());
		std::memcpy(out.data(), blob.data(), blob.size());
		return {};
	}

	// SQLite has no BOOLEAN type; bools are stored as INTEGER 0/1.
	Result<void> column(int index, bool& out) noexcept
	{
		if(int t = ::sqlite3_column_type(stmt, index); t != SQLITE_INTEGER)
			return makeError(SQLITE_MISMATCH);

		int value = ::sqlite3_column_int(stmt, index);
		if(value != 0 && value != 1) return makeError(SQLITE_MISMATCH);

		out = (value != 0);
		return {};
	}

	// nullable bool: NULL is false, non-zero INTEGER is true.
	Result<void> column(int index, bool* out) noexcept
	{
		int t = ::sqlite3_column_type(stmt, index);
		if(t == SQLITE_NULL)
			*out = false;
		else if(t == SQLITE_INTEGER)
			*out = (::sqlite3_column_int(stmt, index) != 0);
		else
			return makeError(SQLITE_MISMATCH);

		return {};
	}

	template<std::integral T>
	Result<void> column(int index, T& out) noexcept
	{
		if(int t = ::sqlite3_column_type(stmt, index); t != SQLITE_INTEGER)
			return makeError(SQLITE_MISMATCH);

		if constexpr (sizeof(T) <= sizeof(int))
			out = static_cast<T>(::sqlite3_column_int(stmt, index));
		else
			out = static_cast<T>(::sqlite3_column_int64(stmt, index));

		return {};
	}

	template<std::floating_point T>
	Result<void> column(int index, T& out) noexcept
	{
		if(int t = ::sqlite3_column_type(stmt, index); t != SQLITE_FLOAT)
			return makeError(SQLITE_MISMATCH);

		out = static_cast<T>(::sqlite3_column_double(stmt, index));
		return {};
	}

	// placeholder overload to make it possible to ignore certain columns
	constexpr Result<void> column(int, std::nullptr_t) noexcept { return {}; }

	template<typename Arg, typename... Args>
	Result<void> columnsImpl(int index, Arg&& arg, Args&&... args) noexcept
	{
		auto res = column(index, arg);
		if(!res) return unexpected(res.error());
		return columnsImpl(index + 1, std::forward<Args>(args)...);
	}

	constexpr Result<void> columnsImpl(int) noexcept { return {}; }

	template<typename T>
	static constexpr bool correctColumnType =
		std::is_lvalue_reference_v<T> ||
		JK::Meta::TList<std::nullptr_t, bool*>::template contains<std::decay_t<T>>;

	template<typename... Args>
	Result<void> columns(Args&&... args) noexcept
	{
		static_assert(
			(correctColumnType<Args> && ...),
			"all arguments must be lvalue references or nullptr_t or bool*");
		return columnsImpl(0, args...);
	}

public:
	template<typename... Args>
	Result<bool> fetch(Args&... args) noexcept
	{
		auto rs = step();
		if(!rs) return unexpected(rs.error());

		auto rc = columns(args...);
		if(!rc) return unexpected(rc.error());

		return rs;
	}

public:
	::sqlite3_stmt* stmt;
};

struct Blob
{
public:
	Blob(::sqlite3_blob* blob) noexcept :
		blob(blob)
	{}

	Blob(const Blob&) = delete;
	Blob& operator=(const Blob&) = delete;
	Blob(Blob&& other) noexcept :
		blob(std::exchange(other.blob, nullptr))
	{}
	Blob& operator=(Blob&& other) & noexcept
	{
		blob = std::exchange(other.blob, nullptr);
		return *this;
	}
	~Blob() noexcept
	{
		if(blob) ::sqlite3_blob_close(blob);
	}

public:
	Result<void> reopen(std::int64_t rowid) noexcept
	{
		int rc = ::sqlite3_blob_reopen(blob, rowid);
		return check(rc);
	}

	Result<void> close() noexcept
	{
		if(!blob) return {};

		::sqlite3_blob* current = std::exchange(blob, nullptr);
		int rc = ::sqlite3_blob_close(current);
		return check(rc);
	}

	Result<void> read(int offset, std::span<std::byte> buffer) noexcept
	{
		int rc = ::sqlite3_blob_read(
			blob, buffer.data(), static_cast<int>(buffer.size()), offset);
		return check(rc);
	}

	Result<void> write(int offset, std::span<const std::byte> buffer) noexcept
	{
		int rc = ::sqlite3_blob_write(
			blob, buffer.data(), static_cast<int>(buffer.size()), offset);
		return check(rc);
	}

	int bytes() const noexcept { return ::sqlite3_blob_bytes(blob); }

public:
	::sqlite3_blob* blob;
};

struct Conn
{
public:
	Conn(::sqlite3* db) noexcept :
		db(db)
	{
		// extended result codes carry more detail than the primary ones
		if(db) ::sqlite3_extended_result_codes(db, 1);
	}

	Conn(const Conn&) = delete;
	Conn& operator=(const Conn&) = delete;
	Conn(Conn&& other) noexcept :
		db(std::exchange(other.db, nullptr))
	{}
	Conn& operator=(Conn&& other) & noexcept
	{
		db = std::exchange(other.db, nullptr);
		return *this;
	}
	~Conn() noexcept { close(); }

public:
	std::error_code lastError() const noexcept
	{
		if(!db) return makeCode(SQLITE_MISUSE);
		return makeCode(::sqlite3_extended_errcode(db));
	}

	void close() noexcept
	{
		if(!db) return;

		::sqlite3* current = std::exchange(db, nullptr);
		::sqlite3_close_v2(current);
	}

	void interrupt() noexcept
	{
		if(db) ::sqlite3_interrupt(db);
	}

public:
	enum openFlag : int
	{
		readonly      = SQLITE_OPEN_READONLY,
		readwrite     = SQLITE_OPEN_READWRITE,
		create        = SQLITE_OPEN_CREATE,
		deleteonclose = SQLITE_OPEN_DELETEONCLOSE,
		exclusive     = SQLITE_OPEN_EXCLUSIVE,
		autoproxy     = SQLITE_OPEN_AUTOPROXY,
		uri           = SQLITE_OPEN_URI,
		memory        = SQLITE_OPEN_MEMORY,
		main_db       = SQLITE_OPEN_MAIN_DB,
		temp_db       = SQLITE_OPEN_TEMP_DB,
		transient_db  = SQLITE_OPEN_TRANSIENT_DB,
		main_journal  = SQLITE_OPEN_MAIN_JOURNAL,
		temp_journal  = SQLITE_OPEN_TEMP_JOURNAL,
		subjournal    = SQLITE_OPEN_SUBJOURNAL,
		super_journal = SQLITE_OPEN_SUPER_JOURNAL,
		nomutex       = SQLITE_OPEN_NOMUTEX,
		fullmutex     = SQLITE_OPEN_FULLMUTEX,
		sharedcache   = SQLITE_OPEN_SHAREDCACHE,
		privatecache  = SQLITE_OPEN_PRIVATECACHE,
		wal           = SQLITE_OPEN_WAL,
		nofollow      = SQLITE_OPEN_NOFOLLOW,
		exrescode     = SQLITE_OPEN_EXRESCODE,
	};

	static Result<Conn> open(std::string_view path, int flags = readwrite | create | nomutex)
	{
		std::string filename(path);
		::sqlite3* conn = nullptr;
		int rc = ::sqlite3_open_v2(filename.c_str(), &conn, flags, nullptr);
		if(rc == SQLITE_OK) return conn;

		int code = conn ? ::sqlite3_extended_errcode(conn) : rc;
		if(conn) ::sqlite3_close_v2(conn);
		return makeError(code);
	}

	Result<Stmt> prepare(std::string_view sql)
	{
		::sqlite3_stmt* stmt = nullptr;
		int rc = ::sqlite3_prepare_v2(db, sql.data(), static_cast<int>(sql.size()), &stmt, nullptr);
		if(rc == SQLITE_OK) return stmt;
		return makeError(rc);
	}

	Result<void> exec(std::string_view sql)
	{
		std::string statement(sql);
		int rc = ::sqlite3_exec(db, statement.c_str(), nullptr, nullptr, nullptr);
		return check(rc);
	}

	// returns an empty string on success
	std::string execWithError(std::string_view sql)
	{
		std::string statement(sql);
		char* errMsg = nullptr;
		int rc = ::sqlite3_exec(db, statement.c_str(), nullptr, nullptr, &errMsg);
		if(rc == SQLITE_OK) return {};

		std::string message = errMsg ? errMsg : ::sqlite3_errstr(rc);
		::sqlite3_free(errMsg);
		return message;
	}

	Result<Blob> openBlob(
		std::string_view xdb,
		std::string_view table,
		std::string_view column,
		std::int64_t rowid,
		bool readOnly = true)
	{
		std::string database(xdb);
		std::string tableName(table);
		std::string columnName(column);
		::sqlite3_blob* blob = nullptr;
		int rc = ::sqlite3_blob_open(
			db, database.c_str(), tableName.c_str(), columnName.c_str(), rowid,
			readOnly ? 0 : 1, &blob);
		if(rc == SQLITE_OK) return Blob(blob);
		return makeError(rc);
	}

	Result<Blob> openBlob(
		std::string_view table,
		std::string_view column,
		std::int64_t rowid,
		bool readOnly = true)
	{
		// sqlite 不接受空 schema 名，默认落在 main
		return openBlob("main", table, column, rowid, readOnly);
	}

public:
	::sqlite3* db;
};
} // namespace JK::Wrap::Details::Sqlite

namespace JK::Wrap::Sqlite
{
using Details::Sqlite::category;
using Details::Sqlite::typeText;
using Details::Sqlite::Stmt;
using Details::Sqlite::Blob;
using Details::Sqlite::Conn;
} // namespace JK::Wrap::Sqlite
