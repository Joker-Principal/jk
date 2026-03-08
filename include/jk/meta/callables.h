#pragma once
#include <functional>
#include <jk/meta/compiler.h>
#include <jk/meta/type-list.h>

namespace JK::Meta::Details::Callables
{
template<typename T>
using FunctorType = decltype(&std::decay_t<T>::operator());

template<typename T>
concept isFunctor = requires { typename FunctorType<T>; };

template<typename F>
using FunctionType = std::remove_pointer_t<std::decay_t<F>>;

template<typename F>
concept isFunction = std::is_function_v<FunctionType<F>>;

/// workaround invoke_r before C++23
#if __cpp_lib_invoke_r >= 202106L
using std::invoke_r;
#else
template<typename R, typename... Args>
constexpr R invoke_r(Args&&... args) noexcept(std::is_nothrow_invocable_r_v<R, Args...>)
{
	return static_cast<R>(std::invoke(std::forward<Args>(args)...));
}
#endif

#define JK_GET_REF(i) JK_SELECT(i, , &, &&)
#define JK_REF(func, cv) func(cv, 0) func(cv, 1) func(cv, 2)

#define JK_GET_CV(i) JK_SELECT(i, , const, volatile, const volatile)
#define JK_CV_REF(func) JK_REF(func, 0) JK_REF(func, 1) JK_REF(func, 2) JK_REF(func, 3)

#define JK_V_LIKE(qualifier) <T qualifier, U>

#define JK_S_LIKE(...) \
	JK_OVERLOAD(__VA_OPT__(,) __VA_ARGS__, JK_V_LIKE(__VA_ARGS__), JK_EMPTY(__VA_ARGS__))

#define JK_X_LIKE(cv, ref)                                             \
	template<typename T, typename U>                                   \
	struct ForwardLikeTrait JK_S_LIKE(JK_GET_CV(cv) JK_GET_REF(ref)) { \
		using type = U JK_GET_CV(cv) JK_GET_REF(ref);                  \
	};                                                                 \

JK_CV_REF(JK_X_LIKE)

#undef JK_V_LIKE
#undef JK_S_LIKE
#undef JK_X_LIKE

template<typename T, typename U>
requires std::is_same_v<std::remove_cvref_t<U>, U>
using ForwardLike = typename ForwardLikeTrait<T, U>::type;

/// CallableTrait (not support for calling conversion)
template<bool variadic, bool nothrow, typename T, typename R, typename... Args>
struct CallableImpl
{
	static constexpr bool isVariadic = variadic;
	static constexpr bool isNothrow = nothrow;

	using Class = T;
	using Return = R;
	using Arguments = TList<Args...>;
	using Signature = std::conditional_t<
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

	template<typename Rx>
	using ReplaceReturn = CallableImpl<variadic, nothrow, T, Rx, Args...>;

	template<typename... NewArgs>
	using ReplaceArgs = CallableImpl<variadic, nothrow, T, R, NewArgs...>;

	template<typename Tx>
	using ReplaceClass = CallableImpl<variadic, nothrow, Tx, R, Args...>;

	template<bool vx>
	using ReplaceVariadic = CallableImpl<vx, nothrow, T, R, Args...>;

	template<bool nx>
	using ReplaceNothrow = CallableImpl<variadic, nx, T, R, Args...>;

