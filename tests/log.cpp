#include <cassert>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>
#include <spdlog/sinks/base_sink.h>
#include <jk/wrap/log.h>

namespace Logs = JK::Wrap::Logs;
namespace Details = JK::Wrap::Details::Logs;

static_assert(Details::LoggerLike<spdlog::logger*>);
static_assert(Details::LoggerLike<std::shared_ptr<spdlog::logger>>);
static_assert(Details::LoggerLike<std::unique_ptr<spdlog::logger>>);
static_assert(Details::LoggerLike<std::shared_ptr<spdlog::logger>&>);

static_assert(!Details::LoggerLike<spdlog::logger>);
static_assert(!Details::LoggerLike<double*>);
static_assert(!Details::LoggerLike<spdlog::level::level_enum>);
static_assert(!Details::LoggerLike<std::string>);

struct CapturedMessage
{
	spdlog::level::level_enum level{};
	std::string message;
	std::string file;
	std::string function;
	int line{};
};

class CapturingSink final : public spdlog::sinks::base_sink<std::mutex>
{
public:
	std::vector<CapturedMessage> messages;

	void clear()
	{
		messages.clear();
	}

protected:
	void sink_it_(const spdlog::details::log_msg& msg) override
	{
		messages.push_back(CapturedMessage{
			msg.level,
			std::string(msg.payload.data(), msg.payload.size()),
			msg.source.filename ? msg.source.filename : "",
			msg.source.funcname ? msg.source.funcname : "",
			msg.source.line,
		});
	}

	void flush_() override {}
};

bool compiledIn(spdlog::level::level_enum level)
{
	return level >= Details::active;
}

void expectExactlyOne(const CapturingSink& sink, spdlog::level::level_enum level, std::string_view message)
{
	assert(sink.messages.size() == 1);
	if (sink.messages.size() != 1)
		return;

	const auto& captured = sink.messages.front();
	assert(captured.level == level);
	assert(captured.message == message);
}

void expectOne(const CapturingSink& sink, spdlog::level::level_enum level, std::string_view message)
{
	if (!compiledIn(level))
	{
		assert(sink.messages.empty());
		return;
	}
	expectExactlyOne(sink, level, message);
}

void testLevels(spdlog::logger* logger, CapturingSink& sink)
{
	sink.clear();
	Logs::trace(logger, "trace {}", 0);
	expectOne(sink, spdlog::level::trace, "trace 0");

	sink.clear();
	Logs::debug(logger, "debug {}", 1);
	expectOne(sink, spdlog::level::debug, "debug 1");

	sink.clear();
	Logs::info(logger, "info {}", 2);
	expectOne(sink, spdlog::level::info, "info 2");

	sink.clear();
	Logs::warn(logger, "warn {}", 3);
	expectOne(sink, spdlog::level::warn, "warn 3");

	sink.clear();
	Logs::err(logger, "err {}", 4);
	expectOne(sink, spdlog::level::err, "err 4");

	sink.clear();
	Logs::critical(logger, "critical {}", 5);
	expectOne(sink, spdlog::level::critical, "critical 5");
}

void testLevelsWithDefaultLogger(const std::shared_ptr<spdlog::logger>& logger, CapturingSink& sink)
{
	const auto previous = spdlog::default_logger();
	spdlog::set_default_logger(logger);

	sink.clear();
	Logs::trace("default trace {}", 0);
	expectOne(sink, spdlog::level::trace, "default trace 0");

	sink.clear();
	Logs::debug("default debug {}", 1);
	expectOne(sink, spdlog::level::debug, "default debug 1");

	sink.clear();
	Logs::info("default info {}", 2);
	expectOne(sink, spdlog::level::info, "default info 2");

	sink.clear();
	Logs::warn("default warn {}", 3);
	expectOne(sink, spdlog::level::warn, "default warn 3");

	sink.clear();
	Logs::err("default err {}", 4);
	expectOne(sink, spdlog::level::err, "default err 4");

	sink.clear();
	Logs::critical("default critical {}", 5);
	expectOne(sink, spdlog::level::critical, "default critical 5");

	spdlog::set_default_logger(previous);
}

void testDynamicLevel(const std::shared_ptr<spdlog::logger>& logger, CapturingSink& sink)
{
	sink.clear();
	Logs::log(logger, spdlog::level::warn, "dynamic {}", 1);
	expectExactlyOne(sink, spdlog::level::warn, "dynamic 1");

	sink.clear();
	Logs::log(logger, spdlog::level::trace, "dynamic {}", 2);
	expectExactlyOne(sink, spdlog::level::trace, "dynamic 2");

	const auto previous = spdlog::default_logger();
	spdlog::set_default_logger(logger);
	sink.clear();
	Logs::log(spdlog::level::err, "dynamic default {}", 3);
	expectExactlyOne(sink, spdlog::level::err, "dynamic default 3");
	spdlog::set_default_logger(previous);
}

void testFormatting(spdlog::logger* logger, CapturingSink& sink)
{
	sink.clear();
	Logs::info(logger, "plain message");
	expectOne(sink, spdlog::level::info, "plain message");

	sink.clear();
	Logs::info(logger, "mixed {} {} {} {} {}", 1, 2u, 3.5, "four", true);
	expectOne(sink, spdlog::level::info, "mixed 1 2 3.5 four true");

	const std::string_view view = "view";
	sink.clear();
	Logs::info(logger, "{}", view);
	expectOne(sink, spdlog::level::info, "view");
}

void testSourceLocation(spdlog::logger* logger, CapturingSink& sink)
{
	sink.clear();

	const std::source_location expected = std::source_location::current();
	const int line = __LINE__ + 1;
	Logs::info(logger, "located");

	if (sink.messages.size() != 1)
	{
		assert(!"exactly one message is expected");
		return;
	}

	const auto& captured = sink.messages.front();
	assert(captured.message == "located");
	assert(captured.file == expected.file_name());
	assert(captured.function == expected.function_name());
	assert(captured.line == line);
}

void testLoggerKinds(const std::shared_ptr<CapturingSink>& sinkPtr)
{
	auto& sink = *sinkPtr;

	auto shared = std::make_shared<spdlog::logger>("jk-test-shared", sinkPtr);
	shared->set_level(spdlog::level::trace);

	sink.clear();
	Logs::info(shared, "shared {}", 1);
	expectOne(sink, spdlog::level::info, "shared 1");

	sink.clear();
	Logs::info(shared.get(), "raw {}", 2);
	expectOne(sink, spdlog::level::info, "raw 2");

	auto owned = std::make_unique<spdlog::logger>("jk-test-unique", sinkPtr);
	owned->set_level(spdlog::level::trace);

	sink.clear();
	Logs::info(owned, "unique {}", 3);
	expectOne(sink, spdlog::level::info, "unique 3");
}

int main()
{
	auto sink = std::make_shared<CapturingSink>();
	auto logger = std::make_shared<spdlog::logger>("jk-test-log", sink);
	logger->set_level(spdlog::level::trace);

	testLoggerKinds(sink);
	testLevels(logger.get(), *sink);
	testFormatting(logger.get(), *sink);
	testSourceLocation(logger.get(), *sink);
	testDynamicLevel(logger, *sink);
	testLevelsWithDefaultLogger(logger, *sink);
}
