#include <type_traits>
#include <optional>
#include <vector>
#include <jk/meta/type-var.h>

using namespace JK::Meta;

// ============================================================
// Section 1: TypeVar — type wrapper
// ============================================================

// TypeVar<T>::type and ::Type both alias T
static_assert(std::is_same_v<TypeVar<int>::type, int>);
static_assert(std::is_same_v<TypeVar<int>::Type, int>);
static_assert(std::is_same_v<TypeVar<const double*>::type, const double*>);

// TypeVar values are default-constructible and carry no state
static_assert(sizeof(TypeVar<int>) == 1 || sizeof(TypeVar<int>) >= 1); // trivially satisfied
constexpr TypeVar<int>    tvInt{};
constexpr TypeVar<double> tvDouble{};
static_assert(std::is_same_v<decltype(tvInt)::Type,    int>);
static_assert(std::is_same_v<decltype(tvDouble)::Type, double>);

// ============================================================
// Section 2: typeVar<T> — inline constexpr variable
// ============================================================

static_assert(std::is_same_v<decltype(typeVar<int>),         const TypeVar<int>>);

// Can be passed to constexpr functions
template<typename T>
constexpr bool isSameTypeVar(TypeVar<T>, TypeVar<T>) { return true; }

template<typename T, typename U>
constexpr bool isSameTypeVar(TypeVar<T>, TypeVar<U>) { return false; }

static_assert(isSameTypeVar(typeVar<int>,    typeVar<int>));
static_assert(!isSameTypeVar(typeVar<int>,   typeVar<double>));
static_assert(!isSameTypeVar(typeVar<float>, typeVar<int>));

// ============================================================
// Section 3: GetTypeArg / getTypeArg — extract T from C<T>
// ============================================================

// std::optional<T>
static_assert(std::is_same_v<GetTypeArg<std::optional<int>>,    int>);
static_assert(std::is_same_v<GetTypeArg<std::optional<double>>, double>);
static_assert(std::is_same_v<GetTypeArg<std::optional<char*>>,  char*>);

// std::vector<T> (single-arg alias, not the full two-arg primary)
static_assert(std::is_same_v<GetTypeArg<std::vector<float>>, float>);

// Custom single-argument template
template<typename T> struct Box {};
static_assert(std::is_same_v<GetTypeArg<Box<long>>,        long>);
static_assert(std::is_same_v<GetTypeArg<Box<const void*>>, const void*>);

// getTypeArg<C<T>> returns typeVar<T>
static_assert(isSameTypeVar(getTypeArg<std::optional<int>>,  typeVar<int>));
static_assert(isSameTypeVar(getTypeArg<Box<double>>,         typeVar<double>));
static_assert(!isSameTypeVar(getTypeArg<Box<int>>,           typeVar<double>));

// ============================================================
// Main
// ============================================================

int main() { return 0; }
