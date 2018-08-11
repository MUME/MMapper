/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "infomarkseditdlg.h"

#include <algorithm>
#include <QString>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "../expandoracommon/coordinate.h"
#include "../mapdata/infomark.h"
#include "../mapdata/mapdata.h"

class QCloseEvent;

InfoMarksEditDlg::InfoMarksEditDlg(MapData *const mapData, QWidget *const parent)
    : QDialog(parent)
    , m_mapData(mapData)
{
    setupUi(this);
    readSettings();

    connect(closeButton, &QAbstractButton::clicked, this, [=]() {
        emit closeEventReceived();
        this->accept(); }
    );
}

void InfoMarksEditDlg::setPoints(
    const double x1, const double y1, const double x2, const double y2, const int layer)
{
    m_sel1.x = x1;
    m_sel1.y = y1;
    m_sel2.x = x2;
    m_sel2.y = y2;
    m_selLayer = layer;

    updateMarkers();
    updateDialog();
}

void InfoMarksEditDlg::closeEvent(QCloseEvent * /*unused*/)
{
    emit closeEventReceived();
}

InfoMarksEditDlg::~InfoMarksEditDlg()
{
    writeSettings();
}

void InfoMarksEditDlg::readSettings()
{
    const auto geometry = Config().readInfoMarksEditDlgGeometry();
    restoreGeometry(geometry);
}

void InfoMarksEditDlg::writeSettings()
{
    Config().writeInfoMarksEditDlgGeometry(saveGeometry());
}

void InfoMarksEditDlg::connectAll()
{
    connect(objectsList,
            SIGNAL(currentIndexChanged(const QString &)),
            this,
            SLOT(objectListCurrentIndexChanged(const QString &)));
    connect(objectType,
            SIGNAL(currentIndexChanged(const QString &)),
            this,
            SLOT(objectTypeCurrentIndexChanged(const QString &)));
    connect(objectCreate, &QAbstractButton::clicked, this, &InfoMarksEditDlg::createClicked);
    connect(objectModify, &QAbstractButton::clicked, this, &InfoMarksEditDlg::modifyClicked);
    connect(objectDelete, &QAbstractButton::clicked, this, &InfoMarksEditDlg::deleteClicked);
    connect(layerUpButton, &QAbstractButton::clicked, this, &InfoMarksEditDlg::onMoveUpClicked);
    connect(layerDownButton, &QAbstractButton::clicked, this, &InfoMarksEditDlg::onMoveDownClicked);
    connect(layerNButton, &QAbstractButton::clicked, this, &InfoMarksEditDlg::onMoveNorthClicked);
    connect(layerSButton, &QAbstractButton::clicked, this, &InfoMarksEditDlg::onMoveSouthClicked);
    connect(layerEButton, &QAbstractButton::clicked, this, &InfoMarksEditDlg::onMoveEastClicked);
    connect(layerWButton, &QAbstractButton::clicked, this, &InfoMarksEditDlg::onMoveWestClicked);
    connect(objectDeleteAll, &QAbstractButton::clicked, this, &InfoMarksEditDlg::onDeleteAllClicked);
}

void InfoMarksEditDlg::objectListCurrentIndexChanged(const QString & /*unused*/)
{
    updateDialog();
}

void InfoMarksEditDlg::objectTypeCurrentIndexChanged(const QString & /*unused*/)
{
    updateDialog();
}

void InfoMarksEditDlg::onDeleteAllClicked()
{
    for (int i = 0; i < objectsList->count(); i++) {
        if (auto m = getInfoMark(objectsList->itemText(i))) {
            if (m != nullptr) {
                m_mapData->removeMarker(m);
            }
        }
    }

    emit mapChanged();
    updateMarkers();
    updateDialog();
}

void InfoMarksEditDlg::onMoveClicked(const Coordinate &offset)
{
    // Moving both was the behavior when the switch statement had an undocumented fall-thru.
    static constexpr const bool MOVE_BOTH_ENDS_OF_LINES_AND_ARROWS = true;
    auto offset_pos1 = [&offset](auto m) { m->setPosition1(m->getPosition1() + offset); };
    auto offset_pos2 = [&offset](auto m) { m->setPosition2(m->getPosition2() + offset); };
    for (int i = 0; i < objectsList->count(); i++) {
        if (auto m = getInfoMark(objectsList->itemText(i))) {
            if (m != nullptr) {
                switch (m->getType()) {
                case InfoMarkType::LINE:
                case InfoMarkType::ARROW:
                    offset_pos2(m);
                    if (MOVE_BOTH_ENDS_OF_LINES_AND_ARROWS) {
                        offset_pos1(m);
                    }
                    break;
                case InfoMarkType::TEXT:
                    offset_pos1(m);
                    break;
                }
            }
        }
    }
    emit mapChanged();
    updateDialog();
}

