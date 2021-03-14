#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QString>
#include <QWidget>
#include <QtCore>

#include "../pandoragroup/mmapper2group.h"
#include "ui_groupmanagerpage.h"

class Mmapper2Group;
class QObject;

namespace Ui {
class GroupManagerPage;
}

class GroupManagerPage final : public QWidget
{
    Q_OBJECT
public:
    explicit GroupManagerPage(Mmapper2Group *, QWidget *parent = nullptr);
    ~GroupManagerPage() final;

public slots:
    void slot_loadConfig();
    void slot_changeColorClicked();
    void slot_charNameTextChanged();
    void slot_allowedSecretsChanged();
    void slot_remoteHostTextChanged();
    void slot_localPortValueChanged(int);
    void slot_rulesWarningChanged(int);
    void slot_shareSelfChanged(int);

signals:
    void sig_updatedSelf();
    void sig_refresh();

private:
    void loadRemoteHostConfig();

private:
    Mmapper2Group *m_groupManager = nullptr;
    Ui::GroupManagerPage *ui = nullptr;
};
