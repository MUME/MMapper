// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "welcomewidget.h"

#include <QPixmap>
#include <QString>
#include <QtWidgets>

#include "../configuration/configuration.h"
#include "ui_welcomewidget.h"

WelcomeWidget::WelcomeWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WelcomeWidget)
{
    ui->setupUi(this);

    // Picture
    QPixmap mellon(":/pixmaps/mellon.png");
    ui->pixmapLabel->setFixedSize(mellon.size());
    ui->pixmapLabel->setPixmap(mellon);

    // Port
    ui->port->setText(QString("%1").arg(getConfig().connection.localPort));

    ui->playButton->setFocus();
    connect(ui->playButton, &QAbstractButton::clicked, this, &WelcomeWidget::onPlayButtonClicked);
}

WelcomeWidget::~WelcomeWidget()
{
    delete ui;
}

void WelcomeWidget::onPlayButtonClicked(bool /*unused*/)
{
    emit playMumeClicked();
}
