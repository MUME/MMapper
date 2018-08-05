#pragma once
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

#ifndef GRAPHICSPAGE_H
#define GRAPHICSPAGE_H

#include <QString>
#include <QWidget>
#include <QtCore>

#include "ui_graphicspage.h"

class QObject;

namespace Ui {
class GraphicsPage;
}

class GraphicsPage : public QWidget
{
    Q_OBJECT

public:
    explicit GraphicsPage(QWidget *parent = nullptr);

signals:

public slots:
    void changeColorClicked();
    void antialiasingSamplesTextChanged(const QString &);
    void trilinearFilteringStateChanged(int);
    void softwareOpenGLStateChanged(int);

    void updatedStateChanged(int);
    void drawNotMappedExitsStateChanged(int);
    void drawNoMatchExitsStateChanged(int);
    void drawDoorNamesStateChanged(int);
    void drawUpperLayersTexturedStateChanged(int);

private:
    Ui::GraphicsPage *ui;
};

#endif // GRAPHICSPAGE_H
