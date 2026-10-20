#pragma once
#include <source_location>
#include <spdlog/spdlog.h>

namespace JK::Wrap::Details::Logs
{
inline constexpr auto active = static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL);

template<typename Logger>
concept LoggerLike = std::same_as<
	typename std::pointer_traits<std::decay_t<Logger>>::element_type, 
	spdlog::logger
>;

template<typename... Args>
struct LoggingFormatStr
{
	template<std::convertible_to<std::string_view> StrLike>
	consteval LoggingFormatStr(
		const StrLike& str, 
		std::source_location location = std::source_location::current()) :
		str(str),
		location(location) 
	{}
	
	consteval LoggingFormatStr(
		std::source_location location = std::source_location::current()) :
		str(),
		location(location) 
	{}

	spdlog::format_string_t<Args...> str;
	std::source_location location;
};

template<typename... Args>
using LoggingFormat = LoggingFormatStr<std::type_identity_t<Args>...>;

template<LoggerLike Logger, typename... Args>
void logImpl(
	Logger&& logger,
	spdlog::level::level_enum level,
	std::source_location location,
	spdlog::format_string_t<Args...> fmt,
	Args&&... args)
{
	auto loc = spdlog::source_loc{
		location.file_name(),
		static_cast<int>(location.line()),
		location.function_name(),
	};
	logger->log(loc, level, fmt, std::forward<Args>(args)...);
}

template<spdlog::level::level_enum level, LoggerLike Logger, typename... Args>
void logWithLevel(Logger&& logger, LoggingFormat<Args...> fmt, Args&&... args)
{
	if constexpr (level >= active)
		logImpl(std::forward<Logger>(logger), level, fmt.location, fmt.str, std::forward<Args>(args)...);
}

template<spdlog::level::level_enum level, typename... Args>
void logWithLevel(LoggingFormat<Args...> fmt, Args&&... args)
{
	logWithLevel<level>(spdlog::default_logger_raw(), fmt, std::forward<Args>(args)...);
}

template<LoggerLike Logger, typename... Args>
void trace(Logger&& logger, LoggingFormat<Args...> fmt, Args&&... args) 
{
	logWithLevel<spdlog::level::trace>(std::forward<Logger>(logger), fmt, std::forward<Args>(args)...);
}

template<LoggerLike Logger, typename... Args>
void debug(Logger&& logger, LoggingFormat<Args...> fmt, Args&&... args) 
{
	logWithLevel<spdlog::level::debug>(std::forward<Logger>(logger), fmt, std::forward<Args>(args)...);
}

template<LoggerLike Logger, typename... Args>
void info(Logger&& logger, LoggingFormat<Args...> fmt, Args&&... args) 
{
	logWithLevel<spdlog::level::info>(std::forward<Logger>(logger), fmt, std::forward<Args>(args)...);
}

template<LoggerLike Logger, typename... Args>
void warn(Logger&& logger, LoggingFormat<Args...> fmt, Args&&... args) 
{
	logWithLevel<spdlog::level::warn>(std::forward<Logger>(logger), fmt, std::forward<Args>(args)...);
}

template<LoggerLike Logger, typename... Args>
void err(Logger&& logger, LoggingFormat<Args...> fmt, Args&&... args) 
{
	logWithLevel<spdlog::level::err>(std::forward<Logger>(logger), fmt, std::forward<Args>(args)...);
}

template<LoggerLike Logger, typename... Args>
void critical(Logger&& logger, LoggingFormat<Args...> fmt, Args&&... args) 
{
	logWithLevel<spdlog::level::critical>(std::forward<Logger>(logger), fmt, std::forward<Args>(args)...);
}

template<LoggerLike Logger, typename... Args>
void log(Logger&& logger, spdlog::level::level_enum level, LoggingFormat<Args...> fmt, Args&&... args)
{
	logImpl(std::forward<Logger>(logger), level, fmt.location, fmt.str, std::forward<Args>(args)...);
}

template<typename... Args>
void trace(LoggingFormat<Args...> fmt, Args&&... args) 
{
	logWithLevel<spdlog::level::trace>(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void debug(LoggingFormat<Args...> fmt, Args&&... args) 
{
	logWithLevel<spdlog::level::debug>(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void info(LoggingFormat<Args...> fmt, Args&&... args) 
{
	logWithLevel<spdlog::level::info>(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void warn(LoggingFormat<Args...> fmt, Args&&... args) 
{
	logWithLevel<spdlog::level::warn>(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void err(LoggingFormat<Args...> fmt, Args&&... args) 
{
	logWithLevel<spdlog::level::err>(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void critical(LoggingFormat<Args...> fmt, Args&&... args) 
{
	logWithLevel<spdlog::level::critical>(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void log(spdlog::level::level_enum level, LoggingFormat<Args...> fmt, Args&&... args)
{
	logImpl(spdlog::default_logger_raw(), level, fmt.location, fmt.str, std::forward<Args>(args)...);
}
} // namespace JK::Wrap::Detail::Logs

namespace JK::Wrap::Logs
{
using Details::Logs::active;
using Details::Logs::trace;
using Details::Logs::debug;
using Details::Logs::info;
using Details::Logs::warn;
using Details::Logs::err;
using Details::Logs::critical;
using Details::Logs::log;
}
