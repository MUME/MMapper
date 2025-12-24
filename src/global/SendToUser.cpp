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
} // namespace global
