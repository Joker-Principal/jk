#pragma once
#include <functional>
#include <jk/meta/compiler.h>
#include <jk/meta/type-list.h>

namespace JK::Meta::Details::Callables
{
template<typename T, typename = void>
struct IsFunctor : std::false_type { using type = T; };

template<typename T>
struct IsFunctor<T, std::void_t<decltype(&T::operator())>> : std::true_type
{
	using type = decltype(&T::operator());
};

template<typename F>
using FunctorType = typename IsFunctor<std::decay_t<F>>::type;

template<bool variadic, bool nothrow, typename T, typename R, typename... Args>
struct CallableImpl
{
	static constexpr bool isVariadic = variadic;
	static constexpr bool isNothrow = nothrow;

	using Class = T;
	using Return = R;
	using Arguments = TList<Args...>;
	using Invoker = std::conditional_t<
		!variadic,
		R(Args...) noexcept(nothrow),
		R(Args..., ...) noexcept(nothrow)
	>;

	template<template<typename Rx, typename... Ax> typename E>
	using Expand = E<R, Args...>;

	template<template<typename Tx, typename Rx, typename... Ax> typename E>
	using ExpandClass = E<T, R, Args...>;

	template<template<bool vx, bool nt, typename Tx, typename Rx, typename... Ax> typename E>
	using ExpandAll = E<variadic, nothrow, T, R, Args...>;
};

template<typename F> struct CallableTrait;

#define JK_CALLABLE_GET_NOEXCEPT(i) JK_SELECT(i, , noexcept)
#define JK_CALLABLE_NOTHROW(func, ...)                          \
	JK_EXEC(func, (__VA_ARGS__ JK_VA_OPT_COMMA(__VA_ARGS__) 0)) \
	JK_EXEC(func, (__VA_ARGS__ JK_VA_OPT_COMMA(__VA_ARGS__) 1)) \

#define JK_CALLABLE_GET_REF(i) JK_SELECT(i, , &, &&)
#define JK_CALLABLE_MEM_REF(func, ...)               \
	func(__VA_ARGS__ JK_VA_OPT_COMMA(__VA_ARGS__) 0) \
	func(__VA_ARGS__ JK_VA_OPT_COMMA(__VA_ARGS__) 1) \
	func(__VA_ARGS__ JK_VA_OPT_COMMA(__VA_ARGS__) 2) \

#define JK_CALLABLE_GET_CV(i) JK_SELECT(i, , const, volatile, const volatile)
#define JK_CALLABLE_MEM_CV(func, ...)                \
	func(__VA_ARGS__ JK_VA_OPT_COMMA(__VA_ARGS__) 0) \
	func(__VA_ARGS__ JK_VA_OPT_COMMA(__VA_ARGS__) 1) \
	func(__VA_ARGS__ JK_VA_OPT_COMMA(__VA_ARGS__) 2) \
	func(__VA_ARGS__ JK_VA_OPT_COMMA(__VA_ARGS__) 3) \

#define JK_CALLABLE_MEM(func) JK_CALLABLE_MEM_CV(JK_CALLABLE_MEM_REF, JK_CALLABLE_NOTHROW, func)

// member function pointer
#define JK_CALLABLE_MEM_CV_REF_NOTHROW(cv, ref, nothrow)                                                                          \
	template<typename R, typename T, typename... Args>                                                                            \
	struct CallableTrait<R(T::*)(Args...) JK_CALLABLE_GET_CV(cv) JK_CALLABLE_GET_REF(ref) JK_CALLABLE_GET_NOEXCEPT(nothrow)>      \
	{                                                                                                                             \
		using type = CallableImpl<false, bool(nothrow), T JK_CALLABLE_GET_CV(cv) JK_CALLABLE_GET_REF(ref) , R, Args...>;          \
	};                                                                                                                            \
	template<typename R, typename T, typename... Args>                                                                            \
	struct CallableTrait<R(T::*)(Args..., ...) JK_CALLABLE_GET_CV(cv) JK_CALLABLE_GET_REF(ref) JK_CALLABLE_GET_NOEXCEPT(nothrow)> \
	{                                                                                                                             \
		using type = CallableImpl<true, bool(nothrow), T JK_CALLABLE_GET_CV(cv) JK_CALLABLE_GET_REF(ref) , R, Args...>;           \
	};                                                                                                                            \

// c function pointer
#define JK_CALLABLE_FUNC_NOTHROW(nothrow)                                      \
	template<typename R, typename... Args>                                     \
	struct CallableTrait<R(*)(Args...) JK_CALLABLE_GET_NOEXCEPT(nothrow)>      \
	{                                                                          \
		using type = CallableImpl<false, bool(nothrow), void, R, Args...>;     \
	};                                                                         \
	template<typename R, typename... Args>                                     \
	struct CallableTrait<R(*)(Args..., ...) JK_CALLABLE_GET_NOEXCEPT(nothrow)> \
	{                                                                          \
		using type = CallableImpl<true, bool(nothrow), void, R, Args...>;      \
	};                                                                         \

// total 42 template specializations, without calling conversion supporting
JK_CALLABLE_MEM(JK_CALLABLE_MEM_CV_REF_NOTHROW)
JK_CALLABLE_NOTHROW(JK_CALLABLE_FUNC_NOTHROW)

// member data pointer
template<typename R, typename T>
struct CallableTrait<R T::*> : CallableTrait<R(T::*)()> {};

// functor(lambda) fallback
template <typename F>
struct CallableTrait : CallableTrait<FunctorType<F>>{};

// std::function support
template<typename F>
struct CallableTrait<std::function<F>> : CallableTrait<F> {};

template<typename F>
using Callable = typename CallableTrait<F>::type;

template<typename F, template<typename... Ts> typename E>
using Expand = typename Callable<F>::template Expand<E>;

template<typename F, template<typename... Ts> typename E>
using ExpandClass = typename Callable<F>::template ExpandClass<E>;

template<auto f>
struct CallableOf : Callable<decltype(f)>
{
	static constexpr auto function = f;
};

template<auto f>
inline constexpr auto callableOf = CallableOf<f>{};

template<auto f> using ReturnOf    = typename CallableOf<f>::Return;
template<auto f> using ClassOf     = typename CallableOf<f>::Class;
template<auto f> using ArgumentsOf = typename CallableOf<f>::Arguments;
template<auto f> using InvokerOf   = typename CallableOf<f>::Invoker;

template<auto f, template<typename... Ts> typename E>
using ExpandOf = typename CallableOf<f>::template Expand<E>;

template<auto f, template<typename... Ts> typename E>
using ExpandClassOf = typename CallableOf<f>::template ExpandClass<E>;

template<typename R, typename... Args>
struct InvokeTypedFunctor
{
	template<typename F>
	friend constexpr auto operator|(F&& f, InvokeTypedFunctor)
	{
		return [fx = std::forward<F>(f)](Args... ts) constexpr -> R {
			return static_cast<R>(fx(std::forward<Args>(ts)...));
		};
	}
};

template<typename Fr>
inline constexpr auto invokeAs = typename Callable<Fr>::template Expand<InvokeTypedFunctor>{};

template<typename R>
struct ReturnTypedFunctor
{
	template<typename... Args, typename F>
	static constexpr auto wrap(F&& f, TList<Args...>)
	{
		return [fx = std::forward<F>(f)](Args... ts) constexpr -> R {
			return static_cast<R>(fx(std::forward<Args>(ts)...));
		};
	}

