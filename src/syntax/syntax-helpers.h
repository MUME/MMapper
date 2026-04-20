#pragma once

#include "Sublist.h"
#include "SyntaxArgs.h"

namespace syntax {
template<typename T>
NODISCARD decltype(auto) remap(T &&x)
{
    static_assert(!std::is_same_v<std::decay_t<T>, std::string_view>);
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>
                  || std::is_same_v<std::decay_t<T>, const char *>) {
        return syntax::abbrevToken(std::forward<T>(x));
    } else if constexpr (true) {
        return std::forward<T>(x);
    }

    std::abort();
}

template<typename... Args>
NODISCARD auto syn(Args &&...args)
{
    return syntax::buildSyntax(remap(std::forward<Args>(args))...);
}
} // namespace syntax
