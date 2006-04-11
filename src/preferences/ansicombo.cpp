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


#include "ansicombo.h"
#include <QtGui>


AnsiCombo::AnsiCombo(QWidget *parent) : super(parent)
{
	setEditable(true);

	connect(this, SIGNAL(editTextChanged(const QString&)), 
				SLOT(afterEdit(const QString&)));
}
	
void AnsiCombo::fillAnsiList()
{
	AnsiItem item;
	clear();
		
	foreach(item, colours)
	{
		addItem(item.picture, item.ansiCode);
	}
}

QString AnsiCombo::text() const
{
	return super::currentText();
}
	
void AnsiCombo::setText(const QString& t)
{
	QString tmp = t;
	if (t == "255" || t == "254")
		tmp = "none";
	
	const int index = findText(tmp);

	if (index >= 0)
	{
		setCurrentIndex(index);
	}
	else
	{
		super::setEditText(tmp);
	}
}

void AnsiCombo::afterEdit(const QString& t)
{
	setText(t);
}

void AnsiCombo::initColours(AnsiMode mode)
{
	switch (mode)
	{
		case ANSI_FG:		
			colours.push_back(initAnsiItem(254));
			for (int i = 30; i < 38; ++i)
			{
				colours.push_back(initAnsiItem(i));
			}
			break;
		case ANSI_BG:
			colours.push_back(initAnsiItem(255));
			for (int i = 40; i < 48; ++i)
			{
				colours.push_back(initAnsiItem(i));
			}
			break;
	}
	fillAnsiList();
}

AnsiCombo::AnsiItem AnsiCombo::initAnsiItem(int index)
{
	QColor col;
	AnsiItem retVal;

	if (colorFromNumber(index, col, retVal.description))
	{
		QPixmap pix(20, 20);

		pix.fill(col);

		retVal.picture = pix;
		if (index != 254 && index != 255)
			retVal.ansiCode = QString("%1").arg(index);
		else
			retVal.ansiCode = QString("none");		
	}
	return retVal;
}

bool AnsiCombo::colorFromString(const QString& colString, 
								QColor& colFg, int& ansiCodeFg, QString& intelligibleNameFg, 
								QColor& colBg, int& ansiCodeBg, QString& intelligibleNameBg, 
								bool& bold, bool& underline)
{
	int tmpInt;
	
	ansiCodeFg = 254; intelligibleNameFg = "none"; colFg = QColor(Qt::white);
	ansiCodeBg = 255; intelligibleNameBg = "none"; colBg = QColor(Qt::black);
	bold = false;
	underline = false;				
	
	QRegExp re("^\\[((?:\\d+;)*\\d+)m$");

	if (re.indexIn(colString) >= 0)
	{  //matches
		QString tmpStr = colString;

		tmpStr.chop(1);
		tmpStr.remove(0,1);

		QStringList list = tmpStr.split(";", QString::SkipEmptyParts);
		
		switch (list.size())
		{
			case 4:
				if 	(list.at(3) == "1") bold = true;
				if 	(list.at(3) == "4") underline = true;				
			case 3:
				if 	(list.at(2) == "1") bold = true;
				if 	(list.at(2) == "4") underline = true;				
			case 2:
				tmpInt = list.at(1).toInt();
				if 	(tmpInt == 1) bold = true;
				if 	(tmpInt == 4) underline = true;				
				if (tmpInt >= 40 && tmpInt <=47)
				{
					ansiCodeBg = tmpInt;
					colorFromNumber(ansiCodeBg, colBg, intelligibleNameBg);
				}
				if (tmpInt >= 30 && tmpInt <=37)
				{
					ansiCodeFg = tmpInt;
					colorFromNumber(ansiCodeFg, colFg, intelligibleNameFg);
				}
			case 1:
				tmpInt = list.at(0).toInt();
				if 	(tmpInt == 1) bold = true;
				if 	(tmpInt == 4) underline = true;				
				if (tmpInt >= 40 && tmpInt <=47)
				{
					ansiCodeBg = tmpInt;
					colorFromNumber(ansiCodeBg, colBg, intelligibleNameBg);
				}
				if (tmpInt >= 30 && tmpInt <=37)
				{
					ansiCodeFg = tmpInt;
					colorFromNumber(ansiCodeFg, colFg, intelligibleNameFg);
				}
				break;
			default:
				break;
		}
		return true;
	}
	return false;
}

bool AnsiCombo::colorFromNumber(int numColor, QColor& col, QString& intelligibleName)
{
	intelligibleName = tr("undefined!"); 
	col = Qt::white;

	const bool retVal = ( (((numColor >= 30) && (numColor <= 37)) || ((numColor >= 40) && (numColor <= 47))) || numColor == 254 || numColor == 255);

	switch (numColor)
	{
	case 254: 
		col = Qt::white; 
		intelligibleName = tr("none");
		break;
	case 255: 
		col = Qt::black; 
		intelligibleName = tr("none");
		break;
	case 30: 
	case 40: 
		col = Qt::black; 
		intelligibleName = tr("black");
		break;
	case 31: 
	case 41: 
		col = Qt::red; 
		intelligibleName = tr("red");
		break;
	case 32: 
	case 42: 
		col = Qt::green; 
		intelligibleName = tr("green");
		break;
	case 33: 
	case 43: 
		col = Qt::yellow; 
		intelligibleName = tr("yellow");
		break;
	case 34: 
	case 44: 
		col = Qt::blue; 
		intelligibleName = tr("blue");
		break;
	case 35: 
	case 45: 
		col = Qt::magenta; 
		intelligibleName = tr("magenta");
		break;
	case 36: 
	case 46: 
		col = Qt::cyan; 
		intelligibleName = tr("cyan");
		break;
	case 37: 
	case 47: 
		col = Qt::white; 
		intelligibleName = tr("white");
		break;
	}
	return retVal;
}

void AnsiCombo::makeWidgetColoured(QWidget* pWidget, const QString& ansiColor)
{
	if (pWidget)
	{
		QColor colFg;
		QColor colBg;
		int ansiCodeFg;
		int ansiCodeBg;
		QString intelligibleNameFg;
		QString intelligibleNameBg;
		bool bold;
		bool underline;

		colorFromString(ansiColor, 
								colFg, ansiCodeFg, intelligibleNameFg, 
								colBg, ansiCodeBg, intelligibleNameBg, 
								bold, underline);

		QPalette palette = pWidget->palette();

		// crucial call to have background filled
		pWidget->setAutoFillBackground(true);

		palette.setColor(QPalette::WindowText, colFg);
		palette.setColor(QPalette::Window, colBg);

		pWidget->setPalette(palette);
		pWidget->setBackgroundRole(QPalette::Window);

		QLabel *pLabel = qobject_cast<QLabel *>(pWidget);
		if (pLabel)
		{
			pLabel->setText(intelligibleNameFg);
		}
	}
}
