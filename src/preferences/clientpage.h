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

#ifndef CLIENTPAGE_H
#define CLIENTPAGE_H

#include <QString>
#include <QWidget>
#include <QtCore>

class QObject;

namespace Ui {
class ClientPage;
}

class ClientPage : public QWidget
{
    Q_OBJECT

public:
    explicit ClientPage(QWidget *parent = nullptr);
    ~ClientPage();

    void updateFontAndColors();

public slots:
    void onChangeFont();
    void onChangeBackgroundColor();
    void onChangeForegroundColor();
    void onChangeColumns(int);
    void onChangeRows(int);
    void onChangeLinesOfScrollback(int);
    void onChangeLinesOfInputHistory(int);
    void onChangeTabCompletionDictionarySize(int);

signals:

private:
    Ui::ClientPage *ui = nullptr;
};

#endif // CLIENTPAGE_H
