#include <jk/wrap/string.h>
#include <cassert>

int main()
{
    using namespace JK::Wrap;
    using namespace std::literals;

    // trim
    static_assert(trim("  \t\nabc\r\f\v  ") == "abc");
    static_assert(trimLeft("  \t\nabc\r\f\v  ") == "abc\r\f\v  ");
    static_assert(trimRight("  \t\nabc\r\f\v  ") == "  \t\nabc");

    // removePrefix/removeSuffix
    static_assert(removePrefix("foobar", "foo") == "bar");
    static_assert(removePrefix("foobar", "bar") == "foobar");
    static_assert(removeSuffix("foobar", "bar") == "foo");
    static_assert(removeSuffix("foobar", "foo") == "foobar");

    // hex
    std::string s = "hello";
    assert(hex(s) == "68656c6c6f"sv);
    assert(hex(s, "-") == "68-65-6c-6c-6f"sv);
}
