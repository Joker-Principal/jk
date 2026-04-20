#include <jk/meta/callables.h>
#include <cassert>
#include <functional>
#include <string>

using namespace JK::Meta;

// ============================================================
// Section 1: Callable traits for member functions
//   Verify that Callable<R(T::*)(Args...) cv ref noexcept>
//   correctly extracts Class, Return, Arguments, isVariadic, isNothrow.
// ============================================================

#define CONCAT(prefix, cv, ref, nothrow) prefix ## cv ## ref ## nothrow

#define REF_NAME(ref) JK_SELECT(ref, _n, _l, _r)
#define CV_NAME(cv)   JK_SELECT(cv, _m, _c, _v, _cv)
#define NOTHROW_NAME(nothrow) JK_SELECT(nothrow, _t, _nt)

#define NAME(cv, ref, nothrow)    JK_APPLY(CONCAT, test, CV_NAME(cv), REF_NAME(ref), NOTHROW_NAME(nothrow))
#define CO_NAME(cv, ref, nothrow) JK_APPLY(CONCAT, co,   CV_NAME(cv), REF_NAME(ref), NOTHROW_NAME(nothrow))
#define RET_NAME(cv, ref, nothrow) class JK_APPLY(CONCAT, R, CV_NAME(cv), REF_NAME(ref), NOTHROW_NAME(nothrow))*
#define ARG_NAME(cv, ref, nothrow) class JK_APPLY(CONCAT, A, CV_NAME(cv), REF_NAME(ref), NOTHROW_NAME(nothrow))*

// Declare one member function per (cv, ref, nothrow) combination
#define DECL_MEM(cv, ref, nothrow) \
    RET_NAME(cv, ref, nothrow) NAME(cv, ref, nothrow)(ARG_NAME(cv, ref, nothrow)) \
        JK_CALLABLE_GET_CV(cv) JK_CALLABLE_GET_REF(ref) JK_CALLABLE_GET_NOEXCEPT(nothrow) { return nullptr; }

// Verify CallableOf traits for each combination
#define CHECK_MEM(cv, ref, nothrow) \
    using CO_NAME(cv, ref, nothrow) = CallableOf<&TestMemFn::NAME(cv, ref, nothrow)>; \
    static_assert(CO_NAME(cv, ref, nothrow)::isVariadic == false); \
    static_assert(CO_NAME(cv, ref, nothrow)::isNothrow == JK_SELECT(nothrow, false, true)); \
    static_assert(std::is_same_v<CO_NAME(cv, ref, nothrow)::Class, \
        JK_CALLABLE_GET_CV(cv) TestMemFn JK_CALLABLE_GET_REF(ref)>); \
    static_assert(std::is_same_v<CO_NAME(cv, ref, nothrow)::Return, RET_NAME(cv, ref, nothrow)>); \
    static_assert(std::is_same_v<CO_NAME(cv, ref, nothrow)::Arguments, TList<ARG_NAME(cv, ref, nothrow)>>);

struct TestMemFn
{
    JK_CALLABLE_MEM(DECL_MEM)

    // static function: Class == void
    static void staticFn(int, int) noexcept {}

    // member data: Class == TestMemFn*, Return == int/const int
    int memberData{};
    const int constMemberData{};
};

JK_CALLABLE_MEM(CHECK_MEM)

// static function pointer
using StaticFnCallable = CallableOf<&TestMemFn::staticFn>;
static_assert(StaticFnCallable::isVariadic == false);
static_assert(StaticFnCallable::isNothrow == true);
static_assert(std::is_same_v<StaticFnCallable::Class, void>);
static_assert(std::is_same_v<StaticFnCallable::Return, void>);
static_assert(std::is_same_v<StaticFnCallable::Arguments, TList<int, int>>);

// member data pointer
using DataMember = CallableOf<&TestMemFn::memberData>;
static_assert(std::is_same_v<DataMember::Class, TestMemFn*>);
static_assert(std::is_same_v<DataMember::Return, int>);
static_assert(DataMember::isNothrow == true);

using ConstDataMember = CallableOf<&TestMemFn::constMemberData>;
static_assert(std::is_same_v<ConstDataMember::Class, TestMemFn*>);
static_assert(std::is_same_v<ConstDataMember::Return, const int>);

// ============================================================
// Section 2: Callable traits for free functions and functors
// ============================================================

// Free function pointer
static void freeFn(int, double) noexcept {}
using FreeFnCallable = Callable<decltype(&freeFn)>;
static_assert(std::is_same_v<FreeFnCallable::Class, void>);
static_assert(std::is_same_v<FreeFnCallable::Return, void>);
static_assert(std::is_same_v<FreeFnCallable::Arguments, TList<int, double>>);
static_assert(FreeFnCallable::isVariadic == false);
static_assert(FreeFnCallable::isNothrow == true);

