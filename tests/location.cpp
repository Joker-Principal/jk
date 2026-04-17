#include <cassert>
#include <tuple>
#include <jk/wrap/location.h>

using namespace JK::Wrap;

template<Location loc>
consteval auto foo(std::source_location expect = std::source_location::current())
{
    return std::tuple(loc, expect);
}

int main()
{
	{
		auto [loc, expect] = foo<where>();
        auto [file, line] = loc;
        assert(line == expect.line());
        assert(std::string_view(expect.file_name()).ends_with(file));
	}
	{
		auto [loc, expect] = foo<where>();
        auto [file, line] = loc;
        assert(line == expect.line());
        assert(std::string_view(expect.file_name()).ends_with(file));
	}
}