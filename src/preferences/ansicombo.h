/************************************************************************
**
** Authors:   Jan 'Kovis' Struhar <kovis@sourceforge.net> (Kovis)
** 			  Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distributione
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.  
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/

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
