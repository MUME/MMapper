#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Mattias 'Mew_' Viklund <devmew@exedump.com> (Mirnir)

#include <QWidget>
#include <QtCore>

namespace Ui {
class AutoLogPage;
}

class AutoLogPage : public QWidget
{
    Q_OBJECT

public:
    explicit AutoLogPage(QWidget *parent = nullptr);
    ~AutoLogPage() override;

public slots:
    void loadConfig();

    void logStrategyChanged(int);

    void selectLogLocationButtonClicked(int);

signals:

private:
    Ui::AutoLogPage *ui = nullptr;
};
