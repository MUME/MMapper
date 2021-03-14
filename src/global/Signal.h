#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <cassert>
#include <functional>
#include <list>
#include <memory>
#include <type_traits>
#include <QDebug>

#include "RuleOf5.h"
#include "utils.h"

template<typename... Args>
struct NODISCARD Signal;

template<typename... Args>
struct NODISCARD Connection : public std::enable_shared_from_this<Connection<Args...>>
{
public:
    using Function = std::function<void(Args...)>;
    using Signal = ::Signal<Args...>;

private:
    struct NODISCARD this_is_private final
    {
        explicit this_is_private(int) {}
    };

private:
    Signal *m_signal = nullptr;
    Function m_function;

private:
    friend Signal;
    NODISCARD static std::shared_ptr<Connection> alloc(Signal &signal, Function function)
    {
        return std::make_shared<Connection>(this_is_private{0}, signal, std::move(function));
    }

public:
    NODISCARD static constexpr bool inline hasValidArgTypes()
    {
        constexpr bool noReferences = !std::disjunction_v<std::is_reference<Args>...>;
        constexpr bool allCopyConstructible = std::conjunction_v<std::is_copy_constructible<Args>...>;
        return noReferences && allCopyConstructible;
    }

public:
    explicit Connection(this_is_private, Signal &signal, Function function)
        : m_signal{&signal}
        , m_function(std::move(function))
    {
        static_assert(hasValidArgTypes());
    }

    ~Connection()
    {
        // CAUTION: destructor must not use shared_from_this().
        disconnect();
    }

    DELETE_CTORS_AND_ASSIGN_OPS(Connection);

    void disconnect()
    {
        if (auto sig = std::exchange(m_signal, nullptr)) {
            m_function = nullptr;
            sig->disconnect(*this);
        }
    }

private:
    void invoke(Args... args) const
    {
        if (isValid())
            m_function(args...);
    }

public:
    NODISCARD bool isValid() const { return m_signal != nullptr && m_function != nullptr; }
    explicit operator bool() const { return isValid(); }
};

// NOTE: This is not related to SignalBlocker, which is for QObject.
template<typename... Args>
struct NODISCARD Signal
{
public:
    using Connection = ::Connection<Args...>;
    using Function = typename Connection::Function;
    using SharedConnection = std::shared_ptr<Connection>;
    using WeakConnection = std::weak_ptr<Connection>;

private:
    std::list<WeakConnection> m_connections;
    int m_disableCount = 0;

public:
    Signal() { static_assert(Connection::hasValidArgTypes()); }
    DELETE_CTORS_AND_ASSIGN_OPS(Signal);

    ~Signal() { disconnectAll(); }

    void disconnectAll()
    {
        for (const WeakConnection &weakConnection : std::exchange(m_connections, {})) {
            if (const SharedConnection sharedConnection = weakConnection.lock()) {
                sharedConnection->disconnect();
            }
        }
    }

private:
    static void reportException()
    {
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception &ex) {
            qWarning() << "Exception: " << ex.what();
        } catch (...) {
            qWarning() << "Unknown exception.";
        }
    }

public:
    void invoke(Args... args)
    {
        assert(m_disableCount >= 0);
        if (m_disableCount > 0)
            return;

        m_connections.remove_if(
            [tuple = std::tuple<Args...>(args...)](const WeakConnection &weakConnection) {
                if (const SharedConnection sharedConnection = weakConnection.lock()) {
                    try {
                        const Connection &connection = *sharedConnection;
                        std::apply(
                            [&connection](Args... tupleArgs) {
                                ///
                                connection.invoke(tupleArgs...);
                            },
                            tuple);
                        return false;
                    } catch (...) {
                        reportException();
                        qInfo() << "Automatically removing connection that threw an exception";
                        return true;
                    }
                } else {
                    return true;
                }
            });
    }

    void operator()(Args &&... args) { invoke(std::forward<Args>(args)...); }

public:
    NODISCARD SharedConnection connect(Function function)
    {
        SharedConnection connection = Connection::alloc(*this, std::move(function));
        m_connections.emplace_back(connection);
        return connection;
    }

    template<typename T>
    using MemberFunctionPtr = void (T::*)(Args...);

    template<typename T>
    NODISCARD SharedConnection connectMember(T &obj, MemberFunctionPtr<T> pfn)
    {
        if (pfn == nullptr)
            throw NullPointerException();
        return connect([&obj, pfn](Args... args) { (obj.*pfn)(args...); });
    }

    void disconnect(const SharedConnection &connection)
    {
        if (connection)
            disconnect(*connection);
    }

    void disconnect(const Connection &toRemove)
    {
        if (m_connections.empty())
            return;

        m_connections.remove_if([&toRemove](WeakConnection &weakConnection) {
            if (auto shared = weakConnection.lock()) {
                return shared.get() == &toRemove;
            }
            return true;
        });
    }

public:
    struct NODISCARD ReEnabler final
    {
    private:
        friend Signal;
        Signal &m_self;
        explicit ReEnabler(Signal &self)
            : m_self{self}
        {}

    public:
        DELETE_CTORS_AND_ASSIGN_OPS(ReEnabler);
        ~ReEnabler()
        {
            assert(m_self.m_disableCount > 0);
            --m_self.m_disableCount;
        }
    };

    NODISCARD ReEnabler disable()
    {
        ++m_disableCount;
        return ReEnabler{*this};
    }
};
