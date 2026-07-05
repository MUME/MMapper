// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "AsyncTasks.h"

#include "../global/PrintUtils.h"
#include "ConfigConsts.h"
#include "SendToUser.h"
#include "logging.h"
#include "thread_utils.h"

#include <chrono>
#include <future>

#include <QTimer>

namespace { // anonymous
constexpr auto green = getRawAnsi(AnsiColor16Enum::green);
constexpr auto red = getRawAnsi(AnsiColor16Enum::red);
constexpr auto yellow = getRawAnsi(AnsiColor16Enum::yellow);

auto coloredString(const std::string_view msg)
{
    return ColoredQuotedStringView{green, yellow, msg};
}

} // namespace

namespace async_tasks {
static inline constexpr auto verbose_debugging = IS_DEBUG_BUILD;

std::string get_type_name(const AsyncTaskTypeEnum type)
{
    switch (type) {
        using enum AsyncTaskTypeEnum;
    case IO:
        return "IO Task";
    case Task:
        return "Task";
    }
    return "(error)";
}

static void formatTaskNameId(AnsiOstream &aos, const TaskNameIdType task)
{
    aos << get_type_name(task.type) << " #" << ColoredValue{green, task.id} << " ("
        << coloredString(task.name) << ")";
}
static void formatTaskNameId(AnsiOstream &aos, const AsyncTaskHandle &task)
{
    formatTaskNameId(aos, task.getNameIdType());
}

void formatElapsedSeconds(AnsiOstream &aos, const Clock::duration dt)
{
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(dt).count();
    auto min = sec / 60;
    auto hrs = min / 60;
    const auto days = hrs / 24;
    hrs %= 24;
    min %= 60;
    sec %= 60;

    auto prefix = "";
    size_t printed = 0;
    const auto show = [&aos, &prefix, &printed](const auto val, const auto suffix) {
        if (printed >= 2) {
            return;
        }
        aos << prefix << ColoredValue{green, val} << suffix;
        prefix = " ";
        ++printed;
    };
    if (days > 0) {
        show(days, "d");
    }
    if (hrs > 0) {
        show(hrs, "h");
    }
    if (min > 0) {
        show(min, "m");
    }
    if (sec > 0 || printed == 0) {
        show(sec, "s");
    }
}

namespace AsyncTasksWatcher {
static void started(const AsyncTaskHandle &task)
{
    if (verbose_debugging) {
        global::logOnly(global::LogDestEnum::Debug, [&task](AnsiOstream &aos) {
            formatTaskNameId(aos, task.getNameIdType());
            aos << " started.";
        });
    }
}
// the async portion ended, but it hasn't run the callback function on the main thread yet.
static void completedAsync(const AsyncTaskHandle &task)
{
    if (verbose_debugging) {
        global::logOnly(global::LogDestEnum::Debug, [&task](AnsiOstream &aos) {
            formatTaskNameId(aos, task.getNameIdType());
            aos << " completed its background execution.";
        });
    }
}
static void finished(const AsyncTaskHandle &task)
{
    if (verbose_debugging) {
        global::logOnly(global::LogDestEnum::Debug, [&task](AnsiOstream &aos) {
            formatTaskNameId(aos, task.getNameIdType());
            aos << " finished.";
        });
    }
}
static void removed(const AsyncTaskHandle &task)
{
    if (verbose_debugging) {
        global::logOnly(global::LogDestEnum::Debug, [&task](AnsiOstream &aos) {
            formatTaskNameId(aos, task.getNameIdType());
            aos << " was removed.";
        });
    }
}

}; // namespace AsyncTasksWatcher

class NODISCARD AsyncTask final : public std::enable_shared_from_this<AsyncTask>
{
private:
    static inline std::atomic_size_t g_next_id = 100;

private:
    const std::shared_ptr<ProgressCounter> m_sharedpc;
    const std::string m_name;
    BackgroundWorker2 m_backgroundWorker;
    OnSuccess2 m_onSuccess;
    const size_t m_id = g_next_id++;
    const Clock::time_point m_start = Clock::now();
    std::optional<Clock::time_point> m_end;
    TaskStatusEnum m_taskStatus = TaskStatusEnum::Allocated;
    AsyncTaskTypeEnum m_type = AsyncTaskTypeEnum::Task;
    std::atomic_bool m_isRemoved = false;
    std::future<void> m_future;

public:
    NODISCARD bool isRunningOnBackgroundThread() const { return m_future.valid(); }
    NODISCARD const std::string &getName() const { return m_name; }
    NODISCARD AsyncTaskTypeEnum getType() const { return m_type; }
    NODISCARD size_t getId() const { return m_id; }
    NODISCARD TaskNameIdType getNameIdType() const { return {getName(), getId(), getType()}; }
    NODISCARD std::shared_ptr<ProgressCounter> getSharedProgressCounter() const
    {
        return m_sharedpc;
    }
    NODISCARD ProgressCounter &getProgressCounter() const { return deref(m_sharedpc); }

