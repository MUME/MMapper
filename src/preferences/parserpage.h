#pragma once
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

#include <QString>
#include <QWidget>
#include <QtCore>

#include "ui_parserpage.h"

class QObject;

enum class UiCharset { AsciiOrLatin1, UTF8 };

class ParserPage : public QWidget, private Ui::ParserPage
{
    Q_OBJECT

public slots:
    void roomNameColorClicked();
    void roomDescColorClicked();
    void removeEndDescPatternClicked();
    void addEndDescPatternClicked();
    void testPatternClicked();
    void validPatternClicked();
    void endDescPatternsListActivated(const QString &);
    void suppressXmlTagsCheckBoxStateChanged(int);

public:
    explicit ParserPage(QWidget *parent = nullptr);

private:
    void savePatterns();
};