// Variadic free function pointer
static int variadicFn(int, ...) { return 0; }
using VariadicFnCallable = Callable<decltype(&variadicFn)>;
static_assert(VariadicFnCallable::isVariadic == true);
static_assert(VariadicFnCallable::isNothrow == false);
static_assert(std::is_same_v<VariadicFnCallable::Return, int>);
static_assert(std::is_same_v<VariadicFnCallable::Arguments, TList<int>>);

// Lambda / functor
static auto lambdaFn = [](int x, double y) -> float { return static_cast<float>(x + y); };
using LambdaCallable = Callable<decltype(lambdaFn)>;
static_assert(std::is_same_v<LambdaCallable::Return, float>);
static_assert(std::is_same_v<LambdaCallable::Arguments, TList<int, double>>);
static_assert(LambdaCallable::isVariadic == false);

// std::function
using StdFuncCallable = Callable<std::function<int(char)>>;
static_assert(std::is_same_v<StdFuncCallable::Return, int>);
static_assert(std::is_same_v<StdFuncCallable::Arguments, TList<char>>);

// ============================================================
// Section 3: Callable helper aliases (Return, Class, Arguments, Signature, isVariadic, isNothrow)
// ============================================================

struct HelpAlias { int foo(double) const noexcept { return 0; } };

static_assert(std::is_same_v<Return<decltype(&HelpAlias::foo)>, int>);
static_assert(std::is_same_v<Class<decltype(&HelpAlias::foo)>, const HelpAlias>);
static_assert(std::is_same_v<Arguments<decltype(&HelpAlias::foo)>, TList<double>>);
static_assert(std::is_same_v<Signature<decltype(&HelpAlias::foo)>, int(double) noexcept>);
static_assert(isVariadic<decltype(&HelpAlias::foo)> == false);
static_assert(isNothrow<decltype(&HelpAlias::foo)> == true);

// ============================================================
// Section 4: CallableValue, CallableOf, fn, and compile-time aliases
// ============================================================

struct CvStruct { constexpr int bar(int x) const noexcept { return x * 2; } };

// fn<> creates a CallableOf instance
constexpr auto fnBar = fn<&CvStruct::bar>;
static_assert(std::is_same_v<decltype(fnBar)::Return, int>);
static_assert(std::is_same_v<decltype(fnBar)::Arguments, TList<int>>);

// ReturnOf / ClassOf / ArgumentsOf / SignatureOf
static_assert(std::is_same_v<ReturnOf<&CvStruct::bar>, int>);
static_assert(std::is_same_v<ClassOf<&CvStruct::bar>, const CvStruct>);
static_assert(std::is_same_v<ArgumentsOf<&CvStruct::bar>, TList<int>>);
static_assert(std::is_same_v<SignatureOf<&CvStruct::bar>, int(int) noexcept>);

// fn equality: two functions with the same signature compare equal via fn<>
constexpr int freeCmp1(int) { return 1; }
constexpr int freeCmp2(int) { return 2; }
constexpr double freeCmp3(int) { return 1.0; }
static_assert(fn<freeCmp1> == fn<freeCmp2>); // same signature int(int) => equal
static_assert(fn<freeCmp1> != fn<freeCmp3>); // int(int) vs double(int) => not equal

// CallableValue stores the function and exposes isCompatibleWith
constexpr auto cvBar = CallableValue(&CvStruct::bar);
static_assert(cvBar.isCompatibleWith<int(const CvStruct&, int)>);
static_assert(!cvBar.isCompatibleWith<int(int)>);

// ============================================================
// Section 5: Callable::Expand, ExpandClass, ExpandAll, Replace*
// ============================================================

struct ExpandTest { double baz(float) volatile noexcept {} };
using BazCallable = Callable<decltype(&ExpandTest::baz)>;

// Expand<E> => E<R, Args...>
template<typename R, typename... Args>
using MySig = R(*)(Args...);
using ExpandedSig = BazCallable::Expand<MySig>;
static_assert(std::is_same_v<ExpandedSig, double(*)(float)>);

// ReplaceReturn
using BazInt = BazCallable::ReplaceReturn<int>;
static_assert(std::is_same_v<BazInt::Return, int>);

// ReplaceArgs
using BazNewArgs = BazCallable::ReplaceArgs<int, char>;
static_assert(std::is_same_v<BazNewArgs::Arguments, TList<int, char>>);

