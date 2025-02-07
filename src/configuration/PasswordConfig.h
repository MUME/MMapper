#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/RuleOf5.h"

#include <QObject>
#include <QString>

#ifndef MMAPPER_NO_QTKEYCHAIN
#include <keychain.h>
#endif

class PasswordConfig final : public QObject
{
    Q_OBJECT

private:
#ifndef MMAPPER_NO_QTKEYCHAIN
    QKeychain::ReadPasswordJob m_readJob;
    QKeychain::WritePasswordJob m_writeJob;
#endif

public:
    explicit PasswordConfig(QObject *parent = nullptr);
    ~PasswordConfig() final = default;
    DELETE_CTORS_AND_ASSIGN_OPS(PasswordConfig);

    void setPassword(const QString &password);
    void getPassword();

signals:
    void sig_error(const QString &msg);
    void sig_incomingPassword(const QString &password);
};