	template<bool vx, bool nx, typename Tx, typename Rx, typename... Ax>
	friend constexpr bool operator==(const CallableImpl&, const CallableImpl<vx, nx, Tx, Rx, Ax...>&) noexcept
	{
		return std::is_same_v<CallableImpl, CallableImpl<vx, nx, Tx, Rx, Ax...>>;
	}
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
#define JK_CALLABLE_MEM_X_CV_REF_NOTHROW(cv, ref, nothrow, noexcept)     \
	template<typename R, typename T, typename... Args>                   \
	struct CallableTrait<R(T::*)(Args...) cv ref noexcept>               \
	{                                                                    \
		using type = CallableImpl<false, nothrow, T cv ref, R, Args...>; \
	};                                                                   \
	template<typename R, typename T, typename... Args>                   \
	struct CallableTrait<R(T::*)(Args..., ...) cv ref noexcept>          \
	{                                                                    \
		using type = CallableImpl<true, nothrow, T cv ref, R, Args...> ; \
	};                                                                   \

#define JK_CALLABLE_MEM_CV_REF_NOTHROW(cv, ref, nothrow) \
	JK_CALLABLE_MEM_X_CV_REF_NOTHROW(JK_CALLABLE_GET_CV(cv), JK_CALLABLE_GET_REF(ref), bool(nothrow), JK_CALLABLE_GET_NOEXCEPT(nothrow))

JK_CALLABLE_MEM(JK_CALLABLE_MEM_CV_REF_NOTHROW)
#undef JK_CALLABLE_MEM_CV_REF_NOTHROW

// member data pointer
template<typename R, typename T>
struct CallableTrait<R T::*>
{
	using type = CallableImpl<false, true, T*, R, TList<>>;
};

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

JK_CALLABLE_NOTHROW(JK_CALLABLE_FUNC_NOTHROW)
#undef JK_CALLABLE_FUNC_NOTHROW

// functor (including lambda)
template <isFunctor F>
struct CallableTrait<F> : CallableTrait<FunctorType<F>>{};

// std::function support
template<typename F>
struct CallableTrait<std::function<F>> : CallableTrait<std::decay_t<F>> {};

/// Callable
template<typename F>
concept isCallable = requires { typename CallableTrait<std::decay_t<F>>::type; };

template<isCallable F>
struct Callable : CallableTrait<std::decay_t<F>>::type 
{
	template<typename... Ax>
	static constexpr bool isInvocable = std::is_invocable_v<F, Ax...>;

	template<typename R, typename... Args>
	struct IsCompatibleWith
	{
		static constexpr bool value = std::is_invocable_r_v<R, F, Args...>;
	};

	template<isFunction Signature>
	static constexpr bool isCompatibleWith = Callable<Signature>::template Expand<IsCompatibleWith>::value;
};

template<typename F>
using Return = typename Callable<F>::Return;

template<typename F>
using Class = typename Callable<F>::Class;

template<typename F>
using Arguments = typename Callable<F>::Arguments;

template<typename F>
using Signature = typename Callable<F>::Signature;

template<typename F>
inline constexpr bool isVariadic = Callable<F>::isVariadic;

template<typename F>
inline constexpr bool isNothrow = Callable<F>::isNothrow;

/// CallableValue
template<typename F>
struct CallableValueImpl
{
	template<bool variadic, bool nothrow, typename T, typename R, typename... Args>
	struct Impl
	{
		using RawFunction = F;
		using Function = std::decay_t<F>;
		using Callable = Callable<F>;

		template<typename... Ax>
		static constexpr bool isInvocable = Callable::template isInvocable<Ax...>;

		template<isFunction Signature>
		static constexpr bool isCompatibleWith = Callable::template isCompatibleWith<Signature>;

		constexpr std::strong_ordering operator<=>(const Impl&) const noexcept = default;

		JK_NO_UNIQUE_ADDRESS Function function;
	};
};

template<isCallable F>
struct CallableValue : Callable<F>::template ExpandAll<CallableValueImpl<F>::template Impl>
{
	using Base = Callable<F>::template ExpandAll<CallableValueImpl<F>::template Impl>;