void InfoMarksEditDlg::onMoveNorthClicked()
{
    onMoveClicked(Coordinate{0, -100, 0});
}

void InfoMarksEditDlg::onMoveSouthClicked()
{
    onMoveClicked(Coordinate{0, +100, 0});
}

void InfoMarksEditDlg::onMoveEastClicked()
{
    onMoveClicked(Coordinate{+100, 0, 0});
}

void InfoMarksEditDlg::onMoveWestClicked()
{
    onMoveClicked(Coordinate{-100, 0, 0});
}

void InfoMarksEditDlg::onMoveUpClicked()
{
    onMoveClicked(Coordinate{0, 0, +1});
}

void InfoMarksEditDlg::onMoveDownClicked()
{
    onMoveClicked(Coordinate{0, 0, -1});
}

void InfoMarksEditDlg::createClicked()
{
    MarkerList ml = m_mapData->getMarkersList();
    QString name = objectNameStr->text();

    if (name == "") {
        QMessageBox::critical(this, tr("MMapper2"), tr("Can't create objects with empty name!"));
    }

    for (const auto marker : ml) {
        if (marker->getName() == name) {
            QMessageBox::critical(this, tr("MMapper2"), tr("Object with this name already exists!"));
            return;
        }
    }

    auto *const im = new InfoMark();

    im->setType(getType());
    im->setName(name);
    im->setText(objectText->text());
    im->setClass(getClass());
    const Coordinate pos1(m_x1->value(),
                          m_y1->value(),
                          m_layer->value());
    const Coordinate pos2(m_x2->value(),
                          m_y2->value(),
                          m_layer->value());
    im->setPosition1(pos1);
    im->setPosition2(pos2);
    im->setRotationAngle(m_rotationAngle->value());

    m_mapData->addMarker(im);

    emit mapChanged();
    updateMarkers();
    setCurrentInfoMark(im);
    updateDialog();
}

void InfoMarksEditDlg::modifyClicked()
{
    InfoMark *const im = getCurrentInfoMark();

    im->setType(getType());
    im->setName(objectNameStr->text());
    im->setText(objectText->text());
    im->setClass(getClass());
    const Coordinate pos1(m_x1->value(),
                          m_y1->value(),
                          m_layer->value());
    const Coordinate pos2(m_x2->value(),
                          m_y2->value(),
                          m_layer->value());
    im->setPosition1(pos1);
    im->setPosition2(pos2);
    im->setRotationAngle(m_rotationAngle->value());

    emit mapChanged();
}

void InfoMarksEditDlg::deleteClicked()
{
    InfoMark *im = getCurrentInfoMark();

    m_mapData->removeMarker(im);

    emit mapChanged();
    updateMarkers();
    updateDialog();
}

void InfoMarksEditDlg::disconnectAll()
{
    disconnect(objectsList,
               SIGNAL(currentIndexChanged(const QString &)),
               this,
               SLOT(objectListCurrentIndexChanged(const QString &)));
    disconnect(objectType,
               SIGNAL(currentIndexChanged(const QString &)),
               this,
               SLOT(objectTypeCurrentIndexChanged(const QString &)));
    disconnect(objectCreate, &QAbstractButton::clicked, this, &InfoMarksEditDlg::createClicked);
    disconnect(objectModify, &QAbstractButton::clicked, this, &InfoMarksEditDlg::modifyClicked);
    disconnect(objectDelete, &QAbstractButton::clicked, this, &InfoMarksEditDlg::deleteClicked);
    disconnect(layerUpButton, &QAbstractButton::clicked, this, &InfoMarksEditDlg::onMoveUpClicked);
    disconnect(layerDownButton,
               &QAbstractButton::clicked,
               this,
               &InfoMarksEditDlg::onMoveDownClicked);
    disconnect(layerNButton, &QAbstractButton::clicked, this, &InfoMarksEditDlg::onMoveNorthClicked);
    disconnect(layerSButton, &QAbstractButton::clicked, this, &InfoMarksEditDlg::onMoveSouthClicked);
    disconnect(layerEButton, &QAbstractButton::clicked, this, &InfoMarksEditDlg::onMoveEastClicked);
    disconnect(layerWButton, &QAbstractButton::clicked, this, &InfoMarksEditDlg::onMoveWestClicked);
    disconnect(objectDeleteAll,
               &QAbstractButton::clicked,
               this,
               &InfoMarksEditDlg::onDeleteAllClicked);
}