	template<typename F>
	friend constexpr auto operator|(F&& f, ReturnTypedFunctor)
	{
		if constexpr (IsFunctor<std::decay_t<F>>::value)
		{
			using Arguments = typename Callable<F>::Arguments;
			return wrap(std::forward<F>(f), Arguments{});
		}
		else
		{
			// f may be a functor with overloaded calling operators
			return [fx = std::forward<F>(f)](auto&&... args) constexpr -> R {
				return static_cast<R>(fx(std::forward<decltype(args)>(args)...));
			};
		}
	}
};

template<typename R>
inline constexpr auto returnAs = ReturnTypedFunctor<R>{};

template<typename... Args>
struct ArgTypedFunctor
{
	template<typename F>
	friend constexpr auto operator|(F&& f, ArgTypedFunctor)
	{
		return [fx = std::forward<F>(f)](Args... args) constexpr {
			return fx(std::forward<Args>(args)...);
		};
	}
};

template<typename... Args>
inline constexpr auto argAs = ArgTypedFunctor<Args...>{};

template<typename F> struct FuncPtrWrapper { using type = F; };

template<typename R, typename... Args>
struct FuncPtrWrapper<R(Args...)>
{
	using FuncPtr = R(*)(Args...);
	struct type
	{
		constexpr type(FuncPtr f) noexcept : f(f) {}
		constexpr R operator()(Args... args) const { return f(std::forward<Args>(args)...); }
		FuncPtr f;
	};
};

template<typename... Vs>
struct Visitor : FuncPtrWrapper<Vs>::type...
{
	template<typename V>
	using Base = typename FuncPtrWrapper<V>::type;

