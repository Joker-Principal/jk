module;
#include <jk/meta/compiler.h>
#include <jk/meta/type-list.h>
#include <jk/meta/callables.h>
#include <jk/wrap/string.h>
#include <jk/wrap/location.h>

export module jk;

export namespace JK
{
namespace Meta
{
// type-list.h
using JK::Meta::TList;
using JK::Meta::Merge;
using JK::Meta::From;
using JK::Meta::Zip;
using JK::Meta::Unite;
using JK::Meta::OneOf;

// callables.h
using JK::Meta::isFunctor;
using JK::Meta::isFunction;
using JK::Meta::invoke_r;
using JK::Meta::ForwardLike;
using JK::Meta::Callable;
using JK::Meta::isCallable;
using JK::Meta::Return;
using JK::Meta::Class;
using JK::Meta::Arguments;
using JK::Meta::Signature;
using JK::Meta::isVariadic;
using JK::Meta::isNothrow;
using JK::Meta::CallableValue;
using JK::Meta::CallableOf;
using JK::Meta::fn;
using JK::Meta::ReturnOf;
using JK::Meta::ClassOf;
using JK::Meta::ArgumentsOf;
using JK::Meta::SignatureOf;
using JK::Meta::invokeAs;
using JK::Meta::returnAs;
using JK::Meta::argAs;
using JK::Meta::GenericFunctor;
using JK::Meta::Visitor;
using JK::Meta::curry;
using JK::Meta::bind;
using JK::Meta::unbind;
using JK::Meta::FunctionRef;
using JK::Meta::constructor;
using JK::Meta::creator;
using JK::Meta::unary;
using JK::Meta::IsMemberOf;
using JK::Meta::MemberOf;
using JK::Meta::UseInitializerList;
using JK::Meta::useInit;
using JK::Meta::Lazy;
using JK::Meta::lazy;
using JK::Meta::Cleanup;
using JK::Meta::resetCleans;
} // namespace Meta

namespace Wrap
{
using JK::Wrap::trim;
using JK::Wrap::trimLeft;
using JK::Wrap::trimRight;
using JK::Wrap::removePrefix;
using JK::Wrap::removeSuffix;
using JK::Wrap::join;
using JK::Wrap::hex;
using JK::Wrap::lower;
using JK::Wrap::upper;
using JK::Wrap::Where;
using JK::Wrap::where;
using JK::Wrap::Location;
} // namespace Wrap
} // namespace JK
