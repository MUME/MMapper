#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/Connections.h"
#include "../map/infomark.h"
#include "ui_infomarkseditdlg.h"

#include <memory>
#include <vector>

#include <QDialog>
#include <QString>
#include <QtCore>

class Coordinate;
class InfomarkSelection;
class MapCanvas;
class MapData;
class QCloseEvent;
class QObject;
class QWidget;
struct RawInfomark;

class NODISCARD_QOBJECT InfomarksEditDlg final : public QDialog, private Ui::InfomarksEditDlg
{
    Q_OBJECT

private:
    mmqt::Connections m_connections;
    std::shared_ptr<InfomarkSelection> m_selection;
    std::vector<InfomarkId> m_markers;
    MapData *m_mapData = nullptr;
    MapCanvas *m_mapCanvas = nullptr;

public:
    explicit InfomarksEditDlg(QWidget *parent);
    ~InfomarksEditDlg() final;

    void setInfomarkSelection(const std::shared_ptr<InfomarkSelection> &is,
                              MapData *md,
                              MapCanvas *mc);

private:
    void connectAll();
    void disconnectAll();

    void readSettings();
    void writeSettings();

    NODISCARD InfomarkTypeEnum getType();
    NODISCARD InfomarkClassEnum getClass();
    NODISCARD InfomarkHandle getCurrentInfomark();
    void setCurrentInfomark(InfomarkId m);

    void updateMarkers();
    void updateDialog();
    void updateMark(RawInfomark &im);

public slots:
    void slot_objectListCurrentIndexChanged(int);
    void slot_objectTypeCurrentIndexChanged(int);
    void slot_createClicked();
    void slot_modifyClicked();
};