	template<typename... Vi>
	constexpr Visitor(Vi&&... fs) : Base<Vs>(std::forward<Vi>(fs))... {}

	using Base<Vs>::operator()...;
};

template<typename... Vi>
Visitor(Vi&&...) -> Visitor<std::remove_pointer_t<std::decay_t<Vi>>...>;

template<typename F, typename... Args>
constexpr auto curry(F&& f, Args&&... args1)
	noexcept(noexcept(std::make_tuple(std::forward<F>(f), std::forward<Args>(args1)...)))
{
#if __cpp_init_captures < 201803L
	// https://stackoverflow.com/a/49902823
	return [fx = std::forward<F>(f), args1x = std::make_tuple(std::forward<Args>(args1)...)](auto&&... args2) mutable {
		return std::apply([&](auto&&... args1y) {
			return std::invoke(std::forward<F>(fx), std::forward<Args>(args1y)..., std::forward<decltype(args2)>(args2)...);
		}, std::move(args1x));
	};
#else
	return [fx = std::forward<F>(f), ...args1x = std::forward<Args>(args1)](auto&&... args2) mutable {
		return std::invoke(fx, std::forward<Args>(args1x)..., std::forward<decltype(args2)>(args2)...);
	};
#endif
}

template<typename T> struct RefTrait { using type = T&; };
template<typename T> struct RefTrait<T&&> { using type = T&&; };
template<typename T> using ForceRef = typename RefTrait<T>::type;

template<typename T, typename R, typename... As>
struct BindImpl
{
	template<auto f, typename Tr>
	struct ZeroCostFunc
	{
		constexpr R operator()(As... args) const
		{
			return std::invoke(f, static_cast<ForceRef<T>>(*obj), std::forward<As>(args)...);
		}
		Tr* const obj;
	};

	template<typename F, typename Tr>
	struct Func
	{
		constexpr R operator()(As... args) const
		{
			return std::invoke(f, static_cast<ForceRef<T>>(*obj), std::forward<As>(args)...);
		}
		Tr* const obj;
		F f;
	};
};

template<auto f>
struct Bind
{
	template<typename T>
	constexpr auto operator()(T* obj) const noexcept
	{
		using Impl = typename CallableOf<f>::template ExpandClass<BindImpl>;
		using Func = typename Impl::template ZeroCostFunc<f, T>;
		return Func{obj};
	}
};

template<>
struct Bind<0>
{
	template<typename T, typename F>
	constexpr auto operator()(T* obj, F f) const noexcept
	{
		using Impl = typename Callable<F>::template ExpandClass<BindImpl>;
		using Func = typename Impl::template Func<F, T>;
		return Func{obj, f};
	}
};

template<auto f = 0>
inline constexpr auto bind = Bind<f>{};

template<typename T, typename R, typename... Args>
struct UnbindImpl
{
	template<auto f>
	struct Impl
	{
		constexpr R operator()(std::remove_reference_t<T>* obj, Args... args) const
		{
			return std::invoke(f, static_cast<ForceRef<T>>(*obj), std::forward<Args>(args)...);
		}

