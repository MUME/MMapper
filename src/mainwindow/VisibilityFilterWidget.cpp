// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "VisibilityFilterWidget.h"

#include "../configuration/configuration.h"
#include "../map/infomark.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

VisibilityFilterWidget::VisibilityFilterWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
    setupChangeCallbacks();
    updateCheckboxStates();
}

void VisibilityFilterWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);

    // Group box for marker checkboxes
    auto *groupBox = new QGroupBox(tr("Visibility Filter"), this);
    auto *gridLayout = new QGridLayout(groupBox);
    gridLayout->setSpacing(8);

    // Create checkboxes in 2-column layout
    m_genericCheckBox = new QCheckBox(tr("Generic"), groupBox);
    m_herbCheckBox = new QCheckBox(tr("Herb"), groupBox);
    m_riverCheckBox = new QCheckBox(tr("River"), groupBox);
    m_placeCheckBox = new QCheckBox(tr("Place"), groupBox);
    m_mobCheckBox = new QCheckBox(tr("Mob"), groupBox);
    m_commentCheckBox = new QCheckBox(tr("Comment"), groupBox);
    m_roadCheckBox = new QCheckBox(tr("Road"), groupBox);
    m_objectCheckBox = new QCheckBox(tr("Object"), groupBox);
    m_actionCheckBox = new QCheckBox(tr("Action"), groupBox);
    m_localityCheckBox = new QCheckBox(tr("Locality"), groupBox);
    m_connectionsCheckBox = new QCheckBox(tr("Connections"), groupBox);

    // Add checkboxes to grid (2 columns)
    gridLayout->addWidget(m_genericCheckBox, 0, 0);
    gridLayout->addWidget(m_herbCheckBox, 0, 1);
    gridLayout->addWidget(m_riverCheckBox, 1, 0);
    gridLayout->addWidget(m_placeCheckBox, 1, 1);
    gridLayout->addWidget(m_mobCheckBox, 2, 0);
    gridLayout->addWidget(m_commentCheckBox, 2, 1);
    gridLayout->addWidget(m_roadCheckBox, 3, 0);
    gridLayout->addWidget(m_objectCheckBox, 3, 1);
    gridLayout->addWidget(m_actionCheckBox, 4, 0);
    gridLayout->addWidget(m_localityCheckBox, 4, 1);
    gridLayout->addWidget(m_connectionsCheckBox, 5, 0);

    mainLayout->addWidget(groupBox);

    // Buttons layout
    auto *buttonLayout = new QHBoxLayout();
    m_showAllButton = new QPushButton(tr("Show All"), this);
    m_hideAllButton = new QPushButton(tr("Hide All"), this);
    buttonLayout->addWidget(m_showAllButton);
    buttonLayout->addWidget(m_hideAllButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();

    setLayout(mainLayout);
}

void VisibilityFilterWidget::connectSignals()
{
    auto &visibilityFilter = setConfig().canvas.visibilityFilter;

    connect(m_genericCheckBox, &QCheckBox::toggled, [this, &visibilityFilter](bool checked) {
        visibilityFilter.setVisible(InfomarkClassEnum::GENERIC, checked);
        emit sig_visibilityChanged();
    });

    connect(m_herbCheckBox, &QCheckBox::toggled, [this, &visibilityFilter](bool checked) {
        visibilityFilter.setVisible(InfomarkClassEnum::HERB, checked);
        emit sig_visibilityChanged();
    });

    connect(m_riverCheckBox, &QCheckBox::toggled, [this, &visibilityFilter](bool checked) {
        visibilityFilter.setVisible(InfomarkClassEnum::RIVER, checked);
        emit sig_visibilityChanged();
    });

    connect(m_placeCheckBox, &QCheckBox::toggled, [this, &visibilityFilter](bool checked) {
        visibilityFilter.setVisible(InfomarkClassEnum::PLACE, checked);
        emit sig_visibilityChanged();
    });

    connect(m_mobCheckBox, &QCheckBox::toggled, [this, &visibilityFilter](bool checked) {
        visibilityFilter.setVisible(InfomarkClassEnum::MOB, checked);
        emit sig_visibilityChanged();
    });

    connect(m_commentCheckBox, &QCheckBox::toggled, [this, &visibilityFilter](bool checked) {
        visibilityFilter.setVisible(InfomarkClassEnum::COMMENT, checked);
        emit sig_visibilityChanged();
    });

    connect(m_roadCheckBox, &QCheckBox::toggled, [this, &visibilityFilter](bool checked) {
        visibilityFilter.setVisible(InfomarkClassEnum::ROAD, checked);
        emit sig_visibilityChanged();
    });

    connect(m_objectCheckBox, &QCheckBox::toggled, [this, &visibilityFilter](bool checked) {
        visibilityFilter.setVisible(InfomarkClassEnum::OBJECT, checked);
        emit sig_visibilityChanged();
    });

    connect(m_actionCheckBox, &QCheckBox::toggled, [this, &visibilityFilter](bool checked) {
        visibilityFilter.setVisible(InfomarkClassEnum::ACTION, checked);
        emit sig_visibilityChanged();
    });

    connect(m_localityCheckBox, &QCheckBox::toggled, [this, &visibilityFilter](bool checked) {
        visibilityFilter.setVisible(InfomarkClassEnum::LOCALITY, checked);
        emit sig_visibilityChanged();
    });

    connect(m_connectionsCheckBox, &QCheckBox::toggled, [this, &visibilityFilter](bool checked) {
        visibilityFilter.setConnectionsVisible(checked);
        emit sig_connectionsVisibilityChanged();
    });

    // Show All / Hide All buttons
    connect(m_showAllButton, &QPushButton::clicked, [this, &visibilityFilter]() {
        visibilityFilter.showAll();
        emit sig_visibilityChanged();
        emit sig_connectionsVisibilityChanged();
    });

    connect(m_hideAllButton, &QPushButton::clicked, [this, &visibilityFilter]() {
        visibilityFilter.hideAll();
        emit sig_visibilityChanged();
        emit sig_connectionsVisibilityChanged();
    });
}