    NODISCARD ProgressCounter::Status getStatus() const { return getProgressCounter().getStatus(); }
    NODISCARD Clock::time_point getStartTime() const { return m_start; }
    NODISCARD std::optional<Clock::time_point> getEndTime() const { return m_end; }
    NODISCARD Clock::duration getElapsedTime() const
    {
        return getEndTime().value_or(Clock::now()) - getStartTime();
    }
    NODISCARD TaskStatusEnum getTaskStatus() { return m_taskStatus; }
    NODISCARD AllowCancelEnum getCanCancel() const { return getProgressCounter().allowCancel(); }
    NODISCARD bool getIsRemoved() const { return m_isRemoved; }

public:
    NODISCARD static std::shared_ptr<AsyncTask> alloc(const AsyncTaskTypeEnum type,
                                                      const AllowCancelEnum allowCancel,
                                                      std::string name,
                                                      BackgroundWorker2 backgroundWorker,
                                                      OnSuccess2 onSuccess)
    {
        auto shared = std::make_shared<AsyncTask>(Badge<AsyncTask>{},
                                                  type,
                                                  allowCancel,
                                                  std::move(name),
                                                  std::move(backgroundWorker),
                                                  std::move(onSuccess));

        auto &task = deref(shared);
        task.m_future = std::async(std::launch::async, &AsyncTask::worker, &task);
        return shared;
    }

public:
    explicit AsyncTask(Badge<AsyncTask>,
                       const AsyncTaskTypeEnum type,
                       const AllowCancelEnum allowCancel,
                       std::string name,
                       BackgroundWorker2 backgroundWorker,
                       OnSuccess2 onSuccess)
        : m_sharedpc{std::make_shared<ProgressCounter>(allowCancel)}
        , m_name{std::move(name)}
        , m_backgroundWorker{std::move(backgroundWorker)}
        , m_onSuccess{std::move(onSuccess)}
        , m_type{type}
    {
        if (m_backgroundWorker == nullptr) {
            throw std::invalid_argument("backgroundWorker");
        }
        if (m_onSuccess == nullptr) {
            throw std::invalid_argument("onSuccess");
        }

        if (verbose_debugging) {
            logOnly(global::LogDestEnum::Debug, [this](auto &aos) {
                aos << "Created ";
                formatShortDesc(aos);
                aos << ".\n";
            });
        }
    }