		constexpr R operator()(ForceRef<T> obj, Args... args) const
		{
			return std::invoke(f, static_cast<ForceRef<T>>(obj), std::forward<Args>(args)...);
		}
	};
};

template<auto memberFunc>
constexpr auto unbind =
	typename CallableOf<memberFunc>::template ExpandClass<UnbindImpl>::template Impl<memberFunc>{};

template<typename F>
constexpr bool isFunction = std::is_function_v<std::remove_pointer_t<std::decay_t<F>>>;

template<typename R, typename... Args>
class FunctionRefImpl
{
public:
	FunctionRefImpl() noexcept : target(static_cast<const void*>(nullptr)), func(nullptr) {}

	FunctionRefImpl(std::nullptr_t) noexcept : FunctionRefImpl() {}

	template<typename Rx, typename... Ax>
	FunctionRefImpl(Rx(*f)(Ax...)) noexcept :
		target(reinterpret_cast<void(*)()>(f)),
		func([](Target xtarget, Args&&... args) -> R {
			return reinterpret_cast<Rx(*)(Ax...)>(xtarget.cfunc)(std::forward<Args>(args)...);
		})
	{}

	template<typename F, std::enable_if_t<!isFunction<F>, int> = 0>
	FunctionRefImpl(F&& f) noexcept :
		target(static_cast<const void*>(std::addressof(f))),
		func([](Target xtarget, Args&&... args) -> R {
			auto of = static_cast<std::add_pointer_t<F>>(const_cast<void*>(xtarget.obj));
			return std::invoke(*of, std::forward<Args>(args)...);
		})
	{}

	template<typename C, auto f, std::enable_if_t<f != 0, int> = 0>
	constexpr FunctionRefImpl(C* obj, CallableOf<f>) :
		target(static_cast<const void*>(obj)),
		func([](Target xtarget, Args&&... args) -> R {
			auto of = static_cast<C*>(const_cast<void*>(xtarget.obj));
			return std::invoke(f, *of, std::forward<Args>(args)...);
		})
	{}

	template<typename C, auto f, std::enable_if_t<f != 0, int> = 0>
	constexpr FunctionRefImpl(C& obj, CallableOf<f> callable) :
		FunctionRefImpl(std::addressof(obj), callable)
	{}

	constexpr FunctionRefImpl(const FunctionRefImpl&) noexcept = default;
	constexpr FunctionRefImpl& operator=(const FunctionRefImpl&) & noexcept = default;
	constexpr FunctionRefImpl(FunctionRefImpl&&) noexcept = default;
	constexpr FunctionRefImpl& operator=(FunctionRefImpl&&) & noexcept = default;

	constexpr operator bool() const noexcept { return !!func; }

	constexpr R operator()(Args... args) const
	{
		return std::invoke(func, target, std::forward<Args>(args)...);
	}

private:
	union Target
	{
		constexpr explicit Target(const void* obj) noexcept : obj(obj) {}
		constexpr explicit Target(void(*cfunc)()) noexcept : cfunc(cfunc) {}

		const void* obj;
		void(*cfunc)();
	};
	Target target;
	R(*func)(Target, Args&&...);
};

template<typename F>
using FunctionRef = typename Callable<F>::template Expand<FunctionRefImpl>;

template<typename T, bool useInitialList = false>
inline constexpr auto constructor = [](auto&&... args) -> T {
	if constexpr (!useInitialList)
		return T(std::forward<decltype(args)>(args)...);
	else
		return T{std::forward<decltype(args)>(args)...};
};

template<typename T, bool useInitialList = false>
inline constexpr auto creator = [](auto&&... args) -> T* {
	if constexpr (!useInitialList)
		return new T(std::forward<decltype(args)>(args)...);
	else
		return new T{std::forward<decltype(args)>(args)...};
};

template<typename F>
constexpr auto unary(F&& f)
{
	return [fx = std::forward<F>(f)](auto&& tuple) mutable {
		return std::apply(std::forward<F>(fx), std::forward<decltype(tuple)>(tuple));
	};
}

template<auto f>
constexpr auto unary(CallableOf<f>)
{
	return [](auto&& tuple) {
		return std::apply(f, std::forward<decltype(tuple)>(tuple));
	};
}

template<typename C>
struct IsMemberOf
{
	template<typename Cx, typename R, typename... Args>
	using Value = std::is_same<std::decay_t<Cx>, std::decay_t<C>>;

