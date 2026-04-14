#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"

#include <QDialog>

class QLabel;
class QSpacerItem;

namespace Ui {
class ConfigDialog;
}

class QListWidgetItem;
class QStackedWidget;

class QTimer;

class ConfigDialog final : public QDialog
{
    Q_OBJECT

private:
    struct PageInfo
    {
        QString name;
        QWidget *widget;
        QListWidgetItem *item;
        QWidget *container; // wraps section header + page widget
    };

    Ui::ConfigDialog *ui;
    QList<PageInfo> m_pages;
    QTimer *m_searchTimer = nullptr;

public:
    explicit ConfigDialog(QWidget *parent = nullptr);
    ~ConfigDialog() override;

private:
    void scrollToWidget(QWidget *target, bool focus = false);

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void slot_changePage(QListWidgetItem *current, QListWidgetItem * /*previous*/);
    void slot_onScroll(int value);
    void slot_ok();
    void slot_cancel();
    void slot_search(const QString &text);
    void slot_onResultSelected(QListWidgetItem *item);

signals:
    void sig_graphicsSettingsChanged();
    void sig_groupSettingsChanged();
    void sig_loadConfig();
};
