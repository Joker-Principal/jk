#include <jk/meta/callables.h>

using namespace JK::Meta;

#define CONCAT(prefix, cv, ref, nothrow) prefix ## cv ## ref ## nothrow

#define REF_NAME(ref) JK_SELECT(ref, _n, _l, _r)
#define CV_NAME(cv) JK_SELECT(cv, _m, _c, _v, _cv)
#define NOTHROW_NAME(nothrow) JK_SELECT(nothrow, _t, _nt)

#define NAME(cv, ref, nothrow) JK_APPLY(CONCAT, test, CV_NAME(cv), REF_NAME(ref), NOTHROW_NAME(nothrow))
#define CO_NAME(cv, ref, nothrow) JK_APPLY(CONCAT, co, CV_NAME(cv), REF_NAME(ref), NOTHROW_NAME(nothrow))
#define RET_NAME(cv, ref, nothrow) class JK_APPLY(CONCAT, R, CV_NAME(cv), REF_NAME(ref), NOTHROW_NAME(nothrow))*
#define ARG_NAME(cv, ref, nothrow) class JK_APPLY(CONCAT, A, CV_NAME(cv), REF_NAME(ref), NOTHROW_NAME(nothrow))*

#define MEM(cv, ref, nothrow) \
    RET_NAME(cv, ref, nothrow) NAME(cv, ref, nothrow)(ARG_NAME(cv, ref, nothrow)) JK_CALLABLE_GET_CV(cv) JK_CALLABLE_GET_REF(ref) JK_CALLABLE_GET_NOEXCEPT(nothrow) { return 0; }

#define CHECK(cv, ref, nothrow) \
    using CO_NAME(cv, ref, nothrow) = CallableOf<&TestCallableOf::NAME(cv, ref, nothrow)>; \
    static_assert(CO_NAME(cv, ref, nothrow)::isVariadic == false); \
    static_assert(CO_NAME(cv, ref, nothrow)::isNothrow == JK_SELECT(nothrow, false, true)); \
    static_assert(std::is_same_v<CO_NAME(cv, ref, nothrow)::Class, JK_CALLABLE_GET_CV(cv) TestCallableOf JK_CALLABLE_GET_REF(ref)>); \
    static_assert(std::is_same_v<CO_NAME(cv, ref, nothrow)::Return, RET_NAME(cv, ref, nothrow)>); \
    static_assert(std::is_same_v<CO_NAME(cv, ref, nothrow)::Arguments, TList<ARG_NAME(cv, ref, nothrow)>>); \

struct TestCallableOf
{
    JK_CALLABLE_MEM(MEM)
    constexpr static void staticFunction(int, int) {}
    int memberData{0};
    const int constMemberData{0};
};

JK_CALLABLE_MEM(CHECK)

using StaticFunc = CallableOf<&TestCallableOf::staticFunction>;
static_assert(std::is_same_v<StaticFunc::Class, void>);

using DataMember = CallableOf<&TestCallableOf::memberData>;
static_assert(std::is_same_v<DataMember::Class, TestCallableOf*>);
static_assert(std::is_same_v<DataMember::Return, int>);

using ConstDataMember = CallableOf<&TestCallableOf::constMemberData>;
static_assert(std::is_same_v<ConstDataMember::Class, TestCallableOf*>);
static_assert(std::is_same_v<ConstDataMember::Return, const int>);

template<bool isNothrow>
struct B 
{
    using Invoker = void(int, int) noexcept(isNothrow);
};

using B1 = B<false>;
using B2 = B<true>;

template<bool variadic, bool nothrow, typename R, typename... Args>
struct C
{
    static constexpr bool isVariadic = variadic;
    static constexpr bool isNothrow = nothrow;
    using Invoker = std::conditional_t<!variadic, R(Args...) noexcept(isNothrow), R(Args..., ...) noexcept(isNothrow)>;
};

using C1 = C<false, false, void, int>;
using C2 = C<true, false, void, int>;
using C3 = C<false, true, void, int>;
using C4 = C<true, true, void, int>;

static_assert(C1::isVariadic == false);
static_assert(C1::isNothrow == false);

struct D 
{
    void foo(double) {}
    friend bool operator==(const D&, int) { return false; }
};

struct E : D {};

void foo1() {}
void foo2() {}
static_assert(fn<foo1> == fn<foo2>);

struct N {
    N(N&&) = default;
    N(const N&) = delete;

};

template<typename R, typename... Args>
struct X {
    constexpr X(R(*f)(Args...)) : f(f) {}
    R (*f)(Args...);
};

void xxx(N);

void yyy(N* n) {
    auto v = X(xxx);
}
constexpr int xx(int x) { return 1; }
constexpr int yy(int x) { return 2; }

void foo() 
{
	auto x = Visitor([] {});
	struct A {
		constexpr A() = default;
		A(const A&) = delete;
		A(A&&) = default;
		constexpr int operator()(int, int) const { return 3; }
	};
	constexpr auto yl = Visitor(A{});
	constexpr auto ya = Visitor(A{}, [](int) { return 4; });


	struct X : GenericFunctor<A>, GenericFunctor<int(*)(int)> {
		constexpr X() : 
			GenericFunctor<A>(A{}),
			GenericFunctor<int(*)(int)>(xx) 
		{}

		using GenericFunctor<int(*)(int)>::operator();
		using GenericFunctor<A>::operator();
	} constexpr xxx;

	constexpr auto vx = yl(1, 1);
}

struct D_bind {
	constexpr int foo(int) const { return 0; }
};

template<typename T, typename U>
void mem(U T::*) {}

void foo_bind() {
	static constexpr D_bind d;
	constexpr auto fz = bind<&D_bind::foo>(d);
	constexpr auto fz2 = bind(d, fn<&D_bind::foo>);

	constexpr auto f2 = bind(d, &D_bind::foo);
	constexpr auto r2 = f2(1);

    mem(&D_bind::foo);
}

void func_ref()
{
    static constexpr auto func = [](int x) { return x + 1; };
    constexpr auto f = FunctionRef<int(int)>(func);

    static constexpr auto d = D_bind();
    constexpr auto f2 = FunctionRef<int(int)>(&d, fn<&D_bind::foo>);
    constexpr bool vv = 
        fn<&D_bind::foo>.value.isCompatibleWith<int(const D_bind&, int)>;

    
    constexpr auto value = CallableValue(&D_bind::foo);
    using F = decltype(value)::Function;
    constexpr bool vvc = std::is_invocable_r_v<int, F, const D_bind&, int>;
    using RF = decltype(value)::RawFunction;
    constexpr bool vvr = std::is_invocable_r_v<int, RF, const D_bind&, int>;
}


int main()
{
    using x = std::decay_t<int(&)()>;
    bool qq = std::is_same_v<int(), int(&)()>;

    using F = std::remove_pointer_t<std::decay_t<decltype(&D::foo)>>;
    bool b = std::is_function_v<F>;


    D d{};
    E e;
    e == 1;

    bind(d, fn<&D::foo>)(1);

    auto f1 = [] {};
    auto f2 = [] {};


    return 0;
}

