// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "SendToUser.h"

#include "Signal2.h"
#include "thread_utils.h"

#include <QString>

namespace global {

static auto &getSendToUser()
{
    static Signal2<QString> sig2_sendToUser;
    return sig2_sendToUser;
}

void registerSendToUser(Signal2Lifetime &lifetime, Signal2<QString>::Function callback)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    auto &sig2 = getSendToUser();
    sig2.connect(lifetime, std::move(callback));
}

void sendToUser(const QString &str)
{
    using namespace mmqt;
    ABORT_IF_NOT_ON_MAIN_THREAD();
    auto &fn = getSendToUser();
    if (str.endsWith(QC_NEWLINE)) {
        fn.invoke(str);
    } else {
        qWarning() << "sendToUser() missing a newline: " << str;
        fn.invoke(str + QS_NEWLINE);
    }
}

namespace {

struct NODISCARD LogData final
{
    mm::source_location loc;
    LogDestEnum dest = LogDestEnum::Info;
};

void send_to_log(const LogData log, const std::string_view msg)
{
    switch (log.dest) {
    case LogDestEnum::Debug:
        mm::DebugOstream{log.loc} << msg;
        break;
    case LogDestEnum::Warning:
        mm::WarningOstream{log.loc} << msg;
        break;
    default:
    case LogDestEnum::Info:
        mm::InfoOstream{log.loc} << msg;
        break;
    }
}

void internal_format(std::ostringstream &oss, const std::function<void(AnsiOstream &)> &f)
{
    AnsiOstream aos{oss};
    try {
        f(aos);
    } catch (...) {
        formatException(aos, std::current_exception(), "while formatting message");
    }
}

void internal_format_and_send(const std::optional<LogData> opt_log,
                              const std::function<void(AnsiOstream &)> &f)
{
    std::ostringstream oss;
    internal_format(oss, f);

    const auto s = std::move(oss).str();
    if (s.empty()) {
        qWarning() << "sendToUser(callback) didn't send anything.";
        return;
    }

    if (opt_log) {
        send_to_log(*opt_log, s);
    }

    const auto qs = mmqt::toQStringUtf8(s);
    sendToUser(qs);
}
} // namespace

void sendToUser(const std::function<void(AnsiOstream &)> &f)
{
    internal_format_and_send(std::nullopt, f);
}

void logAndSendToUser(const LogDestEnum dest,
                      const std::function<void(AnsiOstream &)> &f,
                      const mm::source_location loc)
{
    internal_format_and_send(LogData{loc, dest}, f);
}

void logOnly(const LogDestEnum dest,
             const std::function<void(AnsiOstream &)> &f,
             const mm::source_location loc)
{
    std::ostringstream oss;
    {
        AnsiOstream aos{oss};
        try {
            f(aos);
        } catch (...) {
            logException(LogDestEnum::Warning, std::current_exception(), loc);
            return;
        }
    }
    const auto s = std::move(oss).str();
    if (!s.empty()) {
        send_to_log(LogData{loc, dest}, s);
    }
}

void logException(const LogDestEnum dest,
                  const std::exception_ptr &ex,
                  const mm::source_location loc)
{
    std::ostringstream oss;
    {
        AnsiOstream aos{oss};
        formatException(aos, ex);
    }
    send_to_log(LogData{loc, dest}, oss.str());
}

void sendExceptionToUser(const std::exception_ptr &ptr, const std::string_view when)
{
    sendToUser([ptr, when](AnsiOstream &aos) { formatException(aos, ptr, when); });
}

} // namespace global