	constexpr CallableValue(F f) noexcept(noexcept(Base(std::forward<F>(f)))) : // CTAD
		Base(std::forward<F>(f))
	{}
};

template<CallableValue v>
struct CallableOf : decltype(v)::Callable
{
	static constexpr auto value = v;
};

template<CallableValue v>
inline constexpr auto fn = CallableOf<v>{};

template<CallableValue v>
using ReturnOf = typename CallableOf<v>::Return;

template<CallableValue v>
using ClassOf = typename CallableOf<v>::Class;

template<CallableValue v>
using ArgumentsOf = typename CallableOf<v>::Arguments;

template<CallableValue v>
using SignatureOf = typename CallableOf<v>::Signature;

/// invokeAs, returnAs, argAs
template<typename R, typename... Args>
struct InvokeTypedFunctor
{
	template<typename F>
	friend constexpr auto operator|(F&& f, InvokeTypedFunctor)
	{
		return [fx = std::forward<F>(f)](Args... args) constexpr mutable -> R {
			return invoke_r<R>(std::forward<F>(fx), std::forward<Args>(args)...);
		};
	}
};

template<isFunction Signature>
inline constexpr auto invokeAs = typename Callable<Signature>::template Expand<InvokeTypedFunctor>{};

template<typename R>
struct ReturnTypedFunctor
{
	template<isCallable F>
	friend constexpr auto operator|(F&& f, ReturnTypedFunctor)
	{
		// keep the arguments of f, only change the return type to R
		using Signature = typename Callable<F>::template ReplaceReturn<R>::Signature;
		return std::forward<F>(f) | invokeAs<Signature>;
	}

	template<typename F>
	friend constexpr auto operator|(F&& f, ReturnTypedFunctor)
	{
		// f may be a functor with overloaded calling operators
		return [fx = std::forward<F>(f)]<typename... Args>(Args&&... args) constexpr mutable -> R {
			return invoke_r<R>(std::forward<F>(fx), std::forward<Args>(args)...);
		};
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
		return [fx = std::forward<F>(f)](Args... args) constexpr mutable {
			return std::invoke(std::forward<F>(fx), std::forward<Args>(args)...);
		};
	}
};

template<typename... Args>
inline constexpr auto argAs = ArgTypedFunctor<Args...>{};

/// GenericFunctor: wrap a final overload functor or a non-class type (e.g. function pointer)

template<typename F>
struct GenericFunctor
{
	using XF = std::decay_t<F>;
	JK_NO_UNIQUE_ADDRESS XF f;

	constexpr GenericFunctor(F&& f) noexcept(std::is_nothrow_constructible_v<XF, F&&>) :
		f(std::forward<F>(f))
	{}

#if __cpp_explicit_this_parameter >= 202110L
	template<typename Self, typename... Args>
	constexpr auto operator()(this Self&& self, Args&&... args)
		noexcept(std::is_nothrow_invocable_v<ForwardLike<Self, XF>, Args...>)
		requires(std::is_invocable_v<ForwardLike<Self, XF>, Args...>)
	{
		return std::invoke(std::forward_like<Self>(self.f), std::forward<Args>(args)...);
	}
#else
	template<typename... Args>
	constexpr decltype(auto) operator()(Args&&... args) &
		noexcept(std::is_nothrow_invocable_v<XF &, Args...>)
		requires(std::is_invocable_v<XF &, Args...>)
	{
		return std::invoke(f, std::forward<Args>(args)...);
	}

	template<typename... Args>
	constexpr decltype(auto) operator()(Args&&... args) const&
		noexcept(std::is_nothrow_invocable_v<XF const&, Args...>)
		requires(std::is_invocable_v<XF const&, Args...>)
	{
		return std::invoke(f, std::forward<Args>(args)...);
	}

	template<typename... Args>
	constexpr decltype(auto) operator()(Args&&... args) &&
		noexcept(std::is_nothrow_invocable_v<XF &&, Args...>)
		requires(std::is_invocable_v<XF &&, Args...>)
	{
		return std::invoke(std::move(f), std::forward<Args>(args)...);
	}

	template<typename... Args>
	constexpr decltype(auto) operator()(Args&&... args) const&&
		noexcept(std::is_nothrow_invocable_v<XF const&&, Args...>)
		requires(std::is_invocable_v<XF const&&, Args...>)
	{
		return std::invoke(std::move(f), std::forward<Args>(args)...);
	}
#endif
};

/// Visitor
template<typename... Vs>
struct Visitor : GenericFunctor<Vs>...
{
	constexpr Visitor(Vs&&... vs) : GenericFunctor<Vs>(std::forward<Vs>(vs))... {}
	using GenericFunctor<Vs>::operator()...;
};

template<typename... Vs>
Visitor(Vs&&...) -> Visitor<Vs...>;

/// curry
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
		return std::invoke(std::forward<F>(fx), std::forward<Args>(args1x)..., std::forward<decltype(args2)>(args2)...);
	};
#endif
}