void VisibilityFilterWidget::setupChangeCallbacks()
{
    auto &visibilityFilter = setConfig().canvas.visibilityFilter;
    visibilityFilter.registerChangeCallback(m_changeMonitorLifetime, [this]() {
        updateCheckboxStates();
    });
}

void VisibilityFilterWidget::updateCheckboxStates()
{
    const auto &visibilityFilter = getConfig().canvas.visibilityFilter;

    // Block signals to prevent triggering setVisible during updates
    m_genericCheckBox->blockSignals(true);
    m_herbCheckBox->blockSignals(true);
    m_riverCheckBox->blockSignals(true);
    m_placeCheckBox->blockSignals(true);
    m_mobCheckBox->blockSignals(true);
    m_commentCheckBox->blockSignals(true);
    m_roadCheckBox->blockSignals(true);
    m_objectCheckBox->blockSignals(true);
    m_actionCheckBox->blockSignals(true);
    m_localityCheckBox->blockSignals(true);
    m_connectionsCheckBox->blockSignals(true);

    // Update checkbox states
    m_genericCheckBox->setChecked(visibilityFilter.isVisible(InfomarkClassEnum::GENERIC));
    m_herbCheckBox->setChecked(visibilityFilter.isVisible(InfomarkClassEnum::HERB));
    m_riverCheckBox->setChecked(visibilityFilter.isVisible(InfomarkClassEnum::RIVER));
    m_placeCheckBox->setChecked(visibilityFilter.isVisible(InfomarkClassEnum::PLACE));
    m_mobCheckBox->setChecked(visibilityFilter.isVisible(InfomarkClassEnum::MOB));
    m_commentCheckBox->setChecked(visibilityFilter.isVisible(InfomarkClassEnum::COMMENT));
    m_roadCheckBox->setChecked(visibilityFilter.isVisible(InfomarkClassEnum::ROAD));
    m_objectCheckBox->setChecked(visibilityFilter.isVisible(InfomarkClassEnum::OBJECT));
    m_actionCheckBox->setChecked(visibilityFilter.isVisible(InfomarkClassEnum::ACTION));
    m_localityCheckBox->setChecked(visibilityFilter.isVisible(InfomarkClassEnum::LOCALITY));
    m_connectionsCheckBox->setChecked(visibilityFilter.isConnectionsVisible());

    // Unblock signals
    m_genericCheckBox->blockSignals(false);
    m_herbCheckBox->blockSignals(false);
    m_riverCheckBox->blockSignals(false);
    m_placeCheckBox->blockSignals(false);
    m_mobCheckBox->blockSignals(false);
    m_commentCheckBox->blockSignals(false);
    m_roadCheckBox->blockSignals(false);
    m_objectCheckBox->blockSignals(false);
    m_actionCheckBox->blockSignals(false);
    m_localityCheckBox->blockSignals(false);
    m_connectionsCheckBox->blockSignals(false);
}
