#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Kalev Lember <kalev@smartlink.ee> (Kalev)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QDialog>
#include <QString>
#include <QtCore>
#include <QtGlobal>
#include <QtWidgets/QTreeWidgetItem>

#include "../mapdata/roomselection.h"
#include "../parser/abstractparser.h"
#include "ui_findroomsdlg.h" // auto-generated

class MapCanvas;
class MapData;
class QCloseEvent;
class QObject;
class QShortcut;
class QTreeWidgetItem;
class QWidget;
class Room;

class FindRoomsDlg final : public QDialog, private Ui::FindRoomsDlg
{
    Q_OBJECT

signals:
    void sig_center(const glm::vec2 &worldPos);
    void sig_newRoomSelection(const SigRoomSelection &);
    void sig_editSelection();
    void sig_log(const QString &, const QString &);

public slots:
    void slot_closeEvent(QCloseEvent *event)
    {
        /* virtual */
        closeEvent(event);
    }

private:
    void closeEvent(QCloseEvent *event) final;

public:
    explicit FindRoomsDlg(MapData &, QWidget *parent);
    ~FindRoomsDlg() final;

    void readSettings();
    void writeSettings();

private:
    MapData &m_mapData;
    QTreeWidgetItem *item = nullptr;
    QShortcut *m_showSelectedRoom = nullptr;

    void adjustResultTable();

private slots:
    QString slot_constructToolTip(const Room *);
    void slot_on_lineEdit_textChanged();
    void slot_findClicked();
    void slot_enableFindButton(const QString &text);
    void slot_itemDoubleClicked(QTreeWidgetItem *inputItem);
    void slot_showSelectedRoom();
};
