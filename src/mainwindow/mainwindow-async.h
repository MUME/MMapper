#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/AsyncTasks.h"
#include "../global/macros.h"
#include "mainwindow.h"
#include "utils.h"

#include <memory>

struct NODISCARD MainWindow::ActionDisabler final
{
private:
    MainWindow &m_self;

public:
    explicit ActionDisabler(MainWindow &self)
        : m_self(self)
    {
        self.disableActions(true);
    }
    ~ActionDisabler() { m_self.disableActions(false); }

public:
    DELETE_CTORS_AND_ASSIGN_OPS(ActionDisabler);
};

struct NODISCARD ProgressData final
{
    QTimer timer;
    ProgressMsg lastMsg;
    size_t lastPoll = 0;

    ProgressData() = default;
    ~ProgressData() = default;
    ProgressData(const ProgressData &) = delete;
    ProgressData &operator=(const ProgressData &) = delete;
};

struct NODISCARD MainWindow::AsyncIO final : public QObject
{
private:
    struct NODISCARD AsyncData final
    {
        std::shared_ptr<AsyncBase> asyncIo;
        async_tasks::AsyncTaskHandle asyncTask;
    };
    struct NODISCARD ExtraBlockers final
    {
        ActionDisabler actionDisabler;
        // REVISIT: make this optional, so it's not done during map saving.
        CanvasHider canvasHider;
        MapFrontendBlocker blocker;

        explicit ExtraBlockers(MainWindow &mw, MapFrontend &md)
            : actionDisabler{mw}
            , canvasHider{mw}
            , blocker{md}
        {}
    };

private:
    MainWindow &m_mainWindow;
    std::optional<AsyncData> m_asyncData;
    std::optional<CanvasDisabler> m_canvasDisabler;
    std::unique_ptr<QProgressDialog> m_progressDlg;
    std::optional<ExtraBlockers> m_extraBlockers;
    std::optional<ProgressData> m_progressData;
    std::atomic_bool m_waitingForSaveAtShutdown = false;
    std::atomic_bool m_closedForBusiness = false;

public:
    explicit AsyncIO(MainWindow *mainWindow);
    ~AsyncIO() final;

private:
    NODISCARD MainWindow &getMainWindow() const { return m_mainWindow; }
    NODISCARD bool hasCurrentTask() const { return m_asyncData.has_value(); }
    NODISCARD const AsyncData &getData() const { return deref(m_asyncData); }
    NODISCARD AsyncBase &getHelper() const { return deref(getData().asyncIo); }
    NODISCARD const async_tasks::AsyncTaskHandle &getTask() const { return getData().asyncTask; }

public:
    NODISCARD bool isRunningOnBackgroundThread() const;
    NODISCARD size_t getTaskId() const;
    NODISCARD const std::string &getTaskName() const;
    NODISCARD QString getTaskNameAsQString() const;
    NODISCARD std::shared_ptr<ProgressCounter> getSharedProgressCounter() const;
    NODISCARD ProgressCounter &getProgressCounter() const;
    NODISCARD QProgressDialog *getProgressDlg() const { return m_progressDlg.get(); }

public:
    NODISCARD bool isClosedForBusiness() const { return m_closedForBusiness; }
    void setClosedForBusiness() { m_closedForBusiness = true; }

public:
    void beginAsyncIO(std::unique_ptr<AsyncBase> task,
                      const QString &progressDialogText,
                      AsyncIOTypeEnum type);
    void tick();
    void request_cancel();
    NODISCARD bool is_allowed_to_cancel() const;
    NODISCARD bool isWaitingForSaveAtShutdown() const { return m_waitingForSaveAtShutdown; }
    void setWaitingForSaveAtShutdown() { m_waitingForSaveAtShutdown = true; }

    void percentageChanged(const uint32_t p)
    {
        if (m_progressDlg == nullptr) {
            return;
        }
        m_progressDlg->setValue(static_cast<int>(p));
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    void resetExtraBlockers();

private:
    void reset();
    void updateStatus();
    void createNewProgressDialog(const QString &text, bool allow_cancel);
    void endProgressDialog() { m_progressDlg.reset(); }
    friend QDebug &operator<<(QDebug &debug, const AsyncIO &task);
};
