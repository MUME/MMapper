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

#include "parserpage.h"
#include "configuration.h"
#include "defs.h"

#include <QMessageBox>

enum
{
    UiCharsetAsciiOrLatin1,
    UiCharsetUTF8
};

ParserPage::ParserPage(QWidget *parent)
        : QWidget(parent)
{
    setupUi(this);
    
    //Config().m_roomNameColor = "[36;45;4m";
    //Config().m_roomDescColor = "[32;41;1m";
    
    roomNameAnsiColor->initColours(ANSI_FG);
    roomNameAnsiColorBG->initColours(ANSI_BG);
	roomDescAnsiColor->initColours(ANSI_FG);
	roomDescAnsiColorBG->initColours(ANSI_BG);

	updateColors();

   	IACPromptCheckBox->setChecked(Config().m_IAC_prompt_parser);	
        mpiCheckBox->setChecked(Config().m_mpi);

        if (Config().m_utf8Charset)
            charset->setCurrentIndex(UiCharsetUTF8);
        else
            charset->setCurrentIndex(UiCharsetAsciiOrLatin1);

	suppressXmlTagsCheckBox->setChecked(Config().m_removeXmlTags);
	suppressXmlTagsCheckBox->setEnabled(true);

	forcePatternsList->clear();
	forcePatternsList->addItems( Config().m_moveForcePatternsList );
	cancelPatternsList->clear();
	cancelPatternsList->addItems( Config().m_moveCancelPatternsList );
	endDescPatternsList->clear();
	endDescPatternsList->addItems( Config().m_noDescriptionPatternsList );
    
    connect( removeCancelPattern, SIGNAL(clicked()),SLOT(removeCancelPatternClicked()));
	connect( removeForcePattern, SIGNAL(clicked()),SLOT(removeForcePatternClicked()));
	connect( removeEndDescPattern, SIGNAL(clicked()),SLOT(removeEndDescPatternClicked()));

	connect( addCancelPattern, SIGNAL(clicked()),SLOT(addCancelPatternClicked()));
	connect( addForcePattern, SIGNAL(clicked()),SLOT(addForcePatternClicked()));
	connect( addEndDescPattern, SIGNAL(clicked()),SLOT(addEndDescPatternClicked()));

	connect( testPattern, SIGNAL(clicked()),SLOT(testPatternClicked()));
	connect( validPattern, SIGNAL(clicked()),SLOT(validPatternClicked()));

	connect( forcePatternsList,SIGNAL(activated(const QString&)),
		SLOT(forcePatternsListActivated(const QString&)));
	connect( cancelPatternsList,SIGNAL(activated(const QString&)),
		SLOT(cancelPatternsListActivated(const QString&)));
	connect( endDescPatternsList,SIGNAL(activated(const QString&)),
		SLOT(endDescPatternsListActivated(const QString&)));

//	connect ( roomNameAnsiColor, SIGNAL(editTextChanged(const QString&)), 
//		SLOT(roomNameAnsiColorTextChanged(const QString&)));
//	connect ( roomDescAnsiColor, SIGNAL(editTextChanged(const QString&)), 
//		SLOT(roomDescAnsiColorTextChanged(const QString&)));
//	connect ( roomNameAnsiColor, SIGNAL(editTextChanged(const QString&)), 
//		SLOT(roomNameColorChanged(const QString&)));
//	connect ( roomDescAnsiColor, SIGNAL(editTextChanged(const QString&)), 
//		SLOT(roomDescColorChanged(const QString&)));
//	connect ( roomNameAnsiColor, SIGNAL(highlighted(const QString&)),
//		SLOT(roomNameColorChanged(const QString&)));
//	connect ( roomDescAnsiColor, SIGNAL(highlighted(const QString&)),
//		SLOT(roomDescColorChanged(const QString&)));
	connect ( roomNameAnsiColor, SIGNAL(activated(const QString&)), 
		SLOT(roomNameColorChanged(const QString&)));
	connect ( roomDescAnsiColor, SIGNAL(activated(const QString&)), 
		SLOT(roomDescColorChanged(const QString&)));

	connect ( roomNameAnsiColorBG, SIGNAL(activated(const QString&)), 
		SLOT(roomNameColorBGChanged(const QString&)));
	connect ( roomDescAnsiColorBG, SIGNAL(activated(const QString&)), 
		SLOT(roomDescColorBGChanged(const QString&)));
		
	connect (roomNameAnsiBold	,SIGNAL(toggled(bool)), this, SLOT(anyColorToggleButtonToggled(bool)) );	
	connect (roomNameAnsiUnderline	,SIGNAL(toggled(bool)), this, SLOT(anyColorToggleButtonToggled(bool)) );	
	connect (roomDescAnsiBold	,SIGNAL(toggled(bool)), this, SLOT(anyColorToggleButtonToggled(bool)) );	
	connect (roomDescAnsiUnderline	,SIGNAL(toggled(bool)), this, SLOT(anyColorToggleButtonToggled(bool)) );	
		
	connect( IACPromptCheckBox, SIGNAL(stateChanged(int)),SLOT(IACPromptCheckBoxStateChanged(int)));	
	connect( suppressXmlTagsCheckBox, SIGNAL(stateChanged(int)),SLOT(suppressXmlTagsCheckBoxStateChanged(int)));	
        connect( mpiCheckBox, SIGNAL(stateChanged(int)),SLOT(mpiCheckBoxStateChanged(int)));
        connect( charset, SIGNAL(currentIndexChanged(int)),SLOT(charsetChanged(int)));


}

