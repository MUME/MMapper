// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "Badge.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace { // anonymous

//
// This class uses std::enable_shared_from_this<Example>, so it must always be
// constructed with make_shared. Failing to do so will result in a crash (UB),
// so the class uses the Badge idiom to enforce that invariant.
//
// Note that std::make_shared<T> requires T's constructor to be public,
// so we define a public constructor that accepts a Badge<T>,
// which can only be constructed by members of T.
//
// The only way to construct a Example is to use Example::alloc() which constructs
// and passes a Badge<Example> to std::make_shared<Example>. Therefore we know it's
// impossible to accidentally break the invariant required by
// std::enable_shared_from_this<T>.
//
struct NODISCARD Example final : public std::enable_shared_from_this<Example>
{
    std::string msg;

    NODISCARD static std::shared_ptr<Example> alloc(std::string moved_msg)
    {
        return std::make_shared<Example>(Badge<Example>{}, std::move(moved_msg));
    }

    explicit Example(Badge<Example>, std::string moved_msg)
        : msg{std::move(moved_msg)}
    {
        std::cout << "ctor\n";
    }
    ~Example() { std::cout << "dtor\n"; }
    Example(const Example &) = delete;
    Example &operator=(const Example &) = delete;
};

NODISCARD auto makeLambda(Example &ref)
{
    auto weak = std::weak_ptr<Example>{ref.shared_from_this()};
    return [weak] {
        if (auto p = weak.lock()) {
            std::cout << p->msg << "\n";
        } else if (weak.expired()) {
            std::cout << "(expired)\n";
        } else {
            std::cout << "(null)\n";
        }
    };
}

} // namespace

namespace example {
void badge_example()
{
    auto p = Example::alloc("Hello, world!"); // prints "ctor"
    auto try_greet = makeLambda(*p);          // allocates lambda
    try_greet();                              // prints "Hello, world!"
    p.reset();                                // prints "dtor"
    try_greet();                              // prints "(expired)"
}
} // namespace example