/// bind
template<bool variadic, bool nothrow, typename T, typename R, typename... As>
struct Bind
{
	template<auto f>
	static constexpr auto zeroCost(std::remove_reference_t<T>* const obj)
	{
		if constexpr (!variadic)
			return [obj](As... args) noexcept(nothrow) -> R {
				return std::invoke(f, obj, std::forward<As>(args)...);
			};
		else
			return [obj]<typename... VArgs>(As... args, VArgs&&... vargs) noexcept(nothrow) -> R {
				return std::invoke(f, obj, std::forward<As>(args)..., std::forward<VArgs>(vargs)...);
			};
	}

	template<typename F>
	static constexpr auto twoPointer(std::remove_reference_t<T>* const obj, F f)
	{
		if constexpr (!variadic)
			return [obj, f](As... args) noexcept(nothrow) -> R {
				return std::invoke(f, obj, std::forward<As>(args)...);
			};
		else
			return [obj, f]<typename... VArgs>(As... args, VArgs&&... vargs) noexcept(nothrow) -> R {
				return std::invoke(f, obj, std::forward<As>(args)..., std::forward<VArgs>(vargs)...);
			};
	}
};

template<CallableValue v, typename T>
[[nodiscard]] constexpr auto bind(T* obj) noexcept
{
	static_assert(std::is_member_pointer_v<decltype(v.function)>, "bind is used for member pointer only");
	return CallableOf<v>::template ExpandAll<Bind>::template zeroCost<v.function>(obj);
}

template<CallableValue v, typename T>
constexpr auto bind(T& obj) noexcept { return bind<v>(std::addressof(obj)); }

template<CallableValue v, typename T>
constexpr auto bind(T* obj, CallableOf<v>) noexcept { return bind<v>(obj); }

template<CallableValue v, typename T>
constexpr auto bind(T& obj, CallableOf<v>) noexcept { return bind<v>(std::addressof(obj)); }

template<typename T, typename F>
requires std::is_member_pointer_v<std::decay_t<F>>
[[nodiscard]] constexpr auto bind(T* obj, F f) noexcept
{
	using M = std::decay_t<F>;
	return Callable<M>::template ExpandAll<Bind>::template twoPointer<M>(obj, f);
}

template<typename T, typename F>
constexpr auto bind(T& obj, F f) noexcept { return bind(std::addressof(obj), f); }

/// unbind
template<bool variadic, bool nothrow, typename T, typename R, typename... As>
struct Unbind
{
	template<auto f>
	static constexpr auto impl()
	{
		if constexpr (!variadic)
			return []<typename C>(C&& obj, As... as) noexcept(nothrow) -> R {
				return std::invoke(f, std::forward<C>(obj), std::forward<As>(as)...);
			};
		else
			return []<typename C, typename... VArgs>(C&& obj, As... as, VArgs&&... vargs) noexcept(nothrow)  -> R {
				return std::invoke(f, std::forward<C>(obj), std::forward<As>(as)..., std::forward<VArgs>(vargs)...);
			};
	}
};

template<std::is_member_pointer memFn>
inline constexpr auto unbind = CallableOf<memFn>::template ExpandClass<Unbind>::template impl<memFn>();

/// FunctionRef
template<bool variadic, bool nothrow, typename, typename R, typename... Args>
class FunctionRefImpl
{
	static_assert(!variadic, "no way to forward arguments to a variadic function pointer");

	union Target
	{
		const void* obj;
		void(*cfunc)();
	};
public:
	constexpr FunctionRefImpl() noexcept : target(), func(nullptr) {}
	constexpr FunctionRefImpl(std::nullptr_t) noexcept : FunctionRefImpl() {}
	constexpr FunctionRefImpl(const FunctionRefImpl&) noexcept = default;
	constexpr FunctionRefImpl& operator=(const FunctionRefImpl&) & noexcept = default;
	constexpr FunctionRefImpl(FunctionRefImpl&&) noexcept = default;
	constexpr FunctionRefImpl& operator=(FunctionRefImpl&&) & noexcept = default;
	JK_CONSTEXPR_DESTRUCTOR ~FunctionRefImpl() noexcept = default;

