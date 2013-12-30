/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#ifndef GROUPMANAGERPAGE_H
#define GROUPMANAGERPAGE_H

#include <QWidget>
#include "ui_groupmanagerpage.h"

class CGroup;

class GroupManagerPage : public QWidget, private Ui::GroupManagerPage
{
  Q_OBJECT

  public slots:

    void colorNameTextChanged();
    void changeColorClicked();
    void charNameTextChanged();

    void remoteHostTextChanged();
    void remotePortValueChanged(int);
    void localPortValueChanged(int);
    void localHostLinkActivated(const QString&);

    void rulesWarningChanged(int);

  public:
    GroupManagerPage(CGroup*, QWidget *parent = 0);

  private:
    CGroup* m_groupManager;
};


#endif

