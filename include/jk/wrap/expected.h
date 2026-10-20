#pragma once
#include <system_error>

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
#   include <expected>

namespace JK::Wrap
{
template<typename T, typename E>
using expected = std::expected<T, E>;

using std::unexpected;

template<typename E>
using bad_expected_access = std::bad_expected_access<E>;

template<typename T>
using Result = expected<T, std::error_code>;
} // namespace JK::Wrap

#else
#   include <concepts>
#   include <exception>
#   include <optional>
#   include <type_traits>
#   include <utility>
#   include <variant>

namespace JK::Wrap::Details::Expecteds
{
template<typename E>
class unexpected
{
public:
	explicit unexpected(E err) :
		err(std::move(err))
	{}

	constexpr const E& error() const& noexcept { return err; }
	constexpr E& error() & noexcept { return err; }
	constexpr E&& error() && noexcept { return std::move(err); }
	constexpr const E&& error() const&& noexcept { return std::move(err); }

private:
	E err;
};

template<typename E>
unexpected(E) -> unexpected<E>;

template<typename T>
inline constexpr bool isUnexpected = false;

template<typename E>
inline constexpr bool isUnexpected<unexpected<E>> = true;

template<typename E>
class bad_expected_access : public std::exception
{
public:
	explicit bad_expected_access(E err) :
		err(std::move(err))
	{}

	const char* what() const noexcept override { return "bad access to expected without expected value"; }

	constexpr const E& error() const& noexcept { return err; }
	constexpr E& error() & noexcept { return err; }
	constexpr E&& error() && noexcept { return std::move(err); }
	constexpr const E&& error() const&& noexcept { return std::move(err); }

private:
	E err;
};

template<typename T, typename E>
class expected
{
public:
	using value_type = T;
	using error_type = E;
	using unexpected_type = unexpected<E>;

	expected() requires std::default_initializable<T> :
		data(std::in_place_index<0>)
	{}

	template<typename U = T>
		requires (!std::same_as<std::remove_cvref_t<U>, expected>)
		      && (!isUnexpected<std::remove_cvref_t<U>>)
		      && std::constructible_from<T, U&&>
	explicit(!std::convertible_to<U&&, T>) expected(U&& value) :
		data(std::in_place_index<0>, std::forward<U>(value))
	{}

	template<typename G>
		requires std::constructible_from<E, const G&>
	expected(const unexpected<G>& err) :
		data(std::in_place_index<1>, E(err.error()))
	{}

	template<typename G>
		requires std::constructible_from<E, G&&>
	expected(unexpected<G>&& err) :
		data(std::in_place_index<1>, E(std::move(err).error()))
	{}

	template<typename... Args>
		requires std::constructible_from<T, Args&&...>
	explicit expected(std::in_place_t, Args&&... args) :
		data(std::in_place_index<0>, std::forward<Args>(args)...)
	{}

	constexpr bool has_value() const noexcept { return data.index() == 0; }
	constexpr explicit operator bool() const noexcept { return has_value(); }

	constexpr T& operator*() & noexcept { return std::get<0>(data); }
	constexpr const T& operator*() const& noexcept { return std::get<0>(data); }
	constexpr T&& operator*() && noexcept { return std::get<0>(std::move(data)); }
	constexpr const T&& operator*() const&& noexcept { return std::get<0>(std::move(data)); }

	constexpr T* operator->() noexcept { return &std::get<0>(data); }
	constexpr const T* operator->() const noexcept { return &std::get<0>(data); }

	T& value() & { return has_value() ? std::get<0>(data) : throw bad_expected_access<E>(error()); }
	const T& value() const& { return has_value() ? std::get<0>(data) : throw bad_expected_access<E>(error()); }
	T&& value() && { return has_value() ? std::get<0>(std::move(data)) : throw bad_expected_access<E>(error()); }
	const T&& value() const&& { return has_value() ? std::get<0>(std::move(data)) : throw bad_expected_access<E>(error()); }

