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

class ClientPage : public QWidget
{
    Q_OBJECT

public:
    explicit ClientPage(QWidget *parent = nullptr);
    ~ClientPage() override;

    void updateFontAndColors();

public slots:
    void onChangeFont();
    void onChangeBackgroundColor();
    void onChangeForegroundColor();
    void onChangeColumns(int);
    void onChangeRows(int);
    void onChangeLinesOfScrollback(int);
    void onChangeLinesOfInputHistory(int);
    void onChangeTabCompletionDictionarySize(int);

signals:

private:
    Ui::ClientPage *ui = nullptr;
};
