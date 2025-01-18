#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "Badge.h"
#include "Signal.h"
#include "macros.h"
#include "utils.h"

#include <functional>
#include <memory>
#include <vector>

class NODISCARD Signal2Lifetime final
{
public:
    struct NODISCARD Obj final
    {};

private:
    std::shared_ptr<Obj> m_obj = std::make_shared<Obj>();

public:
    explicit Signal2Lifetime() = default;
    ~Signal2Lifetime() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(Signal2Lifetime);

    NODISCARD std::shared_ptr<Obj> getObj() const { return m_obj; }
};

template<typename... Args>
class NODISCARD Signal2 final
{
public:
    using Function = std::function<void(Args...)>;

private:
    struct NODISCARD Data final
    {
        Function function;
        std::weak_ptr<Signal2Lifetime::Obj> weak;
        Data(Function f, const std::shared_ptr<Signal2Lifetime::Obj> &shared)
            : function{std::move(f)}
            , weak{shared}
        {}
    };
    std::vector<Data> m_callbacks;
    bool m_invoking = false;

public:
    Signal2() = default;
    ~Signal2() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(Signal2);

public:
    void invoke(Args... args)
    {
        static_assert(sizeof...(args) == 0 || Connection<Args...>::hasValidArgTypes());
        if (m_invoking) {
            throw std::runtime_error("recursion");
        }

        m_invoking = true;

        utils::erase_if(m_callbacks,
                        [tuple = std::tuple<Args...>(args...)](const auto &data) -> bool {
                            // here we avoid std::ignore to guarantee the object stays alive
                            // for the duration of the call, just in case it matters.
                            if (MAYBE_UNUSED const auto ignored = data.weak.lock()) {
                                try {
                                    std::apply(data.function, tuple);
                                    return false;
                                } catch (const std::exception &ex) {
                                    qWarning().nospace()
                                        << "Exception in signal handler: [" << ex.what() << "]";
                                } catch (...) {
                                    qWarning() << "Unknown exception in signal handler.";
                                }
                            }
                            return true;
                        });

        m_invoking = false;
    }
    void operator()(Args... args) { invoke(std::move(args)...); }

public:
    void connect(const Signal2Lifetime &lifetime, Function f)
    {
        if (auto shared = lifetime.getObj()) {
            m_callbacks.emplace_back(std::move(f), shared);
        } else {
            throw std::runtime_error("expired lifetime");
        }
    }

public:
    // Note: This function call does not query the object lifetimes;
    // connections are only removed during invoke().
    NODISCARD size_t getNumConnected() const { return m_callbacks.size(); }
};
