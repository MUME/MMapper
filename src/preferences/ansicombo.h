/************************************************************************
**
** Authors:   Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
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

#ifndef ANSI_COMBO_H
#define ANSI_COMBO_H

#include<QComboBox>
#include<QVector>

enum AnsiMode {ANSI_FG, ANSI_BG};

class AnsiCombo : public QComboBox
{
	typedef QComboBox super;
	Q_OBJECT
public:
	static void makeWidgetColoured(QWidget*, const QString& ansiColor);

	AnsiCombo(QWidget *parent);

	void initColours(AnsiMode mode);
	/// populate the list with ANSI codes and coloured boxes
	void fillAnsiList();

	/// get currently selected ANSI code like [32m for green colour
	QString text() const;

	void setText(const QString&);

	///\return true if string is valid ANSI color code
	static bool colorFromString(const QString& colString, 
								QColor& colFg, int& ansiCodeFg, QString& intelligibleNameFg, 
								QColor& colBg, int& ansiCodeBg, QString& intelligibleNameBg, 
								bool& bold, bool& underline);

	///\return true, if index is valid color code
	static bool colorFromNumber(int numColor, QColor& col, QString& intelligibleName);

protected slots:
	void afterEdit(const QString&);

protected:

	class AnsiItem
	{
	public:
		QString ansiCode;
		QString description;
		QIcon picture;
	};
	typedef QVector<AnsiItem> AnsiItemVector;

	static AnsiItem initAnsiItem(int index);

	AnsiItemVector colours;

};

#endif
