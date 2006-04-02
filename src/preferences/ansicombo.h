#ifndef ANSI_COMBO_H
#define ANSI_COMBO_H

#include<QComboBox>
#include<QVector>

class AnsiCombo : public QComboBox
{
	typedef QComboBox super;
	Q_OBJECT
public:
	static void makeWidgetColoured(QWidget*, const QString& ansiColor);

	AnsiCombo(QWidget *parent);

	/// populate the list with ANSI codes and coloured boxes
	void fillAnsiList();

	/// get currently selected ANSI code like [32m for green colour
	QString text() const;

	void setText(const QString&);

protected slots:
	void afterEdit(const QString&);

protected:
	///\return true if string is valid ANSI color code
	static bool colorFromString(const QString& colString, /* out */ QColor& col, 
		/* out */ QString& intelligibleName, /* out */ bool* bg = NULL);

	///\return true, if index is valid color code
	static bool colorFromNumber(int numColor, /* out */ QColor& col, 
	/* out */ QString& intelligibleName, /* out */ bool* bg = NULL);

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

	void initColours();
};

#endif
