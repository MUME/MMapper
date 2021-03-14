#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include <memory>
#include <vector>
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

    void setInfoMarkSelection(const std::shared_ptr<InfoMarkSelection> &is,
                              MapData *md,
                              MapCanvas *mc);

signals:
    void infomarksChanged();

public slots:
    void objectListCurrentIndexChanged(int);
    void objectTypeCurrentIndexChanged(int);

    void createClicked();
    void modifyClicked();

private:
    std::shared_ptr<InfoMarkSelection> m_selection;
    std::vector<std::shared_ptr<InfoMark>> m_markers;
    MapData *m_mapData = nullptr;
    MapCanvas *m_mapCanvas = nullptr;

    void connectAll();
    void disconnectAll();

    void readSettings();
    void writeSettings();

    NODISCARD InfoMarkTypeEnum getType();
    NODISCARD InfoMarkClassEnum getClass();
    NODISCARD InfoMark *getCurrentInfoMark();
    void setCurrentInfoMark(InfoMark *m);

    void updateMarkers();
    void updateDialog();
    void updateMark(InfoMark &im);
};
