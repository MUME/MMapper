// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "WeakHandle.h"

#include "ConfigConsts.h"

#include <cassert>
#include <string>

namespace {

class Foo final : public EnableGetWeakHandleFromThis<Foo>
{
private:
    std::string s = "Foo";
};

class Bar final
{
private:
    WeakHandleLifetime<Bar> lifetime{*this};
    std::string s = "Bar";

public:
    WeakHandle<Bar> getWeakHandle() { return lifetime.getWeakHandle(); }
    WeakHandle<const Bar> getWeakHandle() const { return lifetime.getWeakHandle(); }
};

template<typename T>
NODISCARD static inline WeakHandle<T> getWeakHandle(T &x) = delete;

template<>
inline WeakHandle<Foo> getWeakHandle(Foo &foo)
{
    return foo.getWeakHandleFromThis();
}
template<>
inline WeakHandle<const Foo> getWeakHandle(const Foo &foo)
{
    return foo.getWeakHandleFromThis();
}
template<>
inline WeakHandle<Bar> getWeakHandle(Bar &bar)
{
    return bar.getWeakHandle();
}
template<>
inline WeakHandle<const Bar> getWeakHandle(const Bar &bar)
{
    return bar.getWeakHandle();
}

template<typename T>
NODISCARD static inline bool tryVisit(const WeakHandle<T> &handle)
{
    return handle.acceptVisitor([](const T &) -> void {
        // could print here
    });
};

template<typename T>
NODISCARD static inline constexpr bool has_expected_properties()
{
    return std::is_default_constructible_v<T>  //
           && !std::is_copy_constructible_v<T> //
           && !std::is_copy_assignable_v<T>    //
           && !std::is_move_constructible_v<T> //
           && !std::is_move_assignable_v<T>;   //
}

template<typename T>
static void test()
{
    static_assert(has_expected_properties<T>());

    assert(!tryVisit(WeakHandle<Foo>()));
    assert(tryVisit(Foo().getWeakHandleFromThis()));
    {
        WeakHandle<T> handle;
        {
            T foo;
            handle = getWeakHandle(foo);
            assert(tryVisit(handle));
        }
        assert(!tryVisit(handle));
    }
}

static const int test_weak_handle = []() -> int {
    if constexpr (!IS_DEBUG_BUILD) {
        return 0;
    }

    test<Foo>();
    test<const Foo>();
    test<Bar>();
    test<const Bar>();

    return 42;
}();

} // namespace