    ~AsyncTask()
    {
        if (isRunningOnBackgroundThread()) {
            requestCancel();
            complete();
        }
        if (verbose_debugging) {
            logOnly(global::LogDestEnum::Debug, [this](auto &aos) {
                aos << "Destroying ";
                formatShortDesc(aos);
                aos << "...\n";
            });
        }
    }

private:
    void formatShortDesc(AnsiOstream &aos) const { formatTaskNameId(aos, getNameIdType()); }

private:
    NODISCARD AsyncTaskHandle getShared()
    {
        return AsyncTaskHandle{Badge<AsyncTask>{}, shared_from_this()};
    }

private:
    void complete()
    {
        ABORT_IF_NOT_ON_MAIN_THREAD();
        if (!isRunningOnBackgroundThread()) {
            std::abort();
        }

        try {
            m_future.get();
        } catch (ProgressCanceledException &) {
            m_taskStatus = TaskStatusEnum::Canceled;
            // REVISIT: use white on cyan color scheme here?
            logAndSendToUser(global::LogDestEnum::Info, [this](AnsiOstream &aos) {
                formatShortDesc(aos);
                aos << " was canceled.\n";
            });
            return;
        } catch (...) {
            m_taskStatus = TaskStatusEnum::BackgroundCrashed;
            // REVISIT: use white on cyan color scheme here?
            logAndSendToUser(global::LogDestEnum::Warning, [this](AnsiOstream &aos) {
                formatShortDesc(aos);
                aos << " threw an exception:\n";
                formatException(aos, std::current_exception());
            });
            return;
        }

        AsyncTasksWatcher::completedAsync(getShared());

        try {
            m_taskStatus = TaskStatusEnum::BegunOnSuccess;
            auto &callback = m_onSuccess;
            if (callback == nullptr) {
                throw NullPointerException();
            }
            callback(getSharedProgressCounter());
            m_taskStatus = TaskStatusEnum::Finished;
        } catch (...) {
            m_taskStatus = TaskStatusEnum::OnSuccessCrashed;
            logAndSendToUser(global::LogDestEnum::Warning, [this](AnsiOstream &aos) {
                formatShortDesc(aos);
                aos << "'s delayed callback threw an exception:\n";
                formatException(aos, std::current_exception());
            });
        }

        AsyncTasksWatcher::finished(getShared());
    }

private:
    void worker()
    {
        auto &callback = m_backgroundWorker;
        if (callback == nullptr) {
            throw NullPointerException();
        }
        callback(getSharedProgressCounter());
    }

public:
    void requestCancel() { getProgressCounter().requestCancel(); }
    NODISCARD bool tryComplete()
    {
        if (m_future.wait_for(std::chrono::nanoseconds{0}) == std::future_status::timeout) {
            return false;
        }

        complete();
        m_end = Clock::now();
        return true;
    }

    void onStarted()
    {
        m_taskStatus = TaskStatusEnum::Started;
        AsyncTasksWatcher::started(getShared());
    }

    void onRemoved()
    {
        m_isRemoved = true;
        AsyncTasksWatcher::removed(getShared());
    }