	template<typename Func>
	static constexpr bool value = Callable<Func>::template ExpandClass<Value>::value;
};

#ifdef __cpp_concepts
template<typename Func, typename C>
concept MemberOf = IsMemberOf<C>::template value<Func>;
#endif

/// A lazy generator that will only call the function when needed
template<typename F, std::enable_if_t<std::is_invocable_v<F>, int> = 0>
class Lazy
{
public:
	template<typename Fi>
	constexpr Lazy(Fi&& func) : generator(std::forward<Fi>(func)) {}

	/// call generator when this object is used in a context that requires a value
    constexpr operator decltype(auto)() noexcept(noexcept(generator())) { return generator(); }
    constexpr operator decltype(auto)() const noexcept(noexcept(generator())) { return generator(); }

	/// explicitly call the generator to get the value
    constexpr decltype(auto) operator()() noexcept(noexcept(generator())) { return generator(); }
    constexpr decltype(auto) operator()() const noexcept(noexcept(generator())) { return generator(); }

private:
	F generator;
};

template<typename F> Lazy(F&&) -> Lazy<std::decay_t<F>>;

struct UseInitializerList
{
	constexpr explicit UseInitializerList() noexcept = default;
} inline constexpr useInit;

template<typename T, typename... Args>
constexpr auto lazy(Args&&... args)
{
	return Lazy([as = std::forward_as_tuple(std::forward<Args>(args)...)]() mutable {
		return std::apply([](auto&&... as) { return T(std::forward<decltype(as)>(as)...); }, std::move(as));
	});
}

template<typename T, typename... Args>
constexpr auto lazy(UseInitializerList, Args&&... args)
{
	return Lazy([as = std::forward_as_tuple(std::forward<Args>(args)...)]() mutable {
		return std::apply([](auto&&... as) { return T{std::forward<decltype(as)>(as)...}; }, std::move(as));
	});
}
} // namespace Callables::Detail

