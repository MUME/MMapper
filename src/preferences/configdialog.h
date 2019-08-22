#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QDialog>
#include <QString>
#include <QtCore>

class Mmapper2Group;
class QListWidgetItem;
class QObject;
class QStackedWidget;
class QWidget;

namespace Ui {
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(Mmapper2Group *, QWidget *parent = nullptr);
    ~ConfigDialog();

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);

private:
    Ui::ConfigDialog *ui = nullptr;

    void createIcons();

    QStackedWidget *pagesWidget = nullptr;

    Mmapper2Group *m_groupManager = nullptr;
};
