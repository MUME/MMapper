#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QString>
#include <QWidget>
#include <QtCore>

class QObject;

namespace Ui {
class ClientPage;
}

class ClientPage final : public QWidget
{
    Q_OBJECT

private:
    Ui::ClientPage *const ui;

public:
    explicit ClientPage(QWidget *parent);
    ~ClientPage() final;

    void updateFontAndColors();

public slots:
    void slot_loadConfig();
    void slot_onChangeFont();
    void slot_onChangeBackgroundColor();
    void slot_onChangeForegroundColor();
    void slot_onChangeColumns(int);
    void slot_onChangeRows(int);
    void slot_onChangeLinesOfScrollback(int);
    void slot_onChangeLinesOfInputHistory(int);
    void slot_onChangeTabCompletionDictionarySize(int);
};