namespace JK::Meta
{
/// using F = double (T::*)(int, char);
/// template<typename R, typename... As> struct MyTemplate;
/// using V = Callable<F>::Expand<MyTemplate>; // V == MyTemplate<double, int, char>
using Details::Callables::Callable;

using Details::Callables::CallableOf;
using Details::Callables::callableOf;

/// using F = double (T::*)(int, char);
/// using V = FunctorReturn<F>; // V == double
template<typename F>
using FunctorReturn = typename Callable<F>::Return;

/// using F = double (T::*)(int, char);
/// using V = FunctorClass<F>; // V == T
template<typename F>
using FunctorClass = typename Callable<F>::Class;

/// using F = double (T::*)(int, char);
/// using V = FunctorArguments<F>; // V == Types::TList<int, char>
template<typename F>
using FunctorArguments = typename Callable<F>::Arguments;

/// using F = double (T::*)(int, char);
/// using V = FunctorInvoker<F>; // V == double(int, char)
template<typename F>
using FunctorInvoker = typename Callable<F>::Invoker;

using Details::Callables::Expand;

/// template<typename R, typename... As> struct MyTemplate {};
/// using F = double (T::*)(int, char) const&;
/// using V = ExpandClass<F, MyTemplate>; // V == MyTemplate<const T&, double, int, char>
using Details::Callables::ExpandClass;

/// struct A { int foo(); };
/// A& foo2();
/// using R = ReturnOf<&A::foo>; // R == int
/// using C2 = ClassOf<foo2>; // C2 == A&
using Details::Callables::ReturnOf;

/// struct A { void foo() const&&; };
/// A& foo2();
/// using C = ClassOf<&A::foo>; // C == const A&&
/// using C2 = ClassOf<foo2>; // C2 == void
using Details::Callables::ClassOf;

/// struct A { void foo(int, char, double); };
/// using As = Arguments<&A::foo>; // As == Types::TList<int, char, double>
using Details::Callables::ArgumentsOf;

/// struct A { double foo(int, char); };
/// using I = InvokerOf<&A::foo>; // I == double(int, char)
using Details::Callables::InvokerOf;

/// template<typename R, typename... As> struct MyTemplate {};
/// struct A { double foo(int, char); };
/// using My = ExpandOf<&A::foo, MyTemplate>; // My == MyTemplate<double, int, char>
using Details::Callables::ExpandOf;

/// template<typename T, typename R, typename... As> struct MyTemplate {};
/// struct A { double foo(int, char) const&&; };
/// using My = ExpandClassOf<&A::foo, MyTemplate>;
/// // My == MyTemplate<const A&&, double, int, char>
using Details::Callables::ExpandClassOf;

/// auto generic = [...](auto...) -> auto {...};
/// auto typed = generic | invokeAs<int(int)>;
///    int r = typed(10); // int(generic(10));
using Details::Callables::invokeAs;

/// [...](auto...) -> auto {...} | returnAs<int>
///        => [...](auto...) -> int {...}
using Details::Callables::returnAs;

/// [...](auto...) -> auto {...} | argAs<int, int>
///        => [...](int, int) -> auto {...}
using Details::Callables::argAs;

/// https://www.modernescpp.com/index.php/visiting-a-std-variant-with-the-overload-pattern/
using Details::Callables::Visitor;

/// https://en.wikipedia.org/wiki/Currying
using Details::Callables::curry;

/// struct A { void foo(int, int); };
/// A a;
///
/// auto foo = bind<&A::foo>(&a);
/// foo(0, 1); // == a.foo(0, 1);
/// static_assert(sizeof(foo) == sizeof(void*));
///
/// auto foo2 = bind(&a, &A::foo);
/// foo2(0, 1); // == a.foo(0, 1);
/// static_assert(sizeof(foo2) == 2 * sizeof(void*));
using Details::Callables::bind;

/// struct A {
///        void fn(int);
///        void constFn(int) const;
/// };
///
///    A a;
///    constexpr auto fn = unbind<&A::fn>;
/// fn(&a, 1); // == a.fn(1);
/// fn(a, 1);  // == a.fn(1);
///
/// const A ca;
/// constexpr auto cfn = unbind<&A::constFn>;
/// cfn(&ca, 1); // == ca.constFn(1);
using Details::Callables::unbind;

/// double foo(FunctionRef<double(char)> f) { return f('0'); }
/// double x = foo([](char c) -> double { return c + 1; }); // x == '0' + 1
using Details::Callables::FunctionRef;

/// struct T1 { T1(int); T1(std::string); };
/// struct T2 { T2(int); T2(std::string); };
/// std::any fromInt(int i, FunctionRef<std::any(int)> f) { return f(i); }
/// std::any fromStr(std::string s, FunctionRef<std::any(std::string)> f) { return f(s); }
/// auto x = fromInt(0, constructor<T1>); // std::any x = T1(0);
/// auto y = fromStr("str", constructor<T2>); // std::any y = T2("str");
using Details::Callables::constructor;
using Details::Callables::creator;

/// unary([](auto&&... args) { ... }) => [](auto&& tuple) { ... }
using Details::Callables::unary;

/// struct A { int foo(int); };
/// struct B { int bar(int); };
/// IsMemberOf<A>::value<&A::foo> => true
/// IsMemberOf<B>::value<&A::foo> => false
using Details::Callables::IsMemberOf;

#ifdef __cpp_concepts
/// template<MemberOf<A> F> struct X {};
/// X<decltype(&A::foo)> x; // OK
/// X<decltype(&B::bar)> x; // error: no matching template
using Details::Callables::MemberOf;
#endif

using Details::Callables::UseInitializerList;
using Details::Callables::useInit;

/// struct Value {
/// 	Value(std::string);
///     Value(std::initializer_list<int>);
/// };
/// std::map<int, Value> m;
/// m.try_emplace(0, "str"); // m[0] == Value{"str"}
/// m.try_emplace(1, Lazy([] { return Value("str2"); })); // m[1] == Value("str2")
/// m.try_emplace(2, lazy<Value>(useInit, 3, 4)); // m[2] == Value{3, 4}
using Details::Callables::Lazy;
using Details::Callables::lazy;
}