	// functor
	template<typename F>
	requires Callable<F>::template isCompatibleWith<R(Args...)>
	constexpr FunctionRefImpl(F&& f) noexcept :
		target(static_cast<const void*>(std::addressof(f))),
		func([](Target xtarget, Args&&... args) -> R {
			auto* of = static_cast<std::add_pointer_t<F>>(const_cast<void*>(xtarget.obj));
			return invoke_r<R>(*of, std::forward<Args>(args)...);
		})
	{}

	// compile-time callables
	template<CallableValue v>
	requires (v.template isCompatibleWith<R(Args...)>)
	constexpr FunctionRefImpl(CallableOf<v>) noexcept :
		target(),
		func([](Target xtarget, Args&&... args) -> R {
			return invoke_r<R>(v.function, std::forward<Args>(args)...);
		})
	{}

	// bind-style
	template<typename T, CallableValue v>
	requires (v.template isCompatibleWith<R(T*, Args...)>)
	constexpr FunctionRefImpl(T* obj, CallableOf<v>) noexcept :
		target(static_cast<const void*>(obj)),
		func([](Target xtarget, Args&&... args) -> R {
			auto of = static_cast<T*>(const_cast<void*>(xtarget.obj));
			return invoke_r<R>(v.function, of, std::forward<Args>(args)...);
		})
	{}

	template<typename T, CallableValue v>
	requires (v.template isCompatibleWith<R(T&, Args...)>)
	constexpr FunctionRefImpl(T& obj, CallableOf<v> call) noexcept :
		FunctionRefImpl(std::addressof(obj), call)
	{}

	// runtime-assigned function pointer
	template<typename F>
	requires (std::is_function_v<F> && std::is_invocable_r_v<R, F, Args...>)
	explicit FunctionRefImpl(F* f) noexcept :
		target(reinterpret_cast<void(*)()>(f)),
		func([](Target xtarget, Args&&... args) -> R {
			auto fp = reinterpret_cast<F*>(xtarget.cfunc);
			return invoke_r<R>(*fp, std::forward<Args>(args)...);
		})
	{}

	constexpr operator bool() const noexcept { return !!func; }

	constexpr R operator()(Args... args) const noexcept(nothrow)
	{
		return func(target, std::forward<Args&&>(args)...);
	}

private:
	Target target;
	R(*func)(Target, Args&&...) noexcept(nothrow);
};

template<isFunction Signature>
using FunctionRef = typename Callable<Signature>::template ExpandAll<FunctionRefImpl>;

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

struct UseInitializerList
{
	constexpr explicit UseInitializerList() noexcept = default;
} inline constexpr useInit;

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
	JK_NO_UNIQUE_ADDRESS F generator;
};

template<typename F> Lazy(F&&) -> Lazy<std::decay_t<F>>;

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
/// detect whether a class type has a non-overloaded operator() defined, including lambda
using Details::Callables::isFunctor;

/// detect whether a type is a function pointer or reference, but not functor
using Details::Callables::isFunction;

using Details::Callables::invoke_r;

// add cv-qualifier and reference qualifier to U like T
using Details::Callables::ForwardLike;