    void reportStatus(AnsiOstream &aos) const
    {
        if constexpr (IS_DEBUG_BUILD) {
            ABORT_IF_NOT_ON_MAIN_THREAD();
        }

        // REVISIT: The output of this may need to be throttled for long-running tasks.
        auto &pc = getProgressCounter();
        const auto status = pc.getStatus();
        const auto allowCancel = pc.allowCancel() == AllowCancelEnum::Allow;
        const auto isCanceled = pc.hasRequestedCancel();

        formatShortDesc(aos);
        aos << " with progress [";
        formatPercent(aos, green, yellow, status.percent());
        aos << "] ";
        aos << coloredString(status.msg.getStdStringViewUtf8());
        aos << " (";
        if (!allowCancel) {
            aos << ColoredValue{yellow, "non-cancelable"};
        } else {
            // REVISIT: You can't actually cancel it after it has finished.
            MAYBE_UNUSED const auto running = isRunningOnBackgroundThread();
            aos << ColoredValue{green, "cancelable"};
        }

        switch (m_taskStatus) {
            using enum TaskStatusEnum;
#define X_CASE(_x, _color) \
    case (_x): \
        aos << ", " << ColoredValue((_color), #_x); \
        break;
            X_CASE(Allocated, yellow)
            X_CASE(Started, green)
            X_CASE(Canceled, red)
            X_CASE(BackgroundCrashed, red)
            X_CASE(CompletedBackground, yellow)
            X_CASE(BegunOnSuccess, yellow)
            X_CASE(OnSuccessCrashed, red)
            X_CASE(Finished, green)
        default:
            aos << ", " << ColoredValue{red, "*error*"};
            break;
#undef X_CASE
        }
        aos << ")";

        if (m_isRemoved) {
            aos << " [" << ColoredValue{yellow, "removed"} << "]";
        }

        if (const auto dt = getElapsedTime(); dt >= std::chrono::milliseconds(500)) {
            aos << " [elapsed: ";
            formatElapsedSeconds(aos, dt);
            aos << "]";
        }

        aos << AnsiOstream::endl;
    }
};

AsyncTaskTypeEnum AsyncTaskHandle::getType() const
{
    return getTask().getType();
}

size_t AsyncTaskHandle::getId() const
{
    return getTask().getId();
}

const std::string &AsyncTaskHandle::getName() const
{
    return getTask().getName();
}

TaskNameIdType AsyncTaskHandle::getNameIdType() const
{
    return getTask().getNameIdType();
}

bool AsyncTaskHandle::isRunningOnBackgroundThread() const
{
    return getTask().isRunningOnBackgroundThread();
}

std::shared_ptr<ProgressCounter> AsyncTaskHandle::getSharedProgressCounter() const
{
    return getTask().getSharedProgressCounter();
}

ProgressCounter &AsyncTaskHandle::getProgressCounter() const
{
    auto shared = getSharedProgressCounter();
    return deref(shared);
}

ProgressCounter::Status AsyncTaskHandle::getStatus() const
{
    return getTask().getStatus();
}

Clock::time_point AsyncTaskHandle::getStartTime() const
{
    return getTask().getStartTime();
}

std::optional<Clock::time_point> AsyncTaskHandle::getEndTime() const
{
    return getTask().getEndTime();
}

NODISCARD Clock::duration AsyncTaskHandle::getElapsedTime() const
{
    return getTask().getElapsedTime();
}

TaskStatusEnum AsyncTaskHandle::getTaskStatus() const
{
    return getTask().getTaskStatus();
}

AllowCancelEnum AsyncTaskHandle::getCanCancel() const
{
    return getTask().getCanCancel();
}

bool AsyncTaskHandle::getIsRemoved() const
{
    return getTask().getIsRemoved();
}

void AsyncTaskHandle::reportStatus(AnsiOstream &aos) const
{
    getTask().reportStatus(aos);
}

std::string AsyncTaskHandle::getStatusAsPlaintext() const
{
    std::ostringstream oss;
    {
        // note: using AnsiSupportFlags{} here won't keep AnsiOstream from emitting ANSI.
        AnsiOstream aos{oss};
        reportStatus(aos);
    }

    auto str = strip_ansi(std::move(oss).str());
    using namespace char_consts;
    if (!str.empty() && str.ends_with(C_NEWLINE)) {
        str.resize(str.size() - 1);
        if (!str.empty() && str.ends_with(C_CARRIAGE_RETURN)) {
            str.resize(str.size() - 1);
        }
    }
    return str;
}

void AsyncTaskHandle::requestCancel() const
{
    return getTask().requestCancel();
}

bool AsyncTaskHandle::tryComplete(Badge<AsyncTasks>) const
{
    return getTask().tryComplete();
}

void AsyncTaskHandle::onRemoved(Badge<AsyncTasks>) const
{
    getTask().onRemoved();
}

class NODISCARD AsyncTasks final
{
private:
    using Clock = std::chrono::steady_clock;
    static constexpr auto g_timer_period = std::chrono::milliseconds(250);

private:
    std::list<AsyncTaskHandle> m_tasks;
    QTimer m_timer;
    std::optional<Clock::time_point> m_last_status_log_time;

public:
    explicit AsyncTasks()
    {
        QObject::connect(&m_timer, &QTimer::timeout, [this]() { onTimer(); });
        m_timer.setInterval(g_timer_period);
    }
    ~AsyncTasks()
    {
        m_timer.stop();
        cancelAll();
        m_last_status_log_time.reset();
        logStatus();
        waitForCompletions();
        m_last_status_log_time.reset();
        logStatus();
    }

private:
    void waitForCompletions()
    {
        MMLOG() << "Waiting for tasks to complete...";
        while (!m_tasks.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            runCompletionFunctions();
        }
        MMLOG() << "Done waiting for tasks to complete.";
    }

public:
    void reportStatus(AnsiOstream &aos) const
    {
        ABORT_IF_NOT_ON_MAIN_THREAD();

        aos << "Background Tasks:\n";
        if (const auto count = m_tasks.size(); count == 0) {
            aos << " (none)\n";
        } else {
            for (auto &task : m_tasks) {
                if (task.isRunningOnBackgroundThread()) {
                    task.reportStatus(aos);
                }
            }
            aos << "Total: " << count << " task" << ((count == 1) ? "" : "s") << " listed.\n";
        }
    }

