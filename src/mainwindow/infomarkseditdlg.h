#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../map/infomark.h"
#include "ui_infomarkseditdlg.h"

#include <memory>
#include <vector>

#include <QDialog>
#include <QString>
#include <QtCore>

class Coordinate;
class MapCanvas;
class MapData;
class QCloseEvent;
class QObject;
class QWidget;
class InfoMarkSelection;

class InfoMarksEditDlg final : public QDialog, private Ui::InfoMarksEditDlg
{
    Q_OBJECT

public:
    explicit InfoMarksEditDlg(QWidget *parent);
    ~InfoMarksEditDlg() final;

    void setInfoMarkSelection(const std::shared_ptr<InfoMarkSelection> &is,
                              MapData *md,
                              MapCanvas *mc);

signals:
    void sig_infomarksChanged();

public slots:
    void slot_objectListCurrentIndexChanged(int);
    void slot_objectTypeCurrentIndexChanged(int);
    void slot_createClicked();
    void slot_modifyClicked();

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
