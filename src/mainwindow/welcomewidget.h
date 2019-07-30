#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QString>
#include <QWidget>
#include <QtCore>

class QObject;

namespace Ui {
class WelcomeWidget;
}

class WelcomeWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomeWidget(QWidget *parent = nullptr);
    ~WelcomeWidget() override;

public slots:
    void onPlayButtonClicked(bool);

signals:
    void playMumeClicked();

private:
    Ui::WelcomeWidget *ui = nullptr;
};