	constexpr E& error() & noexcept { return std::get<1>(data); }
	constexpr const E& error() const& noexcept { return std::get<1>(data); }
	constexpr E&& error() && noexcept { return std::get<1>(std::move(data)); }
	constexpr const E&& error() const&& noexcept { return std::get<1>(std::move(data)); }

	template<typename U>
		requires std::copy_constructible<T> && std::convertible_to<U&&, T>
	constexpr T value_or(U&& defaultValue) const&
	{
		return has_value() ? std::get<0>(data) : static_cast<T>(std::forward<U>(defaultValue));
	}

	template<typename U>
		requires std::move_constructible<T> && std::convertible_to<U&&, T>
	constexpr T value_or(U&& defaultValue) &&
	{
		return has_value() ? std::get<0>(std::move(data)) : static_cast<T>(std::forward<U>(defaultValue));
	}

	template<typename G>
		requires std::copy_constructible<E> && std::convertible_to<G&&, E>
	constexpr E error_or(G&& defaultError) const&
	{
		return has_value() ? static_cast<E>(std::forward<G>(defaultError)) : std::get<1>(data);
	}

	template<typename G>
		requires std::move_constructible<E> && std::convertible_to<G&&, E>
	constexpr E error_or(G&& defaultError) &&
	{
		return has_value() ? static_cast<E>(std::forward<G>(defaultError)) : std::get<1>(std::move(data));
	}

public:
	template<std::invocable<T&> F>
	constexpr auto and_then(F&& f) &
	{
		using Ret = std::invoke_result_t<F, T&>;
		if(has_value()) return std::invoke(std::forward<F>(f), std::get<0>(data));
		return Ret(unexpected<typename Ret::error_type>(static_cast<typename Ret::error_type>(std::get<1>(data))));
	}

	template<std::invocable<const T&> F>
	constexpr auto and_then(F&& f) const&
	{
		using Ret = std::invoke_result_t<F, const T&>;
		if(has_value()) return std::invoke(std::forward<F>(f), std::get<0>(data));
		return Ret(unexpected<typename Ret::error_type>(static_cast<typename Ret::error_type>(std::get<1>(data))));
	}

	template<std::invocable<T&&> F>
	constexpr auto and_then(F&& f) &&
	{
		using Ret = std::invoke_result_t<F, T&&>;
		if(has_value()) return std::invoke(std::forward<F>(f), std::get<0>(std::move(data)));
		return Ret(unexpected<typename Ret::error_type>(static_cast<typename Ret::error_type>(std::get<1>(data))));
	}

	template<std::invocable<const T&&> F>
	constexpr auto and_then(F&& f) const&&
	{
		using Ret = std::invoke_result_t<F, const T&&>;
		if(has_value()) return std::invoke(std::forward<F>(f), std::get<0>(std::move(data)));
		return Ret(unexpected<typename Ret::error_type>(static_cast<typename Ret::error_type>(std::get<1>(data))));
	}

	template<std::invocable<E&> F>
	constexpr auto or_else(F&& f) &
	{
		using Ret = std::invoke_result_t<F, E&>;
		if(has_value())
		{
			if constexpr (std::is_void_v<typename Ret::value_type>) return Ret();
			else return Ret(std::in_place, std::get<0>(data));
		}
		return std::invoke(std::forward<F>(f), std::get<1>(data));
	}

	template<std::invocable<const E&> F>
	constexpr auto or_else(F&& f) const&
	{
		using Ret = std::invoke_result_t<F, const E&>;
		if(has_value())
		{
			if constexpr (std::is_void_v<typename Ret::value_type>) return Ret();
			else return Ret(std::in_place, std::get<0>(data));
		}
		return std::invoke(std::forward<F>(f), std::get<1>(data));
	}

	template<std::invocable<E&&> F>
	constexpr auto or_else(F&& f) &&
	{
		using Ret = std::invoke_result_t<F, E&&>;
		if(has_value())
		{
			if constexpr (std::is_void_v<typename Ret::value_type>) return Ret();
			else return Ret(std::in_place, std::get<0>(std::move(data)));
		}
		return std::invoke(std::forward<F>(f), std::get<1>(std::move(data)));
	}

