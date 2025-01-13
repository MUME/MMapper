// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "WeakHandle.h"

#include "ConfigConsts.h"
#include "tests.h"

#include <string>

namespace { // anonymous

class NODISCARD Foo final : public EnableGetWeakHandleFromThis<Foo>
{
private:
    std::string m_s = "Foo";
};

class NODISCARD Bar final
{
private:
    WeakHandleLifetime<Bar> m_lifetime{*this};
    std::string m_s = "Bar";

public:
    NODISCARD WeakHandle<Bar> getWeakHandle() { return m_lifetime.getWeakHandle(); }
    NODISCARD WeakHandle<const Bar> getWeakHandle() const { return m_lifetime.getWeakHandle(); }
};

template<typename T>
NODISCARD WeakHandle<T> getWeakHandle(T &x) = delete;

template<>
WeakHandle<Foo> getWeakHandle(Foo &foo)
{
    return foo.getWeakHandleFromThis();
}
template<>
WeakHandle<const Foo> getWeakHandle(const Foo &foo)
{
    return foo.getWeakHandleFromThis();
}
template<>
WeakHandle<Bar> getWeakHandle(Bar &bar)
{
    return bar.getWeakHandle();
}
template<>
WeakHandle<const Bar> getWeakHandle(const Bar &bar)
{
    return bar.getWeakHandle();
}

template<typename T>
NODISCARD bool tryVisit(const WeakHandle<T> &handle)
{
    return handle.acceptVisitor([](const T &) -> void {
        // could print here
    });
};

template<typename T>
NODISCARD constexpr bool has_expected_properties()
{
    return std::is_default_constructible_v<T>  //
           && !std::is_copy_constructible_v<T> //
           && !std::is_copy_assignable_v<T>    //
           && !std::is_move_constructible_v<T> //
           && !std::is_move_assignable_v<T>;   //
}

template<typename T>
void test_weak_handle()
{
    static_assert(has_expected_properties<T>());

    TEST_ASSERT(!tryVisit(WeakHandle<Foo>()));
    TEST_ASSERT(tryVisit(Foo().getWeakHandleFromThis()));
    {
        WeakHandle<T> handle;
        {
            T foo;
            handle = getWeakHandle(foo);
            TEST_ASSERT(tryVisit(handle));
        }
        TEST_ASSERT(!tryVisit(handle));
    }
}

} // namespace

namespace test {
void testWeakHandle()
{
    ::test_weak_handle<Foo>();
    ::test_weak_handle<const Foo>();
    ::test_weak_handle<Bar>();
    ::test_weak_handle<const Bar>();
}
} // namespace test
