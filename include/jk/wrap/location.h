#include <source_location>
#include <string_view>

namespace JK::Wrap
{
struct Where {} inline constexpr where;

struct Location 
{
public:
    consteval Location(std::source_location loc = std::source_location::current()) noexcept : 
        file(), line(loc.line())
    {
		auto file_size = std::char_traits<char>::length(loc.file_name());
		auto size = std::min(sizeof(file) - 1, file_size);
		auto begin = loc.file_name() + file_size - size;
		std::char_traits<char>::copy(file, begin, size);
    }

    consteval Location(Where, std::source_location loc = std::source_location::current()) noexcept :
        Location(loc)
    {}

public:
	char file[256];
	int line;
};
} // namespace JK::Wrap