/// callable traits and utilities:
/// 1. specialization to trait callable infomation out:
/// 	Callable<R(Args...)> = {        |  Callable<R(Args..., ...) noexcept> = {
/// 	    Class = void;               |      Class = void;
/// 	    Return = R;                 |      Return = R;
/// 	    Arguments = TList<Args...>; |      Arguments = TList<Args...>;
/// 	    Signature = R(Args...);     |      Signature = R(Args..., ...) noexcept;
/// 	    isVariadic = false;         |      isVariadic = true;
/// 	    isNothrow = false;          |      isNothrow = true;
/// 	}                               |  }
///     --------------------------------------------------------------------------------------------------
/// 	Callable<R(T::*)(Args...)> = {  |  Callable<R(T::*)(Args..., ...) const volatile && noexcept> = {
/// 	    Class = T;                  |      Class = T const volatile &&;
/// 	    Return = R;                 |      Return = R;
/// 	    Arguments = TList<Args...>; |      Arguments = TList<Args...>;
/// 	    Signature = R(Args...);     |      Signature = R(Args..., ...) noexcept;
/// 	    isVariadic = false;         |      isVariadic = true;
/// 	    isNothrow = false;          |      isNothrow = true;
/// 	}                               |  }
/// 	--------------------------------------------------------------------------------------------------
/// 	                                |  auto f = [](){};
///     Callable<R T::*> = {            |  Callable<decltype(f)> = {
/// 	    Class = T*;                 |      Class = decltype(f);
/// 	    Return = R;                 |      Return = void;
/// 	    Arguments = TList<>;        |      Arguments = TList<>;
/// 	    Signature = R() noexcept;   |      Signature = void();
/// 	    isVariadic = false;         |      isVariadic = false;
/// 	    isNothrow = true;           |      isNothrow = false;
/// 	}                               |  }
///     --------------------------------------------------------------------------------------------------
/// 	Callable<std::function<R(Args...)>> = Callable<R(Args...)>
///
/// 2. expand callable infomation to template E:
///     Callable::Expand<E> = E<R, Args...>;
///     Callable::ExpandClass<E> = E<T, R, Args...>;
///     Callable::ExpandAll<E> = E<isVariadic, isNothrow, T, R, Args...>
/// 
/// 3. replace callable infomation with new one:
///     Callable::ReplaceReturn<Rx>       = CallableImpl<variadic, nothrow, T, Rx, Args...>;
///     Callable::ReplaceArgs<NewArgs...> = CallableImpl<variadic, nothrow, T, R, NewArgs...>;
///     Callable::ReplaceClass<Tx>        = CallableImpl<variadic, nothrow, Tx, R, Args...>;
///     Callable::ReplaceVariadic<vx>     = CallableImpl<vx, nothrow, T, R, Args...>;
///     Callable::ReplaceNothrow<nx>      = CallableImpl<variadic, nx, T, R, Args...>;
using Details::Callables::Callable;
using Details::Callables::isCallable;

using Details::Callables::Return;
using Details::Callables::Class;
using Details::Callables::Arguments;
using Details::Callables::Signature;
using Details::Callables::isVariadic;
using Details::Callables::isNothrow;

/// compile-time callable value wrapper, can be used as non-type template parameter
/// 	template<CallableValue v>
/// 	void foo(CallableOf<v>) { ... }
/// 	foo(fn<&A::foo>);
using Details::Callables::CallableValue;
using Details::Callables::CallableOf;
using Details::Callables::fn;

using Details::Callables::ReturnOf;
using Details::Callables::ClassOf;
using Details::Callables::ArgumentsOf;
using Details::Callables::SignatureOf;

/// specify the signature to invoke a callable, useful for generic lambda with auto parameters
/// invokeAs:
/// 	auto generic = [...](auto...) -> auto {...};
/// 	auto typed = generic | invokeAs<int(int)>;
/// 	int r = typed(10); // int(generic(10));
/// returnAs:
///     [...](auto...) -> auto {...} | returnAs<int>
///        => [...](auto...) -> int {...}
/// argAs:
///     [...](auto...) -> auto {...} | argAs<int, int>
///        => [...](int, int) -> auto {...}
using Details::Callables::invokeAs;
using Details::Callables::returnAs;
using Details::Callables::argAs;

/// https://www.modernescpp.com/index.php/visiting-a-std-variant-with-the-overload-pattern/
using Details::Callables::GenericFunctor;
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
/// template<MemberOf<A> F> struct X {};
/// X<decltype(&A::foo)> x; // OK
/// X<decltype(&B::bar)> x; // error: no matching template
using Details::Callables::IsMemberOf;
using Details::Callables::MemberOf;

/// tag type to specify using initializer list constructor
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
