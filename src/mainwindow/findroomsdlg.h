#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Kalev Lember <kalev@smartlink.ee> (Kalev)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../mapdata/roomselection.h"
#include "../parser/abstractparser.h"

#include <memory>

#include <QDialog>
#include <QString>
#include <QtCore>
#include <QtGlobal>
#include <QtWidgets/QTreeWidgetItem>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
#include "ui_findroomsdlg.h" // auto-generated
#ifdef __clang__
#pragma clang diagnostic pop
#endif

class MapCanvas;
class MapData;
class QCloseEvent;
class QObject;
class QShortcut;
class QTreeWidgetItem;
class QWidget;
class Room;

class NODISCARD_QOBJECT FindRoomsDlg final : public QDialog, private Ui::FindRoomsDlg
{
    Q_OBJECT

private:
    MapData &m_mapData;
    QTreeWidgetItem *item = nullptr;
    std::unique_ptr<QShortcut> m_showSelectedRoom;

private:
    void closeEvent(QCloseEvent *event) final;

public:
    explicit FindRoomsDlg(MapData &, QWidget *parent);
    ~FindRoomsDlg() final;

    void readSettings();
    void writeSettings();

private:
    void adjustResultTable();
    NODISCARD QString constructToolTip(const Room *);

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
private slots:
    void slot_on_lineEdit_textChanged();
    void slot_findClicked();
    void slot_enableFindButton(const QString &text);
    void slot_itemDoubleClicked(QTreeWidgetItem *inputItem);
    void slot_showSelectedRoom();
};