// ReplaceVariadic / ReplaceNothrow
static_assert(BazCallable::ReplaceVariadic<true>::isVariadic == true);
static_assert(BazCallable::ReplaceNothrow<false>::isNothrow == false);

// ============================================================
// Section 6: invokeAs, returnAs, argAs
// ============================================================

void test_invokeAs()
{
    // --- baseline: generic lambda, return type deduced per instantiation ---
    auto generic = [](auto x) { return x + 1; };
    static_assert(std::is_same_v<decltype(generic(0)),   int>);
    static_assert(std::is_same_v<decltype(generic(0.0)), double>);
    assert(generic(3)   == 4);
    assert(generic(0.5) == 1.5);

    // invokeAs: pin to int(int) — argument and return type both fixed
    auto typed = generic | invokeAs<int(int)>;
    static_assert(std::is_same_v<Return<decltype(typed)>,    int>);
    static_assert(std::is_same_v<Arguments<decltype(typed)>, TList<int>>);
    assert(typed(3) == 4);

    // --- baseline: non-generic lambda with deduced (int) return type ---
    auto intFn = [](int x) { return x + 1; };
    static_assert(std::is_same_v<Return<decltype(intFn)>, int>);
    assert(intFn(2) == 3);

    // returnAs: change return type to double, arguments (int) are preserved
    auto asDouble = intFn | returnAs<double>;
    static_assert(std::is_same_v<Return<decltype(asDouble)>,    double>);
    static_assert(std::is_same_v<Arguments<decltype(asDouble)>, TList<int>>);
    assert(asDouble(2) == 3.0);

    // --- baseline: generic binary lambda, args and return all deduced ---
    auto binaryFn = [](auto x, auto y) { return x + y; };
    static_assert(std::is_same_v<decltype(binaryFn(1, 2)),     int>);
    static_assert(std::is_same_v<decltype(binaryFn(1.0, 2.0)), double>);
    assert(binaryFn(1, 2) == 3);

    // argAs: pin argument types to (int, int), return type still deduced
    auto typedArgs = binaryFn | argAs<int, int>;
    static_assert(std::is_same_v<Return<decltype(typedArgs)>,    int>);
    static_assert(std::is_same_v<Arguments<decltype(typedArgs)>, TList<int, int>>);
    assert(typedArgs(1, 2) == 3);
}

// ============================================================
// Section 7: GenericFunctor and Visitor
// ============================================================

void test_visitor()
{
    // Visitor combines multiple callables via overload
    auto v = Visitor(
        [](int x)    { return x * 2; },
        [](double x) { return x + 0.5; }
    );
    assert(v(3) == 6);
    assert(v(1.0) == 1.5);

    // GenericFunctor wraps a single callable
    auto gf = GenericFunctor([](int x) { return x + 10; });
    assert(gf(5) == 15);
}

// ============================================================
// Section 8: curry
// ============================================================

void test_curry()
{
    auto add = [](int a, int b, int c) { return a + b + c; };
    auto add1 = curry(add, 1);
    auto add12 = curry(add1, 2);
    assert(add12(3) == 6);
    assert(curry(add, 10, 20)(30) == 60);
}

// ============================================================
// Section 9: bind (zero-cost and two-pointer variants)
// ============================================================

struct BindTest
{
    constexpr int mul(int x) const noexcept { return x * 3; }
    int value{5};
};

void test_bind()
{
    static constexpr BindTest bt;

    // zero-cost bind via template parameter (size == one pointer)
    constexpr auto f1 = bind<&BindTest::mul>(&bt);
    static_assert(sizeof(f1) == sizeof(void*));
    assert(f1(4) == 12);

    // bind by ref
    constexpr auto f2 = bind<&BindTest::mul>(bt);
    assert(f2(2) == 6);

    // bind with fn<> helper
    constexpr auto f3 = bind(bt, fn<&BindTest::mul>);
    assert(f3(3) == 9);

    // two-pointer bind (runtime member pointer)
    auto f4 = bind(bt, &BindTest::mul);
    static_assert(sizeof(f4) == 2 * sizeof(void*));
    assert(f4(5) == 15);
}

// ============================================================
// Section 10: unbind
// ============================================================

struct UnbindTest
{
    int add(int x) noexcept { return x + value; }
    int addConst(int x) const noexcept { return x + value; }
    int value{7};
};

void test_unbind()
{
    constexpr auto ub = unbind<&UnbindTest::add>;
    UnbindTest ut;
    assert(ub(ut, 3) == 10);
    assert(ub(&ut, 3) == 10);

    constexpr auto ubc = unbind<&UnbindTest::addConst>;
    const UnbindTest cut;
    assert(ubc(cut, 1) == 8);
}

