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

class FindRoomsDlg : public QDialog, private Ui::FindRoomsDlg
{
    Q_OBJECT

signals:
    void center(qint32 x, qint32 y);
    void newRoomSelection(const SigRoomSelection &);
    void editSelection();
    void log(const QString &, const QString &);

public slots:
    void closeEvent(QCloseEvent *event) override;

public:
    explicit FindRoomsDlg(MapData *, QWidget *parent = nullptr);
    ~FindRoomsDlg() override;

private:
    MapData *m_mapData = nullptr;
    QTreeWidgetItem *item = nullptr;
    QShortcut *m_showSelectedRoom = nullptr;

    void adjustResultTable();

    static const QString nullString;

private slots:
    QString constructToolTip(const Room *);
    void on_lineEdit_textChanged();
    void findClicked();
    void enableFindButton(const QString &text);
    void itemDoubleClicked(QTreeWidgetItem *inputItem);
    void showSelectedRoom();
};
