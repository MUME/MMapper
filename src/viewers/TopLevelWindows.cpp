// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "TopLevelWindows.h"

#include "global/ConfigConsts.h"
#include "global/thread_utils.h"
#include "global/utils.h"
#include "global/window_utils.h"

#include <list>
#include <memory>

#include <QDebug>
#include <QMainWindow>
#include <QPointer>
#include <QTimer>

namespace { // anonymous
struct NODISCARD TopLevelWindows final
{
private:
    // periodically free up memory from windows that have been closed by the user
    static inline constexpr const auto g_timer_period = std::chrono::seconds{5};
    static inline constexpr const auto verbose_debugging = false; // IS_DEBUG_BUILD;

private:
    class NODISCARD Entry final
    {
    private:
        std::unique_ptr<QMainWindow> m_unique;
        QString m_name;
        QPointer<QMainWindow> m_weak_ptr;

    public:
        explicit Entry(std::unique_ptr<QMainWindow> ptr)
            : m_unique{std::move(ptr)}
            , m_name{deref(m_unique).windowTitle()}
            , m_weak_ptr{m_unique.get()}
        {
            qInfo() << "Added top level window" << getName();
        }
        ~Entry()
        {
            //
            qInfo() << "Removed top level window" << getName();
        }

    public:
        // NODISCARD bool exists() const { return !weak_ptr.isNull(); }
        NODISCARD const QString &getName() const { return m_name; }

    private:
        NODISCARD QMainWindow *try_get() { return m_weak_ptr.data(); }
        NODISCARD const QMainWindow *try_get() const { return m_weak_ptr.data(); }

    public:
        NODISCARD bool isVisible() const
        {
            if (const QMainWindow *const ptr = try_get()) {
                const QMainWindow &w = deref(ptr);
                if (w.isVisible()) {
                    return true;
                }
            }
            return false;
        }

        void disconnectAllChildren()
        {
            if (QMainWindow *const ptr = try_get()) {
                if (verbose_debugging) {
                    qDebug() << "Disconnecting all chidren of window" << getName();
                }
                mmqt::rdisconnect(ptr);
            }
        }

        void deleteWindowLater()
        {
            if (QMainWindow *const ptr = try_get()) {
                QMainWindow &w = deref(ptr);
                if (verbose_debugging) {
                    qDebug() << "Marking window" << getName() << "for destruction.";
                }
                w.deleteLater();
            }
        }

        void zap()
        {
            if (verbose_debugging) {
                qDebug() << "Zapping" << getName();
            }
            m_unique.reset();
        }
    };

private:
    std::list<Entry> m_entries;
    QTimer m_timer;

public:
    TopLevelWindows()
    {
        QObject::connect(&m_timer, &QTimer::timeout, [this]() { onTimer(); });
        m_timer.setInterval(g_timer_period);
        m_timer.start();
    }

    ~TopLevelWindows()
    {
        // paranoia, in case these are somehow connected to one another
        // first disconnect everything, then flag them for future deletion,
        // and then actually delete them.
        m_timer.stop();
        filterWindows();
        for (Entry &entry : m_entries) {
            entry.disconnectAllChildren();
        }
        filterWindows();
        for (Entry &entry : m_entries) {
            entry.deleteWindowLater();
        }
        filterWindows();
        for (Entry &entry : m_entries) {
            entry.zap();
        }
        filterWindows();
    }

private:
    void filterWindows()
    {
        std::ignore = utils::listRemoveIf(m_entries, [](const Entry &entry) -> bool {
            if (entry.isVisible()) {
                return false;
            }

            if (verbose_debugging) {
                qDebug() << "Removing entry for" << entry.getName();
            }
            return true;
        });
    }

private:
    void onTimer()
    {
        ABORT_IF_NOT_ON_MAIN_THREAD();
        const auto count = m_entries.size();
        if (verbose_debugging) {
            qDebug() << "tick with" << count << ((count == 1) ? "entry" : "entries");
        }
        filterWindows();

        if (m_entries.empty()) {
            m_timer.stop();
        }
    }

public:
    void add(std::unique_ptr<QMainWindow> pWindow)
    {
        auto &window = deref(pWindow);
        const auto title = window.windowTitle();

        if (!window.isVisible()) {
            window.setVisible(true);
            if (!window.isVisible()) {
                qWarning() << "Unable to make window " << title << "visible.";
                return;
            }
        }

        if (verbose_debugging) {
            qDebug() << "Adding window" << title;
        }
        MAYBE_UNUSED auto &entry = m_entries.emplace_back(std::move(pWindow));
        assert(entry.isVisible());

        if (!m_timer.isActive()) {
            m_timer.start();
        }
    }
};

std::unique_ptr<TopLevelWindows> g_topLevelWindows;

} // namespace

void initTopLevelWindows()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    if (g_topLevelWindows != nullptr) {
        std::abort();
    }
    g_topLevelWindows = std::make_unique<TopLevelWindows>();
}

void destroyTopLevelWindows()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    std::ignore = deref(g_topLevelWindows);
    g_topLevelWindows.reset();
}

void addTopLevelWindow(std::unique_ptr<QMainWindow> window)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    deref(g_topLevelWindows).add(std::move(window));
}