	template<std::invocable<const E&&> F>
	constexpr auto or_else(F&& f) const&&
	{
		using Ret = std::invoke_result_t<F, const E&&>;
		if(has_value())
		{
			if constexpr (std::is_void_v<typename Ret::value_type>) return Ret();
			else return Ret(std::in_place, std::get<0>(std::move(data)));
		}
		return std::invoke(std::forward<F>(f), std::get<1>(std::move(data)));
	}

	template<std::invocable<T&> F>
	constexpr auto transform(F&& f) &
	{
		using U = std::remove_cv_t<std::invoke_result_t<F, T&>>;
		using Ret = expected<U, E>;
		if(has_value())
		{
			if constexpr (std::is_void_v<U>)
			{
				std::invoke(std::forward<F>(f), std::get<0>(data));
				return Ret();
			}
			else return Ret(std::invoke(std::forward<F>(f), std::get<0>(data)));
		}
		return Ret(unexpected<E>(std::get<1>(data)));
	}

	template<std::invocable<const T&> F>
	constexpr auto transform(F&& f) const&
	{
		using U = std::remove_cv_t<std::invoke_result_t<F, const T&>>;
		using Ret = expected<U, E>;
		if(has_value())
		{
			if constexpr (std::is_void_v<U>)
			{
				std::invoke(std::forward<F>(f), std::get<0>(data));
				return Ret();
			}
			else return Ret(std::invoke(std::forward<F>(f), std::get<0>(data)));
		}
		return Ret(unexpected<E>(std::get<1>(data)));
	}

	template<std::invocable<T&&> F>
	constexpr auto transform(F&& f) &&
	{
		using U = std::remove_cv_t<std::invoke_result_t<F, T&&>>;
		using Ret = expected<U, E>;
		if(has_value())
		{
			if constexpr (std::is_void_v<U>)
			{
				std::invoke(std::forward<F>(f), std::get<0>(std::move(data)));
				return Ret();
			}
			else return Ret(std::invoke(std::forward<F>(f), std::get<0>(std::move(data))));
		}
		return Ret(unexpected<E>(std::get<1>(std::move(data))));
	}

	template<std::invocable<const T&&> F>
	constexpr auto transform(F&& f) const&&
	{
		using U = std::remove_cv_t<std::invoke_result_t<F, const T&&>>;
		using Ret = expected<U, E>;
		if(has_value())
		{
			if constexpr (std::is_void_v<U>)
			{
				std::invoke(std::forward<F>(f), std::get<0>(std::move(data)));
				return Ret();
			}
			else return Ret(std::invoke(std::forward<F>(f), std::get<0>(std::move(data))));
		}
		return Ret(unexpected<E>(std::get<1>(std::move(data))));
	}

	template<std::invocable<E&> F>
	constexpr auto transform_error(F&& f) &
	{
		using G = std::remove_cv_t<std::invoke_result_t<F, E&>>;
		using Ret = expected<T, G>;
		if(has_value()) return Ret(std::in_place, std::get<0>(data));
		return Ret(unexpected<G>(std::invoke(std::forward<F>(f), std::get<1>(data))));
	}

	template<std::invocable<const E&> F>
	constexpr auto transform_error(F&& f) const&
	{
		using G = std::remove_cv_t<std::invoke_result_t<F, const E&>>;
		using Ret = expected<T, G>;
		if(has_value()) return Ret(std::in_place, std::get<0>(data));
		return Ret(unexpected<G>(std::invoke(std::forward<F>(f), std::get<1>(data))));
	}

	template<std::invocable<E&&> F>
	constexpr auto transform_error(F&& f) &&
	{
		using G = std::remove_cv_t<std::invoke_result_t<F, E&&>>;
		using Ret = expected<T, G>;
		if(has_value()) return Ret(std::in_place, std::get<0>(std::move(data)));
		return Ret(unexpected<G>(std::invoke(std::forward<F>(f), std::get<1>(std::move(data)))));
	}

