#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/AnsiOstream.h"
#include "../global/AsyncTasks.h"
#include "../global/SendToUser.h"
#include "../global/thread_utils.h"
#include "AnsiViewWindow.h"
#include "TopLevelWindows.h"

enum class NODISCARD AsyncWorkerSendResultEnum : uint8_t { ToViewer, ToUser };

template<typename Args>
void launchAsyncAnsiViewerWorker(
    const std::string &task_name,
    const QString &title,
    Args args,
    std::function<void(ProgressCounter &pc, AnsiOstream &, Args &)> worker,
    const AsyncWorkerSendResultEnum sendResultTo = AsyncWorkerSendResultEnum::ToViewer)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();

    // REVISIT: use white on cyan color scheme here?
    static constexpr auto green = getRawAnsi(AnsiColor16Enum::green);
    static constexpr auto yellow = getRawAnsi(AnsiColor16Enum::yellow);
    // static constexpr auto whiteOnCyan = getRawAnsi(AnsiColor16Enum::white, AnsiColor16Enum::cyan);

    static constexpr auto defaultColor = RawAnsi{}; // whiteOnCyan
    static constexpr auto val = green;              // whiteOnCyan.withBold();
    static constexpr auto str = green;              // val;
    static constexpr auto esc = yellow;             // whiteOnCyan;

    using Worker = decltype(worker);

    // We'll cheat by passing the result in a type-erased fashion
    // so that the worker function returns void,
    // and the completion function receives no arguments.
    class NODISCARD Data final
    {
    private:
        std::string m_task_name;
        QString m_title;
        Args m_args;
        Worker m_worker;
        std::string m_resultAnsiString;
        AsyncWorkerSendResultEnum m_sendResultTo = AsyncWorkerSendResultEnum::ToViewer;

    public:
        explicit Data(std::string moved_task_name,
                      QString moved_title,
                      Args args,
                      Worker worker,
                      const AsyncWorkerSendResultEnum sendResultTo_)
            : m_task_name{std::move(moved_task_name)}
            , m_title{std::move(moved_title)}
            , m_args{std::move(args)}
            , m_worker{std::move(worker)}
            , m_sendResultTo{sendResultTo_}
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

            switch (m_sendResultTo) {
            case AsyncWorkerSendResultEnum::ToUser:
                // Note: We don't know the task number here.
                global::sendToUser([this](AnsiOstream &aos) {
                    const auto before = aos.getNextAnsi();
                    aos << defaultColor << "Completed async task "
                        << ColoredQuotedStringView{str, esc, m_task_name};
                    if (m_resultAnsiString.empty()) {
                        aos << " with no output.\n";
                    } else {
                        aos << ":\n\n";
                        aos << before;
                        aos.writeWithEmbeddedAnsi(m_resultAnsiString);
                    }
                });
                break;

            default:
            case AsyncWorkerSendResultEnum::ToViewer: {
                auto w = makeAnsiViewWindow("MMapper Ansi Viewer", m_title, m_resultAnsiString);
                addTopLevelWindow(std::move(w));

                global::sendToUser([this](AnsiOstream &aos) {
                    const auto t = mmqt::toStdStringUtf8(m_title);
                    aos << defaultColor << "Launched " << ColoredQuotedStringView{str, esc, t}
                        << ".\n";
                });
            }
            }
        }
    };

    auto sharedData = std::make_shared<Data>(task_name,
                                             title,
                                             std::move(args),
                                             std::move(worker),
                                             sendResultTo);

    const async_tasks::AsyncTaskHandle taskHandle = async_tasks::startAsyncTask(
        AsyncTaskTypeEnum::Task,
        AllowCancelEnum::Allow,
        task_name,
        [sharedData](ProgressCounter &pc) { deref(sharedData).backgroundWorker(pc); },
        [sharedData]() { deref(sharedData).onSuccess(); });

    global::sendToUser([&title, sendResultTo, taskHandle](AnsiOstream &aos) {
        const auto t = mmqt::toStdStringUtf8(title);

        aos << defaultColor << "Started task #" << ColoredValue{val, taskHandle.getId()} << " ("
            << ColoredQuotedStringView{str, esc, taskHandle.getName()} << ").";
        switch (sendResultTo) {
        case AsyncWorkerSendResultEnum::ToUser:
            aos << " The output will be sent to your terminal...";
            break;
        default:
        case AsyncWorkerSendResultEnum::ToViewer:
            aos << " A new window called " << ColoredQuotedStringView{str, esc, t}
                << "will open with the result...";
            break;
        }
        aos << "\n";
    });
}
