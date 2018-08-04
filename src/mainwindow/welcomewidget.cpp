/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
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
    ui->port->setText(QString::number(Config().connection.localPort));

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
