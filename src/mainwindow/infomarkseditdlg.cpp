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
#include "defs.h"
#include "mapcanvas.h"
#include "mapdata.h"
#include <cmath>

#include <QMessageBox>
#include <QSettings>

InfoMarksEditDlg::InfoMarksEditDlg(MapData *mapData, QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    readSettings();

    m_selX1 = 0;
    m_selY1 = 0;
    m_selX2 = 0;
    m_selY2 = 0;
    m_selLayer = 0;

    m_mapData = mapData;
}

void InfoMarksEditDlg::setPoints(double x1, double y1, double x2, double y2, int layer)
{
    m_selX1 = x1;
    m_selY1 = y1;
    m_selX2 = x2;
    m_selY2 = y2;
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
    QSettings settings("Caligor soft", "MMapper2");
    settings.beginGroup("InfoMarksEditDlg");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    settings.endGroup();
    move(pos);
}

void InfoMarksEditDlg::writeSettings()
{
    QSettings settings("Caligor soft", "MMapper2");
    settings.beginGroup("InfoMarksEditDlg");
    settings.setValue("pos", pos());
    settings.endGroup();
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
    connect(objectClassesList,
            SIGNAL(currentIndexChanged(const QString &)),
            this,
            SLOT(objectClassCurrentIndexChanged(const QString &)));
    connect(objectNameStr, SIGNAL(textChanged(QString)), this, SLOT(objectNameTextChanged(QString)));
    connect(objectText, SIGNAL(textChanged(QString)), this, SLOT(objectTextChanged(QString)));
    connect(m_x1, SIGNAL(valueChanged(double)), this, SLOT(x1ValueChanged(double)));
    connect(m_y1, SIGNAL(valueChanged(double)), this, SLOT(y1ValueChanged(double)));
    connect(m_x2, SIGNAL(valueChanged(double)), this, SLOT(x2ValueChanged(double)));
    connect(m_y2, SIGNAL(valueChanged(double)), this, SLOT(y2ValueChanged(double)));
    connect(m_rotationAngle, SIGNAL(valueChanged(double)), this, SLOT(rotValueChanged(double)));
    connect(m_layer, SIGNAL(valueChanged(int)), this, SLOT(layerValueChanged(int)));
    connect(objectCreate, SIGNAL(clicked()), this, SLOT(createClicked()));
    connect(objectModify, SIGNAL(clicked()), this, SLOT(modifyClicked()));
    connect(objectDelete, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(layerUpButton, SIGNAL(clicked()), this, SLOT(onMoveUpClicked()));
    connect(layerDownButton, SIGNAL(clicked()), this, SLOT(onMoveDownClicked()));
    connect(layerNButton, SIGNAL(clicked()), this, SLOT(onMoveNorthClicked()));
    connect(layerSButton, SIGNAL(clicked()), this, SLOT(onMoveSouthClicked()));
    connect(layerEButton, SIGNAL(clicked()), this, SLOT(onMoveEastClicked()));
    connect(layerWButton, SIGNAL(clicked()), this, SLOT(onMoveWestClicked()));
    connect(objectDeleteAll, SIGNAL(clicked()), this, SLOT(onDeleteAllClicked()));
}

void InfoMarksEditDlg::objectListCurrentIndexChanged(const QString & /*unused*/)
{
    updateDialog();
}

void InfoMarksEditDlg::objectTypeCurrentIndexChanged(const QString & /*unused*/)
{
    updateDialog();
}

void InfoMarksEditDlg::objectClassCurrentIndexChanged(const QString & /*unused*/) {}

void InfoMarksEditDlg::objectNameTextChanged(const QString /*unused*/ &) {}

void InfoMarksEditDlg::objectTextChanged(const QString /*unused*/ &) {}

void InfoMarksEditDlg::x1ValueChanged(double /*unused*/) {}

void InfoMarksEditDlg::y1ValueChanged(double /*unused*/) {}

void InfoMarksEditDlg::x2ValueChanged(double /*unused*/) {}

void InfoMarksEditDlg::y2ValueChanged(double /*unused*/) {}

void InfoMarksEditDlg::rotValueChanged(double /*unused*/) {}

void InfoMarksEditDlg::layerValueChanged(int /*unused*/) {}

void InfoMarksEditDlg::onDeleteAllClicked()
{
    InfoMark *m;

    for (int i = 0; i < objectsList->count(); i++) {
        m = getInfoMark(objectsList->itemText(i));
        if (m != nullptr) {
            m_mapData->removeMarker(m);
        }
    }

    emit mapChanged();
    updateMarkers();
    updateDialog();
}

void InfoMarksEditDlg::onMoveNorthClicked()
{
    InfoMark *m;
    Coordinate c;

    for (int i = 0; i < objectsList->count(); i++) {
        m = getInfoMark(objectsList->itemText(i));
        if (m != nullptr) {
            switch (m->getType()) {
            case MT_LINE:
            case MT_ARROW:
                c = m->getPosition2();
                c.y -= 100;
                m->setPosition2(c);
            case MT_TEXT:
                c = m->getPosition1();
                c.y -= 100;
                m->setPosition1(c);
                break;
            }
        }
    }
    emit mapChanged();
    updateDialog();
}

void InfoMarksEditDlg::onMoveSouthClicked()
{
    InfoMark *m;
    Coordinate c;

    for (int i = 0; i < objectsList->count(); i++) {
        m = getInfoMark(objectsList->itemText(i));
        if (m != nullptr) {
            switch (m->getType()) {
            case MT_LINE:
            case MT_ARROW:
                c = m->getPosition2();
                c.y += 100;
                m->setPosition2(c);
            case MT_TEXT:
                c = m->getPosition1();
                c.y += 100;
                m->setPosition1(c);
                break;
            }
        }
    }
    emit mapChanged();
    updateDialog();
}

void InfoMarksEditDlg::onMoveEastClicked()
{
    InfoMark *m;
    Coordinate c;

    for (int i = 0; i < objectsList->count(); i++) {
        m = getInfoMark(objectsList->itemText(i));
        if (m != nullptr) {
            switch (m->getType()) {
            case MT_LINE:
            case MT_ARROW:
                c = m->getPosition2();
                c.x += 100;
                m->setPosition2(c);
            case MT_TEXT:
                c = m->getPosition1();
                c.x += 100;
                m->setPosition1(c);
                break;
            }
        }
    }
    emit mapChanged();
    updateDialog();
}

void InfoMarksEditDlg::onMoveWestClicked()
{
    InfoMark *m;
    Coordinate c;

    for (int i = 0; i < objectsList->count(); i++) {
        m = getInfoMark(objectsList->itemText(i));
        if (m != nullptr) {
            switch (m->getType()) {
            case MT_LINE:
            case MT_ARROW:
                c = m->getPosition2();
                c.x -= 100;
                m->setPosition2(c);
            case MT_TEXT:
                c = m->getPosition1();
                c.x -= 100;
                m->setPosition1(c);
                break;
            }
        }
    }
    emit mapChanged();
    updateDialog();
}

void InfoMarksEditDlg::onMoveUpClicked()
{
    InfoMark *m = nullptr;
    Coordinate c;

    for (int i = 0; i < objectsList->count(); i++) {
        m = getInfoMark(objectsList->itemText(i));
        if (m != nullptr) {
            c = m->getPosition1();
            c.z++;
            m->setPosition1(c);

            c = m->getPosition2();
            c.z++;
            m->setPosition2(c);
        }
    }
    emit mapChanged();
    updateDialog();
}

void InfoMarksEditDlg::onMoveDownClicked()
{
    InfoMark *m = nullptr;
    Coordinate c;

    for (int i = 0; i < objectsList->count(); i++) {
        m = getInfoMark(objectsList->itemText(i));
        if (m != nullptr) {
            m = getInfoMark(objectsList->itemText(i));
            c = m->getPosition1();
            c.z--;
            m->setPosition1(c);

            c = m->getPosition2();
            c.z--;
            m->setPosition2(c);
        }
    }
    emit mapChanged();
    updateDialog();
}

void InfoMarksEditDlg::createClicked()
{
    MarkerList ml = m_mapData->getMarkersList();
    QString name = objectNameStr->text();

    if (name == "") {
        QMessageBox::critical(this, tr("MMapper2"), tr("Can't create objects with empty name!"));
    }

    MarkerListIterator mi(ml);
    while (mi.hasNext()) {
        InfoMark *marker = mi.next();
        if (marker->getName() == name) {
            QMessageBox::critical(this, tr("MMapper2"), tr("Object with this name already exists!"));
            return;
        }
    }

    auto *im = new InfoMark();

    im->setType(getType());
    im->setName(name);
    im->setText(objectText->text());
    im->setClass(getClass());
    Coordinate pos1(static_cast<int>(m_x1->value() * 100.0f),
                    static_cast<int>(m_y1->value() * 100.0f),
                    m_layer->value());
    Coordinate pos2(static_cast<int>(m_x2->value() * 100.0f),
                    static_cast<int>(m_y2->value() * 100.0f),
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
    InfoMark *im = getCurrentInfoMark();

    im->setType(getType());
    im->setName(objectNameStr->text());
    im->setText(objectText->text());
    im->setClass(getClass());
    Coordinate pos1(static_cast<int>(m_x1->value() * 100.0f),
                    static_cast<int>(m_y1->value() * 100.0f),
                    m_layer->value());
    Coordinate pos2(static_cast<int>(m_x2->value() * 100.0f),
                    static_cast<int>(m_y2->value() * 100.0f),
                    m_layer->value());
    im->setPosition1(pos1);
    im->setPosition2(pos2);
    im->setRotationAngle(m_rotationAngle->value());

    emit mapChanged();
}

void InfoMarksEditDlg::deleteClicked()
{
    InfoMark *im = getCurrentInfoMark();

    //MarkerList ml = m_mapData->getMarkersList();
    m_mapData->removeMarker(im);
    //ml.removeAll(im);
    //delete im;

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
    disconnect(objectNameStr,
               SIGNAL(textChanged(QString)),
               this,
               SLOT(objectNameTextChanged(QString)));
    disconnect(objectText, SIGNAL(textChanged(QString)), this, SLOT(objectTextChanged(QString)));
    disconnect(objectClassesList,
               SIGNAL(currentIndexChanged(const QString &)),
               this,
               SLOT(objectClassCurrentIndexChanged(const QString &)));
    disconnect(m_x1, SIGNAL(valueChanged(double)), this, SLOT(x1ValueChanged(double)));
    disconnect(m_y1, SIGNAL(valueChanged(double)), this, SLOT(y1ValueChanged(double)));
    disconnect(m_x2, SIGNAL(valueChanged(double)), this, SLOT(x2ValueChanged(double)));
    disconnect(m_y2, SIGNAL(valueChanged(double)), this, SLOT(y2ValueChanged(double)));
    disconnect(m_rotationAngle, SIGNAL(valueChanged(double)), this, SLOT(rotValueChanged(double)));
    disconnect(m_layer, SIGNAL(valueChanged(int)), this, SLOT(layerValueChanged(int)));
    disconnect(objectCreate, SIGNAL(clicked()), this, SLOT(createClicked()));
    disconnect(objectModify, SIGNAL(clicked()), this, SLOT(modifyClicked()));
    disconnect(objectDelete, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    disconnect(layerUpButton, SIGNAL(clicked()), this, SLOT(onMoveUpClicked()));
    disconnect(layerDownButton, SIGNAL(clicked()), this, SLOT(onMoveDownClicked()));
    disconnect(layerNButton, SIGNAL(clicked()), this, SLOT(onMoveNorthClicked()));
    disconnect(layerSButton, SIGNAL(clicked()), this, SLOT(onMoveSouthClicked()));
    disconnect(layerEButton, SIGNAL(clicked()), this, SLOT(onMoveEastClicked()));
    disconnect(layerWButton, SIGNAL(clicked()), this, SLOT(onMoveWestClicked()));
    disconnect(objectDeleteAll, SIGNAL(clicked()), this, SLOT(onDeleteAllClicked()));
}

void InfoMarksEditDlg::updateMarkers()
{
    double bx1, by1, bx2, by2;
    bx1 = m_selX1 < m_selX2 ? m_selX1 : m_selX2;
    by1 = m_selY1 < m_selY2 ? m_selY1 : m_selY2;
    bx2 = m_selX1 < m_selX2 ? m_selX2 : m_selX1;
    by2 = m_selY1 < m_selY2 ? m_selY2 : m_selY1;

    bx1 -= 0.2;
    bx2 += 0.2;
    by1 -= 0.2;
    by2 += 0.2;

    bool firstInside;
    bool secondInside;

    objectsList->clear();
    objectsList->addItem("Create New Marker");

    MarkerListIterator mi(m_mapData->getMarkersList());
    while (mi.hasNext()) {
        InfoMark *marker = mi.next();
        Coordinate c1 = marker->getPosition1();
        Coordinate c2 = marker->getPosition2();

        firstInside = false;
        secondInside = false;

        if (c1.x / 100.0f > bx1 && c1.x / 100.0f < bx2 && c1.y / 100.0f > by1
            && c1.y / 100.0f < by2) {
            firstInside = true;
        }
        if (c2.x / 100.0f > bx1 && c2.x / 100.0f < bx2 && c2.y / 100.0f > by1
            && c2.y / 100.0f < by2) {
            secondInside = true;
        }

        switch (marker->getType()) {
        case MT_TEXT:
            if (firstInside && m_selLayer == c1.z) {
                objectsList->addItem(marker->getName());
            }
            break;
        case MT_LINE:
            if (m_selLayer == c1.z && (firstInside || secondInside)) {
                objectsList->addItem(marker->getName());
            }
            break;
        case MT_ARROW:
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

    InfoMark *im = getCurrentInfoMark();
    int i = 0;
    int j = 0;
    if (im != nullptr) {
        i = static_cast<int>(im->getType());
        objectType->setCurrentIndex(i);
        j = static_cast<int>(im->getClass());
        objectClassesList->setCurrentIndex(j);
    }

    switch (getType()) {
    case MT_TEXT:
        m_x2->setEnabled(false);
        m_y2->setEnabled(false);
        m_rotationAngle->setEnabled(true);
        objectText->setEnabled(true);
        break;
    case MT_LINE:
        m_x2->setEnabled(true);
        m_y2->setEnabled(true);
        m_rotationAngle->setEnabled(false);
        objectText->setEnabled(false);
        break;
    case MT_ARROW:
        m_x2->setEnabled(true);
        m_y2->setEnabled(true);
        m_rotationAngle->setEnabled(false);
        objectText->setEnabled(false);
        break;
    }

    InfoMark *marker;

    if ((marker = getCurrentInfoMark()) == nullptr) {
        objectNameStr->clear();
        objectText->clear();
        m_x1->setValue(m_selX1);
        m_y1->setValue(m_selY1);
        m_x2->setValue(m_selX2);
        m_y2->setValue(m_selY2);
        m_rotationAngle->setValue(0.0f);
        m_layer->setValue(m_selLayer);

        objectCreate->setEnabled(true);
        objectModify->setEnabled(false);
        objectDelete->setEnabled(false);
    } else {
        objectNameStr->setText(marker->getName());
        objectText->setText(marker->getText());
        m_x1->setValue(marker->getPosition1().x / 100.0f);
        m_y1->setValue(marker->getPosition1().y / 100.0f);
        m_x2->setValue(marker->getPosition2().x / 100.0f);
        m_y2->setValue(marker->getPosition2().y / 100.0f);
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
    MarkerListIterator mi(m_mapData->getMarkersList());
    while (mi.hasNext()) {
        InfoMark *marker = mi.next();
        if (marker->getName() == name) {
            return marker;
        }
    }
    return nullptr;
}
