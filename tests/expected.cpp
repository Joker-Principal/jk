
#if !defined(__cpp_lib_expected) || __cpp_lib_expected < 202202L

#include <cassert>
#include <memory>
#include <string>
#include <system_error>
#include <jk/wrap/expected.h>

std::error_code testError()
{
	return make_error_code(std::errc::invalid_argument);
}

int main()
{
	using JK::Wrap::bad_expected_access;
	using JK::Wrap::expected;
	using JK::Wrap::unexpected;

	// 默认构造
	{
		expected<int, std::error_code> value;
		assert(value.has_value());
		assert(static_cast<bool>(value));
		assert(value.value() == 0);
		assert(*value == 0);
	}

	// 值构造 + operator->
	{
		expected<std::string, std::error_code> value = std::string("hello");
		assert(value->size() == 5);
		assert(*value == "hello");
	}

	// in_place 构造
	{
		expected<std::string, std::error_code> value(std::in_place, 3, 'x');
		assert(*value == "xxx");
	}

	// unexpected + error()
	{
		expected<int, std::error_code> value = unexpected(testError());
		assert(!value);
		assert(value.error() == testError());
		assert(value.value_or(7) == 7);
		assert(value.error_or(std::error_code{}) == testError());
	}

	// 成功时的 value_or / error_or
	{
		expected<int, std::error_code> value = 42;
		assert(value.value_or(7) == 42);
		assert(value.error_or(testError()) == testError());
		assert(value.error_or(std::error_code{}) == std::error_code{});
	}

	// value() 在错误状态抛异常
	{
		expected<int, std::error_code> value = unexpected(testError());
		bool thrown = false;
		try
		{
			(void)value.value();
		}
		catch(const bad_expected_access<std::error_code>& err)
		{
			thrown = true;
			assert(err.error() == testError());
		}
		assert(thrown);
	}

	// 只能移动的值类型
	{
		expected<std::unique_ptr<int>, std::error_code> value = std::make_unique<int>(5);
		assert(value.has_value());
		assert(**value == 5);

		auto moved = std::move(value).value();
		assert(*moved == 5);
	}

	// void 特化
	{
		expected<void, std::error_code> ok;
		assert(ok.has_value());
		ok.value();

		expected<void, std::error_code> err = unexpected(testError());
		assert(!err);
		assert(err.error() == testError());
		assert(err.error_or(std::error_code{}) == testError());
	}

	// and_then / transform
	{
		expected<int, std::error_code> value = 2;
		auto doubled = value.and_then([](int v) { return expected<int, std::error_code>(v * 2); });
		assert(doubled.has_value());
		assert(*doubled == 4);

		auto text = value.transform([](int v) { return std::to_string(v); });
		assert(text.has_value());
		assert(*text == "2");

		expected<int, std::error_code> failed = unexpected(testError());
		auto propagated = failed.and_then([](int v) { return expected<int, std::error_code>(v); });
		assert(!propagated);
		assert(propagated.error() == testError());
	}

	// or_else / transform_error
	{
		expected<int, std::error_code> failed = unexpected(testError());
		auto recovered = failed.or_else([](std::error_code) { return expected<int, std::error_code>(9); });
		assert(recovered.has_value());
		assert(*recovered == 9);

		auto mapped = failed.transform_error([](std::error_code) { return 5; });
		assert(!mapped);
		assert(mapped.error() == 5);

		expected<int, std::error_code> value = 3;
		auto kept = value.or_else([](std::error_code) { return expected<int, std::error_code>(0); });
		assert(kept.has_value());
		assert(*kept == 3);
	}

	// void 特化的 Monadic 接口
	{
		expected<void, std::error_code> ok;
		auto built = ok.and_then([] { return expected<int, std::error_code>(1); });
		assert(built.has_value());
		assert(*built == 1);

		auto transformed = ok.transform([] { return std::string("done"); });
		assert(transformed.has_value());
		assert(*transformed == "done");
	}

	// 拷贝 / 移动
	{
		expected<std::string, std::error_code> original = std::string("copy");
		auto copy = original;
		assert(*copy == "copy");

		auto moved = std::move(original);
		assert(*moved == "copy");
	}

	// rvalue 限定重载
	{
		expected<std::string, std::error_code> value = std::string("move-me");
		auto taken = std::move(value).value();
		assert(taken == "move-me");
	}
}

#else
int main() {}
#endif
