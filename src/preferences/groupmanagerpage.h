#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../pandoragroup/mmapper2group.h"
#include "ui_groupmanagerpage.h"

#include <QString>
#include <QWidget>
#include <QtCore>

class Mmapper2Group;
class QObject;

namespace Ui {
class GroupManagerPage;
}

class NODISCARD_QOBJECT GroupManagerPage final : public QWidget
{
    Q_OBJECT

private:
    Mmapper2Group *const m_groupManager;
    Ui::GroupManagerPage *const ui;

public:
    explicit GroupManagerPage(Mmapper2Group *, QWidget *parent);
    ~GroupManagerPage() final;

private:
    void loadRemoteHostConfig();

signals:
    void sig_updatedSelf();
    void sig_refresh();

public slots:
    void slot_loadConfig();
    void slot_changeColorClicked();
    void slot_charNameTextChanged();
    void slot_allowedSecretsChanged();
    void slot_remoteHostTextChanged();
    void slot_localPortValueChanged(int);
    void slot_rulesWarningChanged(int);
    void slot_shareSelfChanged(int);
};