void ParserPage::charsetChanged(int index)
{
        Config().m_utf8Charset = (index == UiCharsetUTF8);
}

void ParserPage::mpiCheckBoxStateChanged(int)
{
        Config().m_mpi = mpiCheckBox->isChecked();
}


void ParserPage::suppressXmlTagsCheckBoxStateChanged(int)
{
	Config().m_removeXmlTags = suppressXmlTagsCheckBox->isChecked();		
}

void ParserPage::IACPromptCheckBoxStateChanged(int)
{
	Config().m_IAC_prompt_parser = IACPromptCheckBox->isChecked();	
}


void ParserPage::savePatterns()
{
	Config().m_moveForcePatternsList.clear();
	for ( int i=0; i<forcePatternsList->count(); i++)
		Config().m_moveForcePatternsList.append( forcePatternsList->itemText(i) );

	Config().m_moveCancelPatternsList.clear();
	for ( int i=0; i<cancelPatternsList->count(); i++)
		Config().m_moveCancelPatternsList.append( cancelPatternsList->itemText(i) );

	Config().m_noDescriptionPatternsList.clear();
	for ( int i=0; i<endDescPatternsList->count(); i++)
		Config().m_noDescriptionPatternsList.append( endDescPatternsList->itemText(i) );
}

void ParserPage::removeForcePatternClicked(){
	forcePatternsList->removeItem(forcePatternsList->currentIndex());
	savePatterns();
}

void ParserPage::removeCancelPatternClicked(){
	cancelPatternsList->removeItem(cancelPatternsList->currentIndex());
	savePatterns();
}

void ParserPage::removeEndDescPatternClicked(){
	endDescPatternsList->removeItem(endDescPatternsList->currentIndex());
	savePatterns();
}

void ParserPage::testPatternClicked(){
	QString pattern=newPattern->text();
	QString str=testString->text();
	bool matches = false;
	QRegExp rx; 

	if ((pattern)[0] != '#') {
	}
	else{
        switch ((int)(pattern[1]).toLatin1()){
		case 33:  // !
			rx.setPattern((pattern).remove(0,2));
			if(rx.exactMatch(str)) matches = true;
			break;
		case 60:  // <
			if(str.startsWith((pattern).remove(0,2))) matches = true;
			break;
		case 61:  // =
			if( str==((pattern).remove(0,2)) ) matches = true;
			break;
		case 62:  // >
			if(str.endsWith((pattern).remove(0,2))) matches = true;
			break;
		case 63:  // ?
			if(str.contains((pattern).remove(0,2))) matches = true;
			break;
		default:;
		}
	}

	str= (matches) ? tr("Pattern matches!!!") : tr("Pattern doesn't match!!!");	

	QMessageBox::information( this, tr("Pattern match test"), str);
}

void ParserPage::validPatternClicked(){

	QString pattern=newPattern->text();    
	QString str = "Pattern '"+pattern+"' is valid!!!";

	if (((pattern)[0] != '#') || (((pattern)[1] != '!') && ((pattern)[1] != '?') && ((pattern)[1] != '<') && ((pattern)[1] != '>') && ((pattern)[1] != '=')) ) {
		str = "Pattern must begin with '#t', where t means type of pattern (!?<>=)";	    
	}else
		if ((pattern)[1] == '!'){
			QRegExp rx(pattern.remove(0,2));  
			if (!rx.isValid())
				str="Pattern '"+pattern+"' is not valid!!!";	
		}

		QMessageBox::information( this, "Pattern match test",
			str ,
			"&Discard",
			0,      // Enter == button 0
			0 );    
}

void ParserPage::forcePatternsListActivated(const QString& str){   
	newPattern->setText(str);
}

void ParserPage::cancelPatternsListActivated(const QString& str){
	newPattern->setText(str);
}

void ParserPage::endDescPatternsListActivated(const QString& str){
	newPattern->setText(str);
}