// ============================================================
// Section 11: FunctionRef
// ============================================================

struct FrTest
{
    constexpr int scale(int x) const noexcept { return x * factor; }
    int factor{4};
};

void test_function_ref()
{
    // from a stateless lambda
    static constexpr auto inc = [](int x) { return x + 1; };
    constexpr FunctionRef<int(int)> f1(inc);
    assert(f1(9) == 10);

    // from a compile-time fn<> value (no object)
    constexpr int freeAdd(int x) noexcept;
    // from a bind-style (obj + fn<>)
    static constexpr FrTest fr;
    constexpr FunctionRef<int(int)> f2(&fr, fn<&FrTest::scale>);
    assert(f2(3) == 12);

    // operator bool
    FunctionRef<int(int)> empty;
    assert(!empty);
    assert(!!f1);

    // runtime function pointer (explicit)
    int(*rawFn)(int) = [](int x) { return x + 100; };
    FunctionRef<int(int)> f3(rawFn);
    assert(f3(5) == 105);
}

constexpr int freeAdd(int x) noexcept { return x + 1; }

// ============================================================
// Section 12: constructor and creator helpers
// ============================================================

struct CtorTest
{
    int x; double y;
    CtorTest(int x, double y) : x(x), y(y) {}
};

struct InitListTest
{
    int sum{};
    InitListTest(std::initializer_list<int> il) { for (int v : il) sum += v; }
};

void test_constructor_creator()
{
    auto c1 = constructor<CtorTest>;
    auto obj = c1(1, 2.5);
    assert(obj.x == 1);

    auto c2 = constructor<InitListTest, true>;
    auto obj2 = c2(3, 4);
    assert(obj2.sum == 7);

    auto* p = creator<CtorTest>(2, 3.0);
    assert(p->x == 2);
    delete p;
}

// ============================================================
// Section 13: unary (tuple adapter)
// ============================================================

void test_unary()
{
    auto add = [](int a, int b) { return a + b; };
    auto tupleAdd = unary(add);
    assert(tupleAdd(std::make_tuple(3, 4)) == 7);

    constexpr auto tupleAdd2 = unary(fn<freeCmp1>);
    assert(tupleAdd2(std::make_tuple(5)) == 1);
}

// ============================================================
// Section 14: IsMemberOf / MemberOf concept
// ============================================================

struct MemOfA { void foo(int) {} };
struct MemOfB { void bar(int) {} };

static_assert(IsMemberOf<MemOfA>::value<decltype(&MemOfA::foo)>);
static_assert(!IsMemberOf<MemOfA>::value<decltype(&MemOfB::bar)>);

template<MemberOf<MemOfA> F>
struct OnlyA {};

// OnlyA<decltype(&MemOfA::foo)> a; // OK (would compile)

// ============================================================
// Section 15: Lazy / lazy
// ============================================================

void test_lazy()
{
    int calls = 0;
    auto gen = Lazy([&] { ++calls; return 42; });
    assert(calls == 0);
    int v = gen;
    assert(v == 42 && calls == 1);

    // lazy<T>(args...)
    auto lazyStr = lazy<std::string>(3u, 'x');
    std::string s = lazyStr;
    assert(s == "xxx");

    // lazy<T>(useInit, args...)  — initializer-list constructor
    auto lazyInitList = lazy<InitListTest>(useInit, 1, 2, 3);
    InitListTest il = lazyInitList;
    assert(il.sum == 6);
}

// ============================================================
// Section 16: Cleanup / resetCleans
// ============================================================

void test_cleanup()
{
    int cleaned = 0;
    {
        Cleanup c([&] { ++cleaned; });
        assert(cleaned == 0);
    }
    assert(cleaned == 1);

    // resetCleans prevents the cleanup from running
    int cleaned2 = 0;
    {
        Cleanup c2([&] { ++cleaned2; });
        resetCleans(c2);
    }
    assert(cleaned2 == 0);
}

// ============================================================
// Section 17: ForwardLike
// ============================================================

static_assert(std::is_same_v<ForwardLike<int&,       double>, double&>);
static_assert(std::is_same_v<ForwardLike<int&&,      double>, double&&>);
static_assert(std::is_same_v<ForwardLike<const int&, double>, const double&>);
static_assert(std::is_same_v<ForwardLike<const int&&,double>, const double&&>);

// ============================================================
// Main
// ============================================================

int main()
{
    test_invokeAs();
    test_visitor();
    test_curry();
    test_bind();
    test_unbind();
    test_function_ref();
    test_constructor_creator();
    test_unary();
    test_lazy();
    test_cleanup();
    return 0;
}

