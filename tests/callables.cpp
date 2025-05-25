#include <jk/meta/callables.h>

struct A 
{
    constexpr void memberFunction(int, int) {}
    constexpr int constMemberFunction(int) const { return 42; }
    constexpr double noexceptFunction(int, int) noexcept { return 3.14; }

    constexpr static void staticFunction(int, int) {}

    int memberData{0};
};

int main()
{
    using namespace JK::Meta;

    using MemFunc = CallableOf<&A::memberFunction>;
    static_assert(MemFunc::isVariadic == false);
    static_assert(MemFunc::isNothrow == false);
    static_assert(std::is_same_v<MemFunc::Class, A>);
    static_assert(std::is_same_v<MemFunc::Return, void>);
    static_assert(std::is_same_v<MemFunc::Arguments, TList<int, int>>);
    static_assert(std::is_same_v<MemFunc::Invoker, void(int, int)>);

    using ConstMemFunc = CallableOf<&A::constMemberFunction>;
    static_assert(std::is_same_v<ConstMemFunc::Class, const A>);

    using NoExceptFunc = CallableOf<&A::noexceptFunction>;
    static_assert(NoExceptFunc::isNothrow == true);

    using StaticFunc = CallableOf<&A::staticFunction>;
    static_assert(std::is_same_v<StaticFunc::Class, void>);
    
    constexpr A ins;
    constexpr auto x = std::invoke(ConstMemFunc::function, ins, 1);
    static_assert(x == 42);
}
