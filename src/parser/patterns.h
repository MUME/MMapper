/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara),
**            Anonymous <lachupe@gmail.com> (Azazello)
**
** This file is part of the MMapper project. 
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
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

#ifndef _PATTERNS_H_
#define _PATTERNS_H_

#include <QtGui>
#include <QtCore>

class Patterns {
	
    static QRegExp m_rx;
    
public:
	static bool matchMoveCancelPatterns(QString&);
	static bool matchMoveForcePatterns(QString&);
	static bool matchNoDescriptionPatterns(QString&);
	static bool matchDynamicDescriptionPatterns(QString&);
	static bool matchPasswordPatterns(QByteArray&);
	static bool matchExitsPatterns(QString&);
    static bool matchScoutPatterns(QString&);
    static bool matchPromptPatterns(QByteArray&);
    static bool matchLoginPatterns(QByteArray&);
    static bool matchMenuPromptPatterns(QByteArray&);

    static bool matchPattern(QString pattern, QString& str);
    static bool matchPattern(QByteArray pattern, QByteArray& str);
};

#endif
