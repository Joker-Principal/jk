#pragma once
#include <algorithm> // std::copy_n
#include <string_view>
#include <jk/meta/compiler.h>

namespace JK::Details::Meta::Literals
{
template<typename C, std::size_t N>
struct StringLiteral;

template<typename T>
struct IsStringLiteralTrait : std::false_type {};

template<typename C, std::size_t N>
struct IsStringLiteralTrait<StringLiteral<C, N>> : std::true_type {};

template<auto literal>
constexpr bool isStringLiteral = IsStringLiteralTrait<std::decay_t<decltype(literal)>>::value;

template<auto literal>
concept IsStringLiteral = isStringLiteral<literal>;

template<typename C, std::size_t N>
struct StringLiteral
{
public:
    static_assert(N >= 1, "StringLiteral must be compatible to a null-terminated string.");
	using Char = C;
    using StdStringView = std::basic_string_view<Char>;

    constexpr StringLiteral() : value{} {}
    constexpr StringLiteral(const Char (&str)[N]) : value{} { std::copy_n(str, N, value); }
    explicit constexpr StringLiteral(const Char* str) : value{} { std::copy_n(str, N, value); }

    constexpr StringLiteral(const StringLiteral&) noexcept = default;
    constexpr StringLiteral& operator=(const StringLiteral&) & noexcept = default;
    
    constexpr StringLiteral(StringLiteral&&) noexcept = default;
    constexpr StringLiteral& operator=(StringLiteral&&) & noexcept = default;

    JK_CONSTEXPR_DESTRUCTOR ~StringLiteral() noexcept = default;

public:
	constexpr const Char* data() const { return value; }
	static constexpr std::size_t size() { return N - 1; }
	constexpr operator StdStringView() const { return {data(), size()}; }

public:
    constexpr bool operator==(StdStringView v) const noexcept
    {
        return StdStringView(*this) == v;
    }

    friend constexpr auto operator<=>(StringLiteral l, StdStringView v) noexcept
    {
        return StdStringView(l) <=> v;
    }

public:
    template<auto prefix>
    constexpr auto removePrefix() const noexcept
    {
        static_assert(isStringLiteral<prefix>);
        using Cp = typename decltype(prefix)::Char;
        constexpr auto Np = prefix.size();
        static_assert(N >= Np, "input must be longer than prefix");
        static_assert(std::is_same_v<Cp, Char>, "input and prefix must be same type");

        if constexpr (StdStringView(*this).starts_with(StdStringView(prefix)))
        {
            using Result = StringLiteral<Char, N - Np + 1>;
            return Result(data() + prefix.size());
        }
        else
        {
            return *this;
        }
    }

public:
    Char value[N];
};

template<typename Char, std::size_t N>
StringLiteral(const Char (&str)[N]) -> StringLiteral<Char, N>;
} // namespace JK::Details::Meta::Literals

namespace JK::Meta::Literals
{
using JK::Details::Meta::Literals::StringLiteral;
using JK::Details::Meta::Literals::isStringLiteral;
using JK::Details::Meta::Literals::IsStringLiteral;
}
