// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "infomarkseditdlg.h"

#include "../configuration/configuration.h"
#include "../display/InfoMarkSelection.h"
#include "../display/mapcanvas.h"
#include "../map/coordinate.h"
#include "../map/infomark.h"
#include "../mapdata/mapdata.h"

#include <cassert>
#include <memory>

#include <QString>
#include <QtWidgets>

InfoMarksEditDlg::InfoMarksEditDlg(QWidget *const parent)
    : QDialog(parent)
{
    setupUi(this);
    readSettings();

    connect(closeButton, &QAbstractButton::clicked, this, [this]() { this->accept(); });
}

void InfoMarksEditDlg::setInfoMarkSelection(const std::shared_ptr<InfoMarkSelection> &is,
                                            MapData *const md,
                                            MapCanvas *const mc)
{
    // NOTE: the selection is allowed to be null.
    assert(md != nullptr);
    assert(mc != nullptr);

    // NOTE: We don't own these.
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
            &InfoMarksEditDlg::sig_infomarksChanged,
            m_mapCanvas,
            &MapCanvas::slot_infomarksChanged);
    connect(objectsList,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &InfoMarksEditDlg::slot_objectListCurrentIndexChanged);
    connect(objectType,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &InfoMarksEditDlg::slot_objectTypeCurrentIndexChanged);
    connect(objectCreate, &QAbstractButton::clicked, this, &InfoMarksEditDlg::slot_createClicked);
    connect(objectModify, &QAbstractButton::clicked, this, &InfoMarksEditDlg::slot_modifyClicked);
}

void InfoMarksEditDlg::slot_objectListCurrentIndexChanged(int /*unused*/)
{
    updateDialog();
}

void InfoMarksEditDlg::slot_objectTypeCurrentIndexChanged(int /*unused*/)
{
    updateDialog();
}

void InfoMarksEditDlg::slot_createClicked()
{
    auto &mapData = deref(m_mapData);
    auto im = InfoMark::alloc(mapData);
    updateMark(*im);

    mapData.addMarker(im);
    m_selection->emplace_back(im);
    updateMarkers();
    setCurrentInfoMark(im.get());
    updateDialog();

    emit sig_infomarksChanged();
}

void InfoMarksEditDlg::updateMark(InfoMark &im)
{
    const Coordinate pos1(m_x1->value(), m_y1->value(), m_layer->value());
    const Coordinate pos2(m_x2->value(), m_y2->value(), m_layer->value());

    const int angle = static_cast<int>(std::lround(m_rotationAngle->value()));
    const InfoMarkTypeEnum type = getType();

    QString text = objectText->text();
    if (type == InfoMarkTypeEnum::TEXT) {
        if (text.isEmpty()) {
            text = "New Marker";
            objectText->setText(text);
        }
    } else {
        if (!text.isEmpty()) {
            text = "";
            objectText->setText(text);
        }
    }

    im.setType(type);
    im.setText(InfoMarkText{text});
    im.setClass(getClass());
    im.setPosition1(pos1);
    im.setPosition2(pos2);
    im.setRotationAngle(angle);
}

void InfoMarksEditDlg::slot_modifyClicked()
{
    InfoMark *const im = getCurrentInfoMark();
    if (im == nullptr)
        return;

    updateMark(*im);
    emit sig_infomarksChanged();
}

void InfoMarksEditDlg::disconnectAll()
{
    disconnect(this,
               &InfoMarksEditDlg::sig_infomarksChanged,
               m_mapCanvas,
               &MapCanvas::slot_infomarksChanged);
    disconnect(objectsList,
               QOverload<int>::of(&QComboBox::currentIndexChanged),
               this,
               &InfoMarksEditDlg::slot_objectListCurrentIndexChanged);
    disconnect(objectType,
               QOverload<int>::of(&QComboBox::currentIndexChanged),
               this,
               &InfoMarksEditDlg::slot_objectTypeCurrentIndexChanged);
    disconnect(objectCreate, &QAbstractButton::clicked, this, &InfoMarksEditDlg::slot_createClicked);
    disconnect(objectModify, &QAbstractButton::clicked, this, &InfoMarksEditDlg::slot_modifyClicked);
}

