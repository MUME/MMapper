#include "ansicombo.h"
#include <QLabel>

AnsiCombo::AnsiCombo(QWidget *parent) : super(parent)
{
	setEditable(true);
	initColours();
	fillAnsiList();

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
	const int index = findText(t);

	if (index >= 0)
	{
		setCurrentIndex(index);
	}
	else
	{
		super::setEditText(t);
	}
}

void AnsiCombo::afterEdit(const QString& t)
{
	setText(t);
}

void AnsiCombo::initColours()
{
	for (int i = 30; i < 38; ++i)
	{
		colours.push_back(initAnsiItem(i));
	}
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
		retVal.ansiCode = QString("[%1m").arg(index);
	}
	return retVal;
}

bool AnsiCombo::colorFromString(const QString& colString, 
								QColor& col, QString& intelligibleName, 
								bool* bg)
{
	QRegExp re(".*\\[(\\d+)m");

	if (re.indexIn(colString) >= 0)
	{
		return colorFromNumber(re.cap(1).toInt(), col, intelligibleName, bg);

	}
	return false;
}

bool AnsiCombo::colorFromNumber(int numColor, /* out */ QColor& col, 
							/* out */ QString& intelligibleName, /* out */ bool* bg)
{
	intelligibleName.clear(); col = Qt::white;

	if (bg)
	{
		*bg = (numColor >= 40);
	}
	if (numColor >= 40) // convert bg codes to foreground
	{
		numColor -= 10;
	}

	const bool retVal = (numColor >= 30) && (numColor <= 37);

	switch (numColor)
	{
	case 30: 
		col = Qt::black; 
		intelligibleName = tr("black");
		break;
	case 31: 
		col = Qt::red; 
		intelligibleName = tr("red");
		break;
	case 32: 
		col = Qt::green; 
		intelligibleName = tr("green");
		break;
	case 33: 
		col = Qt::yellow; 
		intelligibleName = tr("yellow");
		break;
	case 34: 
		col = Qt::blue; 
		intelligibleName = tr("blue");
		break;
	case 35: 
		col = Qt::magenta; 
		intelligibleName = tr("magenta");
		break;
	case 36: 
		col = Qt::cyan; 
		intelligibleName = tr("cyan");
		break;
	case 37: 
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
		bool background;
		QColor col;
		QString name;

		if (colorFromString(ansiColor, col, name, &background))
		{
			QPalette palette = pWidget->palette();

			// crucial call to have background filled
			pWidget->setAutoFillBackground(true);

			if (background)
			{
				pWidget->setBackgroundRole(QPalette::Window);

				palette.setColor(QPalette::WindowText, Qt::black);
				palette.setColor(QPalette::Window, col);
			}
			else
			{
				palette.setColor(QPalette::WindowText, col);
				palette.setColor(QPalette::Window, Qt::black);
			}

			pWidget->setPalette(palette);
			pWidget->setBackgroundRole(QPalette::Window);

			QLabel *pLabel = qobject_cast<QLabel *>(pWidget);
			if (pLabel)
			{
				pLabel->setText(name);
			}
		}
		else
		{ // reset palette
			if (pWidget->parentWidget())
			{
				pWidget->setPalette(pWidget->parentWidget()->palette());
			}
		}
	}
}
