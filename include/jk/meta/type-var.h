#pragma once

namespace JK::Meta::Details::TypeVar
{
template<typename T>
struct TypeVar 
{
    using type = T;
    using Type = T;
};

template<typename T>
inline constexpr auto typeVar = TypeVar<T>{};

template<template<typename...> typename C, typename T>
auto getTypeArgTrait(C<T>*) -> TypeVar<T>;

template<typename T>
using GetTypeArg = typename decltype(getTypeArgTrait(static_cast<T*>(nullptr)))::type;

template<typename T>
inline constexpr auto getTypeArg = typeVar<GetTypeArg<T>>;
} // namespace JK::Meta::Details::TypeVar

namespace JK::Meta
{
using Details::TypeVar::TypeVar;
using Details::TypeVar::typeVar;
using Details::TypeVar::GetTypeArg;
using Details::TypeVar::getTypeArg;
}