	template<std::invocable<const E&&> F>
	constexpr auto transform_error(F&& f) const&&
	{
		using G = std::remove_cv_t<std::invoke_result_t<F, const E&&>>;
		using Ret = expected<T, G>;
		if(has_value()) return Ret(std::in_place, std::get<0>(std::move(data)));
		return Ret(unexpected<G>(std::invoke(std::forward<F>(f), std::get<1>(std::move(data)))));
	}

private:
	std::variant<T, E> data;
};

template<typename E>
class expected<void, E>
{
public:
	using value_type = void;
	using error_type = E;
	using unexpected_type = unexpected<E>;

	expected() noexcept = default;

	template<typename G>
		requires std::constructible_from<E, const G&>
	expected(const unexpected<G>& err) :
		err(E(err.error()))
	{}

	template<typename G>
		requires std::constructible_from<E, G&&>
	expected(unexpected<G>&& err) :
		err(E(std::move(err).error()))
	{}

	constexpr bool has_value() const noexcept { return !err.has_value(); }
	constexpr explicit operator bool() const noexcept { return has_value(); }

	void value() const& { if(err) throw bad_expected_access<E>(*err); }
	void value() && { if(err) throw bad_expected_access<E>(*err); }

	constexpr E& error() & noexcept { return *err; }
	constexpr const E& error() const& noexcept { return *err; }
	constexpr E&& error() && noexcept { return std::move(*err); }
	constexpr const E&& error() const&& noexcept { return std::move(*err); }

	template<typename G>
		requires std::copy_constructible<E> && std::convertible_to<G&&, E>
	constexpr E error_or(G&& defaultError) const&
	{
		return err ? *err : static_cast<E>(std::forward<G>(defaultError));
	}

	template<typename G>
		requires std::move_constructible<E> && std::convertible_to<G&&, E>
	constexpr E error_or(G&& defaultError) &&
	{
		return err ? std::move(*err) : static_cast<E>(std::forward<G>(defaultError));
	}

public:
	template<std::invocable F>
	constexpr auto and_then(F&& f) &
	{
		using Ret = std::invoke_result_t<F>;
		if(has_value()) return std::invoke(std::forward<F>(f));
		return Ret(unexpected<typename Ret::error_type>(static_cast<typename Ret::error_type>(*err)));
	}

	template<std::invocable F>
	constexpr auto and_then(F&& f) const&
	{
		using Ret = std::invoke_result_t<F>;
		if(has_value()) return std::invoke(std::forward<F>(f));
		return Ret(unexpected<typename Ret::error_type>(static_cast<typename Ret::error_type>(*err)));
	}

	template<std::invocable F>
	constexpr auto and_then(F&& f) &&
	{
		using Ret = std::invoke_result_t<F>;
		if(has_value()) return std::invoke(std::forward<F>(f));
		return Ret(unexpected<typename Ret::error_type>(static_cast<typename Ret::error_type>(std::move(*err))));
	}

	template<std::invocable F>
	constexpr auto and_then(F&& f) const&&
	{
		using Ret = std::invoke_result_t<F>;
		if(has_value()) return std::invoke(std::forward<F>(f));
		return Ret(unexpected<typename Ret::error_type>(static_cast<typename Ret::error_type>(std::move(*err))));
	}

	template<std::invocable<E&> F>
	constexpr auto or_else(F&& f) &
	{
		using Ret = std::invoke_result_t<F, E&>;
		if(has_value()) return Ret();
		return std::invoke(std::forward<F>(f), *err);
	}

	template<std::invocable<const E&> F>
	constexpr auto or_else(F&& f) const&
	{
		using Ret = std::invoke_result_t<F, const E&>;
		if(has_value()) return Ret();
		return std::invoke(std::forward<F>(f), *err);
	}

	template<std::invocable<E&&> F>
	constexpr auto or_else(F&& f) &&
	{
		using Ret = std::invoke_result_t<F, E&&>;
		if(has_value()) return Ret();
		return std::invoke(std::forward<F>(f), std::move(*err));
	}

