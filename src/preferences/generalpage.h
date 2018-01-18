/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#ifndef GENERALPAGE_H
#define GENERALPAGE_H

#include <QWidget>
#include "ui_generalpage.h"

class GeneralPage : public QWidget, private Ui::GeneralPage
{
    Q_OBJECT

public slots:

    void remoteNameTextChanged(const QString &);
    void remotePortValueChanged(int);
    void localPortValueChanged(int);

    void changeColorClicked();
    void antialiasingSamplesTextChanged(const QString &);
    void trilinearFilteringStateChanged(int);

    void emulatedExitsStateChanged(int);
    void showHiddenExitFlagsStateChanged(int);

    void updatedStateChanged(int);
    void drawNotMappedExitsStateChanged(int);
    void drawNoMatchExitsStateChanged(int);
    void drawDoorNamesStateChanged(int);
    void drawUpperLayersTexturedStateChanged(int);

    void autoLoadFileNameTextChanged(const QString &);
    void autoLoadCheckStateChanged(int);

    void selectWorldFileButtonClicked();

    void displayMumeClockStateChanged(int);

public:
    GeneralPage(QWidget *parent = 0);
};


#endif

