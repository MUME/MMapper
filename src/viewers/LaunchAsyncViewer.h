#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/AnsiOstream.h"
#include "../global/AsyncTasks.h"
#include "../global/SendToUser.h"
#include "../global/thread_utils.h"
#include "AnsiViewWindow.h"
#include "TopLevelWindows.h"

template<typename Args>
void launchAsyncAnsiViewerWorker(
    const std::string &task_name,
    const QString &title,
    Args args,
    std::function<void(ProgressCounter &pc, AnsiOstream &, Args &)> worker)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();

    global::sendToUser(
        QString("Computing %1 asynchronously. A window will open with the result...\n").arg(title));

    using Worker = decltype(worker);

    // We'll cheat by passing the result in a type-erased fashion
    // so that the worker function returns void,
    // and the completion function receives no arguments.
    class NODISCARD Data final
    {
    private:
        QString m_title;
        Args m_args;
        Worker m_worker;
        std::string m_resultAnsiString;

    public:
        explicit Data(QString title, Args args, Worker worker)
            : m_title{std::move(title)}
            , m_args{std::move(args)}
            , m_worker{std::move(worker)}
        {}

        void backgroundWorker(ProgressCounter &pc)
        {
            /* on background thread */
            std::ostringstream os;
            {
                AnsiOstream aos{os};
                m_worker(pc, aos, m_args);
            }
            m_resultAnsiString = std::move(os).str();
        }
        void onSuccess()
        {
            ABORT_IF_NOT_ON_MAIN_THREAD();

            /* on main thread, after background completes successfully */
            auto w = makeAnsiViewWindow("MMapper Ansi Viewer", m_title, m_resultAnsiString);
            addTopLevelWindow(std::move(w));
            global::sendToUser(QString("Launched %1 window.\n").arg(m_title));
        }
    };

    auto sharedData = std::make_shared<Data>(title, std::move(args), std::move(worker));

    // REVISIT: report failure?
    async_tasks::startAsyncTask(
        task_name,
        [sharedData](ProgressCounter &pc) { deref(sharedData).backgroundWorker(pc); },
        [sharedData]() { deref(sharedData).onSuccess(); });
}
