#pragma once
#include <string_view>
#include <format>
#include <span>

namespace JK::Wrap::Details::Strings
{
using namespace std::literals;

inline constexpr auto whitespace = " \f\n\r\t\v"sv;

constexpr std::string_view trim(std::string_view s) noexcept
{
    auto b = s.find_first_not_of(whitespace);
    if(b == std::string_view::npos) return {};
    auto e = s.find_last_not_of(whitespace);
    return s.substr(b, e - b + 1);
}

constexpr std::string_view trimLeft(std::string_view s) noexcept
{
    auto b = s.find_first_not_of(whitespace);
    if(b == std::string_view::npos) return {};
    return s.substr(b);
}

constexpr std::string_view trimRight(std::string_view s) noexcept
{
    auto e = s.find_last_not_of(whitespace);
    if(e == std::string_view::npos) return {};
    return s.substr(0, e + 1);
}

constexpr std::string_view removePrefix(std::string_view s, std::string_view prefix) noexcept
{
    if(s.starts_with(prefix)) return s.substr(prefix.size());
    return s;
}

constexpr std::string_view removeSuffix(std::string_view s, std::string_view suffix) noexcept
{
    if(s.ends_with(suffix)) return s.substr(0, s.size() - suffix.size());
    return s;
}

template<typename It, typename Sentinel, typename Char>
struct JoinRange
{
	It begin;
	Sentinel end;
	std::basic_string_view<Char> sep;

	JoinRange(It b, Sentinel e, std::basic_string_view<Char> s) : begin(b), end(e), sep(s) {}
};

template<typename It, typename Sentinel, typename Char>
auto join(It begin, Sentinel end, std::basic_string_view<Char> sep)
{
	return JoinRange<It, Sentinel, Char>(begin, end, sep);
}

template<typename Range, typename Char>
auto join(Range&& range, std::basic_string_view<Char> sep)
{
	return join(std::begin(range), std::end(range), sep);
}
} // namespace JK::Wrap::Details::Strings

template<typename It, typename Sentinel, typename Char>
struct std::formatter<JK::Wrap::Details::Strings::JoinRange<It, Sentinel, Char>, Char> :
	std::formatter<typename std::iterator_traits<It>::value_type, Char>
{
	template<typename FormatContext>
	auto format(JK::Wrap::Details::Strings::JoinRange<It, Sentinel, Char> value, FormatContext& ctx) const
	{
		using base = formatter<typename std::iterator_traits<It>::value_type, Char>;
		auto it = value.begin;
		auto out = ctx.out();
		if(it != value.end)
		{
			out = base::format(*it++, ctx);
			while(it != value.end)
			{
				out = std::copy(value.sep.begin(), value.sep.end(), out);
				ctx.advance_to(out);
				out = base::format(*it++, ctx);
			}
		}
		return out;
	}
};

namespace JK::Wrap::Details::Strings
{
template<bool upper = false, typename T, std::size_t extent>
requires std::is_trivially_copyable_v<T>
std::string hex(std::span<T, extent> elems, std::string_view sep = {})
{
    using Span = std::span<const char, extent == std::dynamic_extent ? extent : sizeof(T) * extent>;
    auto bytes = Span{reinterpret_cast<const char*>(elems.data()), elems.size_bytes()};

    std::string result;
    result.reserve(2 * bytes.size() + sep.size() * (bytes.size() - 1));
    constexpr auto fmt = upper ? "{:02X}" : "{:02x}";
    std::format_to(std::back_inserter(result), fmt, join(bytes, sep));
    return result;
}

template<bool upper = false>
std::string hex(std::string_view bytes, std::string_view sep = {})
{
    return hex<upper>(std::span(bytes), sep);
}

inline std::string lower(std::string_view s)
{
	std::string result;
	result.reserve(s.size());
	for(char c : s) result.push_back(std::tolower(static_cast<unsigned char>(c)));
	return result;
}

inline void lower(std::string& s) noexcept
{
	for(char& c : s) c = std::tolower(static_cast<unsigned char>(c));
}

inline std::string upper(std::string_view s)
{
	std::string result;
	result.reserve(s.size());
	for(char c : s) result.push_back(std::toupper(static_cast<unsigned char>(c)));
	return result;
}

inline void upper(std::string& s) noexcept
{
	for(char& c : s) c = std::toupper(static_cast<unsigned char>(c));
}
} // namespace JK::Wrap::Details::Strings

namespace JK::Wrap
{
using Details::Strings::trim;
using Details::Strings::trimLeft;
using Details::Strings::trimRight;
using Details::Strings::removePrefix;
using Details::Strings::removeSuffix;
using Details::Strings::join;
using Details::Strings::hex;
using Details::Strings::lower;
using Details::Strings::upper;
} // namespace JK::Wrap
