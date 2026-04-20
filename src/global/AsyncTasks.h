#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "Badge.h"
#include "progresscounter.h"

#include <functional>
#include <string>

class AnsiOstream;

enum class NODISCARD AsyncTaskTypeEnum : uint8_t { IO, Task };

namespace async_tasks {

enum class NODISCARD TaskStatusEnum : uint8_t {
    Allocated,
    Started,
    Canceled,
    BackgroundCrashed,
    CompletedBackground,
    BegunOnSuccess,
    OnSuccessCrashed,
    Finished,
};
class AsyncTask;
class AsyncTasks;
using Clock = std::chrono::steady_clock;
using BackgroundWorker = std::function<void(ProgressCounter &pc)>;
using OnSuccess = std::function<void()>;

using BackgroundWorker2 = std::function<void(const std::shared_ptr<ProgressCounter> &pc)>;
using OnSuccess2 = std::function<void(const std::shared_ptr<ProgressCounter> &pc)>;

/* Caution: This only contains a view of the name. */
struct NODISCARD TaskNameIdType final
{
    std::string_view name;
    size_t id = 0;
    AsyncTaskTypeEnum type = AsyncTaskTypeEnum::Task;
};

class NODISCARD AsyncTaskHandle final
{
private:
    std::shared_ptr<AsyncTask> m_task;

public:
    AsyncTaskHandle() = delete;
    template<typename T>
        requires(std::is_same_v<T, AsyncTask> or std::is_same_v<T, AsyncTasks>)
    explicit AsyncTaskHandle(Badge<T>, std::shared_ptr<AsyncTask> task)
        : m_task{std::move(task)}
    {
        if (m_task == nullptr) {
            throw std::invalid_argument("task");
        }
    }

private:
    NODISCARD AsyncTask &getTask() const { return deref(m_task); }

public:
    NODISCARD AsyncTaskTypeEnum getType() const;
    NODISCARD size_t getId() const;
    NODISCARD const std::string &getName() const;
    NODISCARD TaskNameIdType getNameIdType() const;
    NODISCARD bool isRunningOnBackgroundThread() const;
    NODISCARD std::shared_ptr<ProgressCounter> getSharedProgressCounter() const;
    NODISCARD ProgressCounter &getProgressCounter() const;
    NODISCARD ProgressCounter::Status getStatus() const;

    void reportStatus(AnsiOstream &aos) const;
    NODISCARD std::string getStatusAsPlaintext() const;

    NODISCARD Clock::time_point getStartTime() const;
    NODISCARD std::optional<Clock::time_point> getEndTime() const;
    NODISCARD Clock::duration getElapsedTime() const;

    NODISCARD TaskStatusEnum getTaskStatus() const;
    NODISCARD AllowCancelEnum getCanCancel() const;
    NODISCARD bool getIsRemoved() const;

public:
    void requestCancel() const;
    NODISCARD bool tryComplete(Badge<AsyncTasks>) const;
    void onRemoved(Badge<AsyncTasks>) const;

public:
    NODISCARD bool operator==(const AsyncTaskHandle &other) const { return m_task == other.m_task; }
};

extern void init();
extern void cleanup();
NODISCARD extern AsyncTaskHandle startAsyncTask(AsyncTaskTypeEnum type,
                                                AllowCancelEnum allowCancel,
                                                std::string name,
                                                BackgroundWorker backgroundWorker,
                                                OnSuccess onSuccess);

NODISCARD extern AsyncTaskHandle startAsyncTask2(AsyncTaskTypeEnum type,
                                                 AllowCancelEnum allowCancel,
                                                 std::string name,
                                                 BackgroundWorker2 backgroundWorker,
                                                 OnSuccess2 onSuccess);

extern void report_status(AnsiOstream &aos);
extern void report_status(AnsiOstream &aos, size_t id);
extern void cancel_all();
NODISCARD extern bool cancel(AnsiOstream &aos, size_t id);
extern void for_each(const std::function<void(const AsyncTaskHandle &)> &);
extern void for_each_status(
    const std::function<void(size_t id, std::string_view name, const ProgressCounter::Status &)> &);

void formatElapsedSeconds(AnsiOstream &aos, const Clock::duration dt);

} // namespace async_tasks