    void reportStatus(AnsiOstream &aos, const size_t id) const
    {
        ABORT_IF_NOT_ON_MAIN_THREAD();

        bool found = false;
        for (auto &task : m_tasks) {
            if (task.isRunningOnBackgroundThread() && task.getId() == id) {
                found = true;
                task.reportStatus(aos);
            }
        }
        if (!found) {
            aos << "Error: Invalid task id " << ColoredValue{red, id} << ".\n";
        }
    }

    void cancelAll()
    {
        for (auto &task : m_tasks) {
            task.requestCancel();
        }
    }

    NODISCARD static bool try_cancel(AnsiOstream &aos, const AsyncTaskHandle &task)
    {
        if (task.getCanCancel() == AllowCancelEnum::Forbid) {
            formatTaskNameId(aos, task);
            aos << " cannot be canceled!\n";
        } else if (task.getProgressCounter().hasRequestedCancel()) {
            formatTaskNameId(aos, task);
            aos << " has already been canceled.\n";
        } else {
            aos << "Attempting to cancel ";
            formatTaskNameId(aos, task);
            aos << "...\n";
            task.requestCancel();
        }
        if (false) {
            task.reportStatus(aos);
        }
        return task.getProgressCounter().hasRequestedCancel();
    }

    NODISCARD bool cancel(AnsiOstream &aos, const size_t id)
    {
        ABORT_IF_NOT_ON_MAIN_THREAD();

        for (auto &task : m_tasks) {
            if (task.getId() == id) {
                return try_cancel(aos, task);
            }
        }

        aos << "Error: Invalid task id " << ColoredValue{red, id} << ".\n";
        return false;
    }

    void forEach(const std::function<void(const AsyncTaskHandle &)> &callback)
    {
        for (const auto &task : m_tasks) {
            callback(task);
        }
    }

    void forEachStatus(
        const std::function<void(size_t id, std::string_view name, const ProgressCounter::Status &)>
            &callback) const
    {
        for (const auto &task : m_tasks) {
            callback(task.getId(), task.getName(), task.getStatus());
        }
    }

private:
    void runCompletionFunctions()
    {
        bool completedAny = false;
        // ABORT_IF_NOT_ON_MAIN_THREAD();
        for (auto &task : m_tasks) {
            if (task.isRunningOnBackgroundThread() && task.tryComplete(Badge<AsyncTasks>{})) {
                completedAny = true;
                assert(!task.isRunningOnBackgroundThread());
            }
        }
        if (completedAny) {
            filterTasks();
        }
    }

    void logStatus()
    {
        // ABORT_IF_NOT_ON_MAIN_THREAD();
        global::logOnly(global::LogDestEnum::Info, [this](AnsiOstream &aos) { reportStatus(aos); });
    }

