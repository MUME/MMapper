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
class QShowEvent;
class QStackedWidget;
class QWidget;

namespace Ui {
class ConfigDialog;
}

class ConfigDialog final : public QDialog
{
    Q_OBJECT

private:
    Ui::ConfigDialog *const ui;
    Mmapper2Group *const m_groupManager;
    QStackedWidget *pagesWidget = nullptr;

public:
    explicit ConfigDialog(Mmapper2Group *, QWidget *parent);
    ~ConfigDialog() final;

protected:
    void showEvent(QShowEvent *event) override;

signals:
    void sig_graphicsSettingsChanged();
    void sig_loadConfig();

public slots:
    void slot_changePage(QListWidgetItem *current, QListWidgetItem *previous);

private:
    void createIcons();
};
