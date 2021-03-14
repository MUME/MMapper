// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Mattias 'Mew_' Viklund <devmew@exedump.com> (Mirnir)

#include "autologpage.h"

#include <QSpinBox>
#include <QString>
#include <QtGui>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "ui_autologpage.h"

static const constexpr int MEGABYTE_IN_BYTES = 1000000;

AutoLogPage::AutoLogPage(QWidget *const parent)
    : QWidget(parent)
    , ui(new Ui::AutoLogPage)
{
    ui->setupUi(this);

    connect(ui->autoLogCheckBox,
            QOverload<bool>::of(&QCheckBox::toggled),
            this,
            [](const bool autoLog) { setConfig().autoLog.autoLog = autoLog; });
    connect(ui->selectAutoLogLocationButton,
            &QAbstractButton::clicked,
            this,
            &AutoLogPage::selectLogLocationButtonClicked);

    connect(ui->radioButtonKeepForever,
            QOverload<bool>::of(&QRadioButton::toggled),
            this,
            &AutoLogPage::logStrategyChanged);
    connect(ui->radioButtonDeleteDays,
            QOverload<bool>::of(&QRadioButton::toggled),
            this,
            &AutoLogPage::logStrategyChanged);
    connect(ui->spinBoxDays, QOverload<int>::of(&QSpinBox::valueChanged), this, [](const int size) {
        setConfig().autoLog.deleteWhenLogsReachDays = size;
    });
    connect(ui->radioButtonDeleteSize,
            QOverload<bool>::of(&QRadioButton::toggled),
            this,
            &AutoLogPage::logStrategyChanged);
    connect(ui->spinBoxSize, QOverload<int>::of(&QSpinBox::valueChanged), this, [](const int size) {
        setConfig().autoLog.deleteWhenLogsReachBytes = size * MEGABYTE_IN_BYTES;
    });
    connect(ui->askDeleteCheckBox,
            QOverload<bool>::of(&QCheckBox::toggled),
            this,
            [](const bool askDelete) { setConfig().autoLog.askDelete = askDelete; });

    connect(ui->autoLogMaxBytes,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            [](const int size) {
                setConfig().autoLog.deleteWhenLogsReachBytes = size * MEGABYTE_IN_BYTES;
            });
}

AutoLogPage::~AutoLogPage()
{
    delete ui;
}

void AutoLogPage::loadConfig()
{
    const auto &config = getConfig().autoLog;
    ui->autoLogCheckBox->setChecked(config.autoLog);
    ui->autoLogLocation->setText(config.autoLogDirectory);
    ui->autoLogMaxBytes->setValue(config.rotateWhenLogsReachBytes / MEGABYTE_IN_BYTES);
    switch (config.cleanupStrategy) {
    case AutoLoggerEnum::KeepForever:
        ui->radioButtonKeepForever->setChecked(true);
        break;
    case AutoLoggerEnum::DeleteDays:
        ui->radioButtonDeleteDays->setChecked(true);
        break;
    case AutoLoggerEnum::DeleteSize:
        ui->radioButtonDeleteSize->setChecked(true);
        break;
    default:
        abort();
    }
    ui->spinBoxDays->setValue(config.deleteWhenLogsReachDays);
    ui->spinBoxSize->setValue(config.deleteWhenLogsReachBytes / MEGABYTE_IN_BYTES);
    ui->askDeleteCheckBox->setChecked(config.askDelete);
}

void AutoLogPage::selectLogLocationButtonClicked(int /*unused*/)
{
    auto &config = setConfig().autoLog;
    QString logDirectory = QFileDialog::getExistingDirectory(this,
                                                             "Choose log location ...",
                                                             config.autoLogDirectory);

    if (!logDirectory.isEmpty()) {
        ui->autoLogLocation->setText(logDirectory);
        config.autoLogDirectory = logDirectory;
    }
}

void AutoLogPage::logStrategyChanged(int /*unused*/)
{
    auto &strategy = setConfig().autoLog.cleanupStrategy;
    if (ui->radioButtonKeepForever->isChecked())
        strategy = AutoLoggerEnum::KeepForever;
    else if (ui->radioButtonDeleteDays->isChecked())
        strategy = AutoLoggerEnum::DeleteDays;
    else if (ui->radioButtonDeleteSize->isChecked())
        strategy = AutoLoggerEnum::DeleteSize;
    else
        abort();
}
