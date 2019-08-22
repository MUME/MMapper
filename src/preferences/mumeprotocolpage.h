#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QString>
#include <QWidget>
#include <QtCore>

class QObject;

namespace Ui {
class MumeProtocolPage;
}

class MumeProtocolPage : public QWidget
{
    Q_OBJECT

public:
    explicit MumeProtocolPage(QWidget *parent = nullptr);
    ~MumeProtocolPage();

public slots:
    void remoteEditCheckBoxStateChanged(int);
    void internalEditorRadioButtonChanged(bool);
    void externalEditorCommandTextChanged(QString);

private:
    Ui::MumeProtocolPage *ui = nullptr;
};