void InfoMarksEditDlg::updateMarkers()
{
    m_markers.clear();
    if (m_selection != nullptr) {
        m_markers.reserve(m_selection->size());
    }

    objectsList->clear();
    objectsList->addItem("Create New Marker", QVariant(-1));

    assert(m_selection);
    int n = 0;
    for (const auto &marker : *m_selection) {
        switch (marker->getType()) {
        case InfoMarkTypeEnum::TEXT:
        case InfoMarkTypeEnum::LINE:
        case InfoMarkTypeEnum::ARROW:
            assert(m_markers.size() == static_cast<size_t>(n));
            m_markers.emplace_back(marker);
            objectsList->addItem(marker->getText().toQString(), QVariant(n));
            ++n;
            break;
        }
    }

    if (m_selection->size() == 1)
        objectsList->setCurrentIndex(1);
}

void InfoMarksEditDlg::updateDialog()
{
    struct NODISCARD DisconnectReconnectAntiPattern final
    {
        InfoMarksEditDlg &self;
        explicit DisconnectReconnectAntiPattern(InfoMarksEditDlg &self)
            : self{self}
        {
            self.disconnectAll();
        }
        ~DisconnectReconnectAntiPattern() { self.connectAll(); }
    } antiPattern{*this};

    InfoMark *const im = getCurrentInfoMark();
    if (im != nullptr) {
        objectType->setCurrentIndex(static_cast<int>(im->getType()));
        objectClassesList->setCurrentIndex(static_cast<int>(im->getClass()));
    }

    switch (getType()) {
    case InfoMarkTypeEnum::TEXT:
        m_x2->setEnabled(false);
        m_y2->setEnabled(false);
        m_rotationAngle->setEnabled(true);
        objectText->setEnabled(true);
        break;
    case InfoMarkTypeEnum::LINE:
        m_x2->setEnabled(true);
        m_y2->setEnabled(true);
        m_rotationAngle->setEnabled(false);
        objectText->setEnabled(false);
        break;
    case InfoMarkTypeEnum::ARROW:
        m_x2->setEnabled(true);
        m_y2->setEnabled(true);
        m_rotationAngle->setEnabled(false);
        objectText->setEnabled(false);
        break;
    }

    InfoMark *marker = getCurrentInfoMark();

    if (marker == nullptr) {
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
        objectText->setText(marker->getText().toQString());
        m_x1->setValue(marker->getPosition1().x);
        m_y1->setValue(marker->getPosition1().y);
        m_x2->setValue(marker->getPosition2().x);
        m_y2->setValue(marker->getPosition2().y);
        m_rotationAngle->setValue(static_cast<double>(marker->getRotationAngle()));
        m_layer->setValue(marker->getPosition1().z);

        objectCreate->setEnabled(false);
        objectModify->setEnabled(true);
    }
}

InfoMarkTypeEnum InfoMarksEditDlg::getType()
{
    // FIXME: This needs bounds checking.
    return static_cast<InfoMarkTypeEnum>(objectType->currentIndex());
}

InfoMarkClassEnum InfoMarksEditDlg::getClass()
{
    // FIXME: This needs bounds checking.
    return static_cast<InfoMarkClassEnum>(objectClassesList->currentIndex());
}

InfoMark *InfoMarksEditDlg::getCurrentInfoMark()
{
    bool ok = false;
    int n = objectsList->itemData(objectsList->currentIndex()).toInt(&ok);
    if (!ok || n == -1 || n >= static_cast<int>(m_markers.size())) {
        return nullptr;
    }
    return m_markers.at(static_cast<size_t>(n)).get();
}

void InfoMarksEditDlg::setCurrentInfoMark(InfoMark *m)
{
    auto sm = m->shared_from_this();
    int i = 0;
    for (size_t j = 0, len = m_markers.size(); j < len; ++j)
        if (m_markers[j] == sm) {
            i = static_cast<int>(j) + 1;
            break;
        }

    objectsList->setCurrentIndex(i);
}
