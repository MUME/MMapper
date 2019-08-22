#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QString>
#include <QWidget>
#include <QtCore>

namespace Ui {
class GeneralPage;
}

class QObject;

class GeneralPage : public QWidget
{
    Q_OBJECT

public slots:

    void remoteNameTextChanged(const QString &);
    void remotePortValueChanged(int);
    void localPortValueChanged(int);
    void tlsEncryptionCheckBoxStateChanged(int);

    void emulatedExitsStateChanged(int);
    void showHiddenExitFlagsStateChanged(int);
    void showNotesStateChanged(int);

    void autoLoadFileNameTextChanged(const QString &);
    void autoLoadCheckStateChanged(int);

    void selectWorldFileButtonClicked();

    void displayMumeClockStateChanged(int);

public:
    explicit GeneralPage(QWidget *parent = nullptr);
    ~GeneralPage();

private:
    Ui::GeneralPage *ui = nullptr;
};