void ParserPage::addForcePatternClicked(){
	QString str;
	if((str=newPattern->text())!="") {
		forcePatternsList->addItem(str);
		forcePatternsList->setCurrentIndex(forcePatternsList->count()-1);
	}
	savePatterns();
}

void ParserPage::addCancelPatternClicked(){
	QString str;
	if((str=newPattern->text())!=""){
		cancelPatternsList->addItem(str);
		cancelPatternsList->setCurrentIndex(cancelPatternsList->count()-1);
	}
	savePatterns();
}

void ParserPage::addEndDescPatternClicked(){
	QString str;
	if((str=newPattern->text())!=""){
		endDescPatternsList->addItem(str);
		endDescPatternsList->setCurrentIndex(endDescPatternsList->count()-1);
	}  
	savePatterns();
}


void ParserPage::updateColors()
{
	QColor colFg;
	QColor colBg;
	int ansiCodeFg;
	int ansiCodeBg;
	QString intelligibleNameFg;
	QString intelligibleNameBg;
	bool bold;
	bool underline;

    
    // room name color
	if (Config().m_roomNameColor.isEmpty())
		roomNameColorLabel->setText("ERROR - no color!!!");
	else
	    roomNameColorLabel->setText(Config().m_roomNameColor);
    
	AnsiCombo::makeWidgetColoured(labelRoomColor, Config().m_roomNameColor);

	AnsiCombo::colorFromString(Config().m_roomNameColor, 
								colFg, ansiCodeFg, intelligibleNameFg, 
								colBg, ansiCodeBg, intelligibleNameBg, 
								bold, underline);

	roomNameAnsiBold->setChecked(bold);
	roomNameAnsiUnderline->setChecked(underline);
	
	roomNameAnsiColor->setEditable(false);
	roomNameAnsiColorBG->setEditable(false);
	roomNameAnsiColor->setText( QString("%1").arg(ansiCodeFg) );
	roomNameAnsiColorBG->setText( QString("%1").arg(ansiCodeBg) );


    // room desc color
	if (Config().m_roomDescColor.isEmpty())
		roomDescColorLabel->setText("ERROR - no color!!!");		
	else
	    roomDescColorLabel->setText(Config().m_roomDescColor);
    
	AnsiCombo::makeWidgetColoured(labelRoomDesc, Config().m_roomDescColor);

	AnsiCombo::colorFromString(Config().m_roomDescColor, 
								colFg, ansiCodeFg, intelligibleNameFg, 
								colBg, ansiCodeBg, intelligibleNameBg, 
								bold, underline);
	
	roomDescAnsiBold->setChecked(bold);
	roomDescAnsiUnderline->setChecked(underline);
	
	roomDescAnsiColor->setEditable(false);
	roomDescAnsiColorBG->setEditable(false);
	roomDescAnsiColor->setText( QString("%1").arg(ansiCodeFg) );
	roomDescAnsiColorBG->setText( QString("%1").arg(ansiCodeBg) );
		
}


void ParserPage::generateNewAnsiColor()
{
	QString result = "";	
	if (roomNameAnsiColor->text() != "none") result.append(roomNameAnsiColor->text());
	if (roomNameAnsiColorBG->text() != "none") result.append(";"+roomNameAnsiColorBG->text());
	if (roomNameAnsiBold->isChecked()) result.append(";1");
	if (roomNameAnsiUnderline->isChecked()) result.append(";4");

	if (result != "") 
	{
		if (result.startsWith(';')) result.remove(0,1);
		result.prepend("[");
		result.append("m");
	}

    Config().m_roomNameColor = result;
	
	result = "";	
	if (roomDescAnsiColor->text() != "none") result.append(roomDescAnsiColor->text());
	if (roomDescAnsiColorBG->text() != "none") result.append(";"+roomDescAnsiColorBG->text());
	if (roomDescAnsiBold->isChecked()) result.append(";1");
	if (roomDescAnsiUnderline->isChecked()) result.append(";4");

	if (result != "") 
	{
		if (result.startsWith(';')) result.remove(0,1);
		result.prepend("[");
		result.append("m");
	}

    Config().m_roomDescColor = result;
}


void ParserPage::roomDescColorChanged(const QString&)
{
	generateNewAnsiColor();
	updateColors();
}

void ParserPage::roomNameColorChanged(const QString&)
{
	generateNewAnsiColor();
	updateColors();
}

void ParserPage::roomDescColorBGChanged(const QString&)
{
	generateNewAnsiColor();
	updateColors();
}

void ParserPage::roomNameColorBGChanged(const QString&)
{
	generateNewAnsiColor();
	updateColors();
}

void ParserPage::anyColorToggleButtonToggled(bool)
{
	generateNewAnsiColor();
	updateColors();
}

