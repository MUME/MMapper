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

#ifndef PARSERPAGE_H
#define PARSERPAGE_H

#include <QWidget>
#include "ui_parserpage.h"

class ParserPage : public QWidget, private Ui::ParserPage
{
    Q_OBJECT

public slots:

    void removeForcePatternClicked();
    void removeCancelPatternClicked();
    void removeEndDescPatternClicked();
    void addForcePatternClicked();
    void addCancelPatternClicked();
    void addEndDescPatternClicked();
    void testPatternClicked();
    void validPatternClicked();
    void forcePatternsListActivated(const QString&);
    void cancelPatternsListActivated(const QString&);
    void endDescPatternsListActivated(const QString&);
    
	void anyColorToggleButtonToggled(bool);
	
	void IACPromptCheckBoxStateChanged(int);
	void suppressXmlTagsCheckBoxStateChanged(int);
        void mpiCheckBoxStateChanged(int);

public:
    ParserPage(QWidget *parent = 0);

protected slots:
	void roomDescColorChanged(const QString&);
	void roomNameColorChanged(const QString&);
	void roomDescColorBGChanged(const QString&);
	void roomNameColorBGChanged(const QString&);

private:
	void generateNewAnsiColor();
	void updateColors();
	
    void savePatterns(); 
};


#endif

