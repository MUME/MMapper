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

#include <cassert>
#include <QString>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "../display/InfoMarkSelection.h"
#include "../display/mapcanvas.h"
#include "../expandoracommon/coordinate.h"
#include "../mapdata/infomark.h"
#include "../mapdata/mapdata.h"

InfoMarksEditDlg::InfoMarksEditDlg(QWidget *const parent)
    : QDialog(parent)
{
    setupUi(this);
    readSettings();

    connect(closeButton, &QAbstractButton::clicked, this, [this]() { this->accept(); });
}

void InfoMarksEditDlg::setInfoMarkSelection(InfoMarkSelection *is, MapData *md, MapCanvas *mc)
{
    m_selection = is;
    m_mapData = md;
    m_mapCanvas = mc;
    updateMarkers();
    updateDialog();
}

InfoMarksEditDlg::~InfoMarksEditDlg()
{
    writeSettings();
}

void InfoMarksEditDlg::readSettings()
{
    restoreGeometry(getConfig().infoMarksDialog.geometry);
}

void InfoMarksEditDlg::writeSettings()
{
    setConfig().infoMarksDialog.geometry = saveGeometry();
}

void InfoMarksEditDlg::connectAll()
{
    connect(this,
            &InfoMarksEditDlg::mapChanged,
            m_mapCanvas,
            static_cast<void (QWidget::*)(void)>(&QWidget::update));
    connect(objectsList,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
            this,
            &InfoMarksEditDlg::objectListCurrentIndexChanged);
    connect(objectType,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
            this,
            &InfoMarksEditDlg::objectTypeCurrentIndexChanged);
    connect(objectCreate, &QAbstractButton::clicked, this, &InfoMarksEditDlg::createClicked);
    connect(objectModify, &QAbstractButton::clicked, this, &InfoMarksEditDlg::modifyClicked);
}

void InfoMarksEditDlg::objectListCurrentIndexChanged(const QString & /*unused*/)
{
    updateDialog();
}

void InfoMarksEditDlg::objectTypeCurrentIndexChanged(const QString & /*unused*/)
{
    updateDialog();
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
    const Coordinate pos1(m_x1->value(), m_y1->value(), m_layer->value());
    const Coordinate pos2(m_x2->value(), m_y2->value(), m_layer->value());
    im->setPosition1(pos1);
    im->setPosition2(pos2);
    im->setRotationAngle(static_cast<float>(m_rotationAngle->value()));

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
    const Coordinate pos1(m_x1->value(), m_y1->value(), m_layer->value());
    const Coordinate pos2(m_x2->value(), m_y2->value(), m_layer->value());
    im->setPosition1(pos1);
    im->setPosition2(pos2);
    im->setRotationAngle(static_cast<float>(m_rotationAngle->value()));

    emit mapChanged();
}

void InfoMarksEditDlg::disconnectAll()
{
    disconnect(this,
               &InfoMarksEditDlg::mapChanged,
               m_mapCanvas,
               static_cast<void (QWidget::*)(void)>(&QWidget::update));
    disconnect(objectsList,
               static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
               this,
               &InfoMarksEditDlg::objectListCurrentIndexChanged);
    disconnect(objectType,
               static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
               this,
               &InfoMarksEditDlg::objectTypeCurrentIndexChanged);
    disconnect(objectCreate, &QAbstractButton::clicked, this, &InfoMarksEditDlg::createClicked);
    disconnect(objectModify, &QAbstractButton::clicked, this, &InfoMarksEditDlg::modifyClicked);
}

void InfoMarksEditDlg::updateMarkers()
{
    objectsList->clear();
    objectsList->addItem("Create New Marker");

    assert(m_selection);
    for (const auto marker : *m_selection) {
        switch (marker->getType()) {
        case InfoMarkType::TEXT:
            objectsList->addItem(marker->getName());
            break;
        case InfoMarkType::LINE:
            objectsList->addItem(marker->getName());
            break;
        case InfoMarkType::ARROW:
            objectsList->addItem(marker->getName());
            break;
        }
    }

    if (m_selection->size() == 1)
        objectsList->setCurrentIndex(m_selection->size());
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
        m_x1->setValue(m_selection->getPosition1().x);
        m_y1->setValue(m_selection->getPosition1().y);
        m_x2->setValue(m_selection->getPosition2().x);
        m_y2->setValue(m_selection->getPosition2().y);
        m_rotationAngle->setValue(0.0);
        m_layer->setValue(m_selection->getPosition1().z);

        objectCreate->setEnabled(true);
        objectModify->setEnabled(false);
    } else {
        objectNameStr->setText(marker->getName());
        objectText->setText(marker->getText());
        m_x1->setValue(marker->getPosition1().x);
        m_y1->setValue(marker->getPosition1().y);
        m_x2->setValue(marker->getPosition2().x);
        m_y2->setValue(marker->getPosition2().y);
        m_rotationAngle->setValue(static_cast<double>(marker->getRotationAngle()));
        m_layer->setValue(marker->getPosition1().z);

        objectCreate->setEnabled(false);
        objectModify->setEnabled(true);
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