    void filterTasks()
    {
        // ABORT_IF_NOT_ON_MAIN_THREAD();
        std::ignore = utils::listRemoveIf(m_tasks, [](const AsyncTaskHandle &task) -> bool {
            if (task.isRunningOnBackgroundThread()) {
                return false;
            }
            task.onRemoved(Badge<AsyncTasks>{});
            return true;
        });
    }

private:
    void onTimer()
    {
        ABORT_IF_NOT_ON_MAIN_THREAD();

        filterTasks();
        runCompletionFunctions();

        if (m_tasks.empty()) {
            m_timer.stop();
        } else {
            const auto now = Clock::now();
            if (!m_last_status_log_time.has_value()
                || now > m_last_status_log_time.value() + std::chrono::seconds(5)) {
                m_last_status_log_time = now;
                logStatus();
            }
        }
    }

public:
    NODISCARD AsyncTaskHandle start(const AsyncTaskTypeEnum type,
                                    const AllowCancelEnum allowCancel,
                                    std::string name,
                                    BackgroundWorker2 backgroundWorker,
                                    OnSuccess2 onSuccess)
    {
        ABORT_IF_NOT_ON_MAIN_THREAD();
        auto task = AsyncTask::alloc(type,
                                     allowCancel,
                                     std::move(name),
                                     std::move(backgroundWorker),
                                     std::move(onSuccess));
        // This has to happen after the constructor.
        deref(task).onStarted();
        m_tasks.emplace_back(Badge<AsyncTasks>{}, task);

        if (!m_timer.isActive()) {
            m_timer.start();
        }
        m_last_status_log_time.reset();
        return AsyncTaskHandle{Badge<AsyncTasks>{}, std::move(task)};
    }
};

static std::unique_ptr<AsyncTasks> g_tasks;
static std::atomic_bool g_cleaning_up = false;

void init()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    if (g_tasks != nullptr || g_cleaning_up) {
        std::abort();
    }
    g_tasks = std::make_unique<AsyncTasks>();
}

void cleanup()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    MMLOG() << "Cleaning up async tasks.";
    g_cleaning_up = true;
    if (g_tasks != nullptr) {
        g_tasks.reset();
    }
}

AsyncTaskHandle startAsyncTask(const AsyncTaskTypeEnum type,
                               const AllowCancelEnum allowCancel,
                               std::string name,
                               BackgroundWorker backgroundWorker,
                               OnSuccess onSuccess)
{
    if (backgroundWorker == nullptr) {
        throw std::invalid_argument("backgroundWorker");
    }
    if (onSuccess == nullptr) {
        throw std::invalid_argument("onSuccess");
    }

    return startAsyncTask2(
        type,
        allowCancel,
        std::move(name),
        [callback = std::move(backgroundWorker)](const std::shared_ptr<ProgressCounter> &pc) {
            callback(deref(pc));
        },
        [callback = std::move(onSuccess)](const std::shared_ptr<ProgressCounter> & /*pc*/) {
            callback();
        });
}

AsyncTaskHandle startAsyncTask2(const AsyncTaskTypeEnum type,
                                const AllowCancelEnum allowCancel,
                                std::string name,
                                BackgroundWorker2 backgroundWorker,
                                OnSuccess2 onSuccess)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    if (g_cleaning_up) {
        throw std::runtime_error("mapper is shutting down");
    }
    if (backgroundWorker == nullptr) {
        throw std::invalid_argument("backgroundWorker");
    }
    if (onSuccess == nullptr) {
        throw std::invalid_argument("onSuccess");
    }

    return deref(g_tasks).start(type,
                                allowCancel,
                                std::move(name),
                                std::move(backgroundWorker),
                                std::move(onSuccess));
}

void report_status(AnsiOstream &aos)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    deref(g_tasks).reportStatus(aos);
}

void report_status(AnsiOstream &aos, size_t id)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    deref(g_tasks).reportStatus(aos, id);
}

void cancel_all()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    deref(g_tasks).cancelAll();
}

bool cancel(AnsiOstream &aos, size_t id)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    return deref(g_tasks).cancel(aos, id);
}

void for_each(const std::function<void(const AsyncTaskHandle &)> &callback)
{
    deref(g_tasks).forEach(callback);
}

void for_each_status(
    const std::function<void(size_t id, std::string_view name, const ProgressCounter::Status &)>
        &callback)
{
    deref(g_tasks).forEachStatus(callback);
}

} // namespace async_tasks
