// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "AsyncTasks.h"

#include "ConfigConsts.h"
#include "global/PrintUtils.h"
#include "logging.h"
#include "thread_utils.h"

#include <future>

#include <QTimer>

namespace async_tasks {
static inline constexpr const auto verbose_debugging = IS_DEBUG_BUILD;

struct NODISCARD AsyncTask final
{
private:
    ProgressCounter m_pc;
    std::string m_name;
    BackgroundWorker m_backgroundWorker;
    OnSuccess m_onSuccess;
    std::future<void> m_future;

public:
    explicit AsyncTask(std::string name, BackgroundWorker backgroundWorker, OnSuccess onSuccess)
        : m_name{std::move(name)}
        , m_backgroundWorker{std::move(backgroundWorker)}
        , m_onSuccess{std::move(onSuccess)}
        , m_future{std::async(std::launch::async, &AsyncTask::worker, this)}
    {
        MMLOG_INFO() << "Created async task " << getNameQuoted();
    }

    ~AsyncTask()
    {
        if (isRunning()) {
            m_pc.requestCancel();
            complete();
        }
        MMLOG_INFO() << "Destroying async task " << getNameQuoted();
    }

public:
    NODISCARD bool isRunning() const { return m_future.valid(); }
    NODISCARD const std::string &getName() const { return m_name; }
    NODISCARD QuotedString getNameQuoted() const { return QuotedString{m_name}; }

private:
    void complete()
    {
        ABORT_IF_NOT_ON_MAIN_THREAD();
        if (!isRunning()) {
            std::abort();
        }

        try {
            m_future.get();
        } catch (...) {
            MMLOG_WARNING() << "Exception during future.get(); will not invoke completion function.";
            return;
        }

        if (verbose_debugging) {
            MMLOG_DEBUG() << "Completed async task " << getNameQuoted();
        }
        try {
            m_onSuccess();
        } catch (...) {
            MMLOG_WARNING() << "Exception during completion function.";
        }
    }

private:
    void worker() { m_backgroundWorker(m_pc); }

public:
    void requestCancel() { m_pc.requestCancel(); }
    NODISCARD bool tryComplete()
    {
        if (m_future.wait_for(std::chrono::nanoseconds{0}) == std::future_status::timeout) {
            return false;
        }

        complete();
        return true;
    }

    void reportStatus() const
    {
        // REVISIT: The output of this may need to be throttled for long-running tasks.
        const auto status = m_pc.getStatus();
        MMLOG_INFO() << getNameQuoted() << " [" << status.percent() << "%] "
                     << QuotedString{std::string{status.msg.getStdStringViewUtf8()}};
    }
};

class NODISCARD AsyncTasks final
{
private:
    static inline constexpr auto g_timer_period = std::chrono::milliseconds(250);

private:
    std::list<AsyncTask> m_tasks;
    QTimer m_timer;

public:
    explicit AsyncTasks()
    {
        QObject::connect(&m_timer, &QTimer::timeout, [this]() { onTimer(); });
        m_timer.setInterval(g_timer_period);
    }
    ~AsyncTasks()
    {
        m_timer.stop();
        for (auto &task : m_tasks) {
            task.requestCancel();
        }
    }

private:
    void reportStatus()
    {
        for (auto &task : m_tasks) {
            if (task.isRunning() && !task.tryComplete()) {
                task.reportStatus();
            }
        }
    }

    void filterTasks()
    {
        std::ignore = utils::listRemoveIf(m_tasks, [](const AsyncTask &task) -> bool {
            if (task.isRunning()) {
                return false;
            }
            if (verbose_debugging) {
                MMLOG_DEBUG() << "Removing entry for " << task.getNameQuoted();
            }
            return true;
        });
    }

private:
    void onTimer()
    {
        ABORT_IF_NOT_ON_MAIN_THREAD();
        const auto count = m_tasks.size();
        if (verbose_debugging) {
            MMLOG_INFO() << "tick with " << count << " " << ((count == 1) ? "entry" : "entries");
        }
        filterTasks();
        if (m_tasks.empty()) {
            m_timer.stop();
        } else {
            reportStatus();
        }
    }

public:
    void start(std::string name, BackgroundWorker backgroundWorker, OnSuccess onSuccess)
    {
        ABORT_IF_NOT_ON_MAIN_THREAD();
        m_tasks.emplace_back(std::move(name), std::move(backgroundWorker), std::move(onSuccess));
        if (!m_timer.isActive()) {
            m_timer.start();
        }
    }
};

static std::unique_ptr<AsyncTasks> g_tasks;

void init()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    if (g_tasks != nullptr) {
        std::abort();
    }
    g_tasks = std::make_unique<AsyncTasks>();
}

void cleanup()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    std::ignore = deref(g_tasks);
    g_tasks.reset();
}

void startAsyncTask(std::string name, BackgroundWorker backgroundWorker, OnSuccess onSuccess)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    deref(g_tasks).start(std::move(name), std::move(backgroundWorker), std::move(onSuccess));
}

} // namespace async_tasks
