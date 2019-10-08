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

class GroupManagerPage : public QWidget
{
    Q_OBJECT
public:
    explicit GroupManagerPage(Mmapper2Group *, QWidget *parent = nullptr);
    ~GroupManagerPage() override;

public slots:
    void loadConfig();

    void changeColorClicked();
    void charNameTextChanged();

    void allowedSecretsChanged();

    void remoteHostTextChanged();
    void localPortValueChanged(int);

    void rulesWarningChanged(int);
    void shareSelfChanged(int);

signals:
    void updatedSelf();
    void refresh();

private:
    Mmapper2Group *m_groupManager = nullptr;
    Ui::GroupManagerPage *ui = nullptr;
};
