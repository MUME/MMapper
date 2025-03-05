#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"

#include <QString>
#include <QWidget>
#include <QtCore>

namespace Ui {
class GeneralPage;
}

class QObject;

class NODISCARD_QOBJECT GeneralPage final : public QWidget
{
    Q_OBJECT

private:
    Ui::GeneralPage *const ui;

public:
    explicit GeneralPage(QWidget *parent);
    ~GeneralPage() final;

signals:
    void sig_factoryReset();

public slots:
    void slot_loadConfig();
    void slot_remoteNameTextChanged(const QString &);
    void slot_remotePortValueChanged(int);
    void slot_localPortValueChanged(int);

    void slot_emulatedExitsStateChanged(int);
    void slot_showHiddenExitFlagsStateChanged(int);
    void slot_showNotesStateChanged(int);

    void slot_autoLoadFileNameTextChanged(const QString &);
    void slot_autoLoadCheckStateChanged(int);
    void slot_selectWorldFileButtonClicked(bool);

    void slot_displayMumeClockStateChanged(int);
    void slot_displayXPStatusStateChanged(int);
};
