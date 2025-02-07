// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "PasswordConfig.h"

static const QLatin1String PASSWORD_KEY("password");
static const QLatin1String APP_NAME("org.mume.mmapper");

PasswordConfig::PasswordConfig(QObject *const parent)
    : QObject(parent)
#ifndef MMAPPER_NO_QTKEYCHAIN
    , m_readJob(APP_NAME)
    , m_writeJob(APP_NAME)
{
    m_readJob.setAutoDelete(false);
    m_writeJob.setAutoDelete(false);

    connect(&m_readJob, &QKeychain::ReadPasswordJob::finished, [this]() {
        if (m_readJob.error()) {
            emit sig_error(m_readJob.errorString());
        } else {
            emit sig_incomingPassword(m_readJob.textData());
        }
    });

    connect(&m_writeJob, &QKeychain::WritePasswordJob::finished, [this]() {
        if (m_writeJob.error()) {
            emit sig_error(m_writeJob.errorString());
        }
    });
}
#else
{}
#endif

void PasswordConfig::setPassword(const QString &password)
{
#ifndef MMAPPER_NO_QTKEYCHAIN
    m_writeJob.setKey(PASSWORD_KEY);
    m_writeJob.setTextData(password);
    m_writeJob.start();
#else
    emit sig_error("Password setting is not available.");
#endif
}

void PasswordConfig::getPassword()
{
#ifndef MMAPPER_NO_QTKEYCHAIN
    m_readJob.setKey(PASSWORD_KEY);
    m_readJob.start();
#else
    emit sig_error("Password retrieval is not available.");
#endif
}
