// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "PasswordConfig.h"

#include "../global/ConfigConsts-Computed.h"
#include "../global/ConfigEnums.h"
#include "configuration.h"

#ifndef MMAPPER_NO_QTKEYCHAIN
static const QLatin1String PASSWORD_KEY("password");
static const QLatin1String APP_NAME("org.mume.mmapper");
#endif

PasswordConfig::PasswordConfig(QObject *const parent)
    : QObject(parent)
#ifndef MMAPPER_NO_QTKEYCHAIN
    , m_readJob(APP_NAME)
    , m_writeJob(APP_NAME)
    , m_deleteJob(APP_NAME)
{
    m_readJob.setAutoDelete(false);
    m_writeJob.setAutoDelete(false);
    m_deleteJob.setAutoDelete(false);

    auto handleError = [this](const QKeychain::Job &job) {
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
            if (job.error() == QKeychain::AccessDeniedByUser) {
                return true;
            }
        }
        emit sig_error(job.errorString());
        return false;
    };

    connect(&m_readJob, &QKeychain::ReadPasswordJob::finished, [this, handleError]() {
        if (m_readJob.error()) {
            handleError(m_readJob);
        } else {
            emit sig_incomingPassword(m_readJob.textData());
        }
    });

    connect(&m_writeJob, &QKeychain::WritePasswordJob::finished, [this, handleError]() {
        if (m_writeJob.error()) {
            handleError(m_writeJob);
        } else {
            emit sig_passwordSaved();
        }
    });

    connect(&m_deleteJob, &QKeychain::DeletePasswordJob::finished, [this, handleError]() {
        if (m_deleteJob.error()) {
            handleError(m_deleteJob);
        } else {
            emit sig_passwordDeleted();
        }
    });
}
#else
{
}
#endif

void PasswordConfig::setPassword(const QString &password)
{
#ifndef MMAPPER_NO_QTKEYCHAIN
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        m_writeJob.setKey(getConfig().account.accountName);
    } else {
        m_writeJob.setKey(PASSWORD_KEY);
    }
    m_writeJob.setTextData(password);
    m_writeJob.start();
#else
    std::ignore = password;
    emit sig_error("Password setting is not available.");
#endif
}

void PasswordConfig::getPassword()
{
#ifndef MMAPPER_NO_QTKEYCHAIN
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        m_readJob.setKey(getConfig().account.accountName);
    } else {
        m_readJob.setKey(PASSWORD_KEY);
    }
    m_readJob.start();
#else
    emit sig_error("Password retrieval is not available.");
#endif
}

void PasswordConfig::deletePassword()
{
#ifndef MMAPPER_NO_QTKEYCHAIN
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        m_deleteJob.setKey(getConfig().account.accountName);
    } else {
        m_deleteJob.setKey(PASSWORD_KEY);
    }
    m_deleteJob.start();
#else
    emit sig_error("Password deletion is not available.");
#endif
}