	template<std::invocable<const E&&> F>
	constexpr auto or_else(F&& f) const&&
	{
		using Ret = std::invoke_result_t<F, const E&&>;
		if(has_value()) return Ret();
		return std::invoke(std::forward<F>(f), std::move(*err));
	}

	template<std::invocable F>
	constexpr auto transform(F&& f) &
	{
		using U = std::remove_cv_t<std::invoke_result_t<F>>;
		using Ret = expected<U, E>;
		if(has_value())
		{
			if constexpr (std::is_void_v<U>)
			{
				std::invoke(std::forward<F>(f));
				return Ret();
			}
			else return Ret(std::invoke(std::forward<F>(f)));
		}
		return Ret(unexpected<E>(*err));
	}

	template<std::invocable F>
	constexpr auto transform(F&& f) const&
	{
		using U = std::remove_cv_t<std::invoke_result_t<F>>;
		using Ret = expected<U, E>;
		if(has_value())
		{
			if constexpr (std::is_void_v<U>)
			{
				std::invoke(std::forward<F>(f));
				return Ret();
			}
			else return Ret(std::invoke(std::forward<F>(f)));
		}
		return Ret(unexpected<E>(*err));
	}

	template<std::invocable F>
	constexpr auto transform(F&& f) &&
	{
		using U = std::remove_cv_t<std::invoke_result_t<F>>;
		using Ret = expected<U, E>;
		if(has_value())
		{
			if constexpr (std::is_void_v<U>)
			{
				std::invoke(std::forward<F>(f));
				return Ret();
			}
			else return Ret(std::invoke(std::forward<F>(f)));
		}
		return Ret(unexpected<E>(std::move(*err)));
	}

	template<std::invocable F>
	constexpr auto transform(F&& f) const&&
	{
		using U = std::remove_cv_t<std::invoke_result_t<F>>;
		using Ret = expected<U, E>;
		if(has_value())
		{
			if constexpr (std::is_void_v<U>)
			{
				std::invoke(std::forward<F>(f));
				return Ret();
			}
			else return Ret(std::invoke(std::forward<F>(f)));
		}
		return Ret(unexpected<E>(std::move(*err)));
	}

	template<std::invocable<E&> F>
	constexpr auto transform_error(F&& f) &
	{
		using G = std::remove_cv_t<std::invoke_result_t<F, E&>>;
		using Ret = expected<void, G>;
		if(has_value()) return Ret();
		return Ret(unexpected<G>(std::invoke(std::forward<F>(f), *err)));
	}

	template<std::invocable<const E&> F>
	constexpr auto transform_error(F&& f) const&
	{
		using G = std::remove_cv_t<std::invoke_result_t<F, const E&>>;
		using Ret = expected<void, G>;
		if(has_value()) return Ret();
		return Ret(unexpected<G>(std::invoke(std::forward<F>(f), *err)));
	}

	template<std::invocable<E&&> F>
	constexpr auto transform_error(F&& f) &&
	{
		using G = std::remove_cv_t<std::invoke_result_t<F, E&&>>;
		using Ret = expected<void, G>;
		if(has_value()) return Ret();
		return Ret(unexpected<G>(std::invoke(std::forward<F>(f), std::move(*err))));
	}

	template<std::invocable<const E&&> F>
	constexpr auto transform_error(F&& f) const&&
	{
		using G = std::remove_cv_t<std::invoke_result_t<F, const E&&>>;
		using Ret = expected<void, G>;
		if(has_value()) return Ret();
		return Ret(unexpected<G>(std::invoke(std::forward<F>(f), std::move(*err))));
	}

private:
	std::optional<E> err;
};
} // namespace JK::Wrap::Details::Expecteds

namespace JK::Wrap
{
using Details::Expecteds::expected;
using Details::Expecteds::unexpected;
using Details::Expecteds::bad_expected_access;

template<typename T>
using Result = expected<T, std::error_code>;
}

#endif
