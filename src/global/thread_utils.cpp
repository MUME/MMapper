// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "thread_utils.h"

#if 0
#include "../global/Version.h"
#endif

#include <iostream>

#include <QObject>
#include <QThread>
#include <QtCore>

namespace thread_utils {

NODISCARD bool isOnMainThread()
{
    if (QCoreApplication *const inst = QCoreApplication::instance()) {
        return QThread::currentThread() == inst->thread();
    } else {
        return false;
    }
}

void abortIfNotOnMainThread(const mm::source_location loc)
{
    const bool on_main_thread = isOnMainThread();
    if (on_main_thread) {
        return;
    }

    const auto message = "Unexpected use of background thread.";

    // QFatal won't always report the source location,
    // so let's make sure the file and line number are reported.
    std::cout << std::flush;
    std::cerr << "\n\n"
              << "### FATAL ERROR"
#if 0
              << " in mmapper version \"" << getMMapperVersion() << "\""
#endif
              << " at " << loc.file_name() << ":" << loc.line() << " in function \""
              << loc.function_name() << "\": \"" << message << "\"\n\n"
              << "The application will now terminate.\n\n";

    // This should be a proper replacment of qFatal(message) but with the correct source location,
    // so this should not return.
    QMessageLogger(loc.file_name(), static_cast<int>(loc.line()), loc.function_name()).fatal(message);

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#endif
    // For good measure, abort just in case qFatal() somehow returns.
    // NOLINTNEXTLINE (yes, we know QFatal is NORETURN)
    std::abort();
#ifdef __clang__
#pragma clang diagnostic pop
#endif
}
} // namespace thread_utils