void InfoMarksEditDlg::updateMarkers()
{
    const auto margin = 0.2;
    const auto bx1 = static_cast<int>(std::min(m_sel1.x, m_sel2.x) - margin) * 100;
    const auto by1 = static_cast<int>(std::min(m_sel1.y, m_sel2.y) - margin) * 100;
    const auto bx2 = static_cast<int>(std::max(m_sel1.x, m_sel2.x) + margin) * 100;
    const auto by2 = static_cast<int>(std::max(m_sel1.y, m_sel2.y) + margin) * 100;

    bool firstInside = false;
    bool secondInside = false;

    objectsList->clear();
    objectsList->addItem("Create New Marker");

    for (const auto marker : m_mapData->getMarkersList()) {
        const Coordinate c1 = marker->getPosition1();
        const Coordinate c2 = marker->getPosition2();

        firstInside = false;
        secondInside = false;

        if (c1.x > bx1 && c1.x < bx2
            && c1.y > by1 && c1.y < by2) {
            firstInside = true;
        }
        if (c2.x > bx1 && c2.x < bx2
            && c2.y > by1 && c2.y < by2) {
            secondInside = true;
        }

        switch (marker->getType()) {
        case InfoMarkType::TEXT:
            if (firstInside && m_selLayer == c1.z) {
                objectsList->addItem(marker->getName());
            }
            break;
        case InfoMarkType::LINE:
            if (m_selLayer == c1.z && (firstInside || secondInside)) {
                objectsList->addItem(marker->getName());
            }
            break;
        case InfoMarkType::ARROW:
            if (m_selLayer == c1.z && (firstInside || secondInside)) {
                objectsList->addItem(marker->getName());
            }
            break;
        }
    }
}

void InfoMarksEditDlg::updateDialog()
{
    disconnectAll();

    InfoMark *const im = getCurrentInfoMark();
    if (im != nullptr) {
        objectType->setCurrentIndex(static_cast<int>(im->getType()));
        objectClassesList->setCurrentIndex(static_cast<int>(im->getClass()));
    }

    switch (getType()) {
    case InfoMarkType::TEXT:
        m_x2->setEnabled(false);
        m_y2->setEnabled(false);
        m_rotationAngle->setEnabled(true);
        objectText->setEnabled(true);
        break;
    case InfoMarkType::LINE:
        m_x2->setEnabled(true);
        m_y2->setEnabled(true);
        m_rotationAngle->setEnabled(false);
        objectText->setEnabled(false);
        break;
    case InfoMarkType::ARROW:
        m_x2->setEnabled(true);
        m_y2->setEnabled(true);
        m_rotationAngle->setEnabled(false);
        objectText->setEnabled(false);
        break;
    }

    InfoMark *marker = getCurrentInfoMark();

    if (marker == nullptr) {
        objectNameStr->clear();
        objectText->clear();
        m_x1->setValue(m_sel1.x * 100);
        m_y1->setValue(m_sel1.y * 100);
        m_x2->setValue(m_sel2.x * 100);
        m_y2->setValue(m_sel2.y * 100);
        m_rotationAngle->setValue(0.0);
        m_layer->setValue(m_selLayer);

        objectCreate->setEnabled(true);
        objectModify->setEnabled(false);
        objectDelete->setEnabled(false);
    } else {
        objectNameStr->setText(marker->getName());
        objectText->setText(marker->getText());
        m_x1->setValue(marker->getPosition1().x);
        m_y1->setValue(marker->getPosition1().y);
        m_x2->setValue(marker->getPosition2().x);
        m_y2->setValue(marker->getPosition2().y);
        m_rotationAngle->setValue(marker->getRotationAngle());
        m_layer->setValue(marker->getPosition1().z);

        objectCreate->setEnabled(false);
        objectModify->setEnabled(true);
        objectDelete->setEnabled(true);
    }

    connectAll();
}

InfoMarkType InfoMarksEditDlg::getType()
{
    return static_cast<InfoMarkType>(objectType->currentIndex());
}

InfoMarkClass InfoMarksEditDlg::getClass()
{
    return static_cast<InfoMarkClass>(objectClassesList->currentIndex());
}

InfoMark *InfoMarksEditDlg::getCurrentInfoMark()
{
    if (objectsList->currentText() == "Create New Marker") {
        return nullptr;
    }
    return getInfoMark(objectsList->currentText());
}

void InfoMarksEditDlg::setCurrentInfoMark(InfoMark *m)
{
    int i = objectsList->findText(m->getName());
    if (i == -1) {
        i = 0;
    }
    objectsList->setCurrentIndex(i);
}

InfoMark *InfoMarksEditDlg::getInfoMark(const QString &name)
{
    for (const auto marker : m_mapData->getMarkersList()) {
        if (marker->getName() == name) {
            return marker;
        }
    }
    return nullptr;
}
