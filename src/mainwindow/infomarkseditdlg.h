#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <QDialog>
#include <QString>
#include <QtCore>

#include "../mapdata/infomark.h"
#include "ui_infomarkseditdlg.h"

class Coordinate;
class MapCanvas;
class MapData;
class QCloseEvent;
class QObject;
class QWidget;
class InfoMarkSelection;

class InfoMarksEditDlg : public QDialog, private Ui::InfoMarksEditDlg
{
    Q_OBJECT

public:
    explicit InfoMarksEditDlg(QWidget *parent = nullptr);
    ~InfoMarksEditDlg() override;

    void setInfoMarkSelection(InfoMarkSelection *is, MapData *md, MapCanvas *mc);

signals:
    void mapChanged();

public slots:
    void objectListCurrentIndexChanged(const QString &);
    void objectTypeCurrentIndexChanged(const QString &);

    void createClicked();
    void modifyClicked();

private:
    InfoMarkSelection *m_selection = nullptr;
    MapData *m_mapData = nullptr;
    MapCanvas *m_mapCanvas = nullptr;

    void connectAll();
    void disconnectAll();

    void readSettings();
    void writeSettings();

    InfoMarkType getType();
    InfoMarkClass getClass();
    InfoMark *getInfoMark(const QString &name);
    InfoMark *getCurrentInfoMark();
    void setCurrentInfoMark(InfoMark *m);

    void updateMarkers();
    void updateDialog();
};
