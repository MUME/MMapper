/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
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


#include <QtGui>
#include "parserpage.h"
#include "configuration.h"
#include "defs.h"

ParserPage::ParserPage(QWidget *parent)
        : QWidget(parent)
{
    setupUi(this);

   	IACPromptCheckBox->setChecked(Config().m_IAC_prompt_parser);	

#ifdef ALLOW_XML_PARSER	
   	xmlParserCheckBox->setChecked(Config().m_useXmlParser);		
	xmlParserCheckBox->setEnabled(true);
#else
	Config().m_useXmlParser = false;
   	xmlParserCheckBox->setChecked(false);		
	xmlParserCheckBox->setEnabled(false);
#endif

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

	connect ( roomNameAnsiColor, SIGNAL(textChanged(const QString&)),
		SLOT(roomNameAnsiColorTextChanged(const QString&)));
	connect ( roomDescAnsiColor, SIGNAL(textChanged(const QString&)),
		SLOT(roomDescAnsiColorTextChanged(const QString&)));
	connect ( roomNameAnsiColor, SIGNAL(textChanged(const QString&)),
		SLOT(roomNameColorChanged(const QString&)));
	connect ( roomDescAnsiColor, SIGNAL(textChanged(const QString&)),
		SLOT(roomDescColorChanged(const QString&)));
	connect ( roomNameAnsiColor, SIGNAL(activated(const QString&)),
		SLOT(roomNameColorChanged(const QString&)));
	connect ( roomDescAnsiColor, SIGNAL(activated(const QString&)),
		SLOT(roomDescColorChanged(const QString&)));
	connect ( roomNameAnsiColor, SIGNAL(highlighted(const QString&)),
		SLOT(roomNameColorChanged(const QString&)));
	connect ( roomDescAnsiColor, SIGNAL(highlighted(const QString&)),
		SLOT(roomDescColorChanged(const QString&)));
		
	connect( IACPromptCheckBox, SIGNAL(stateChanged(int)),SLOT(IACPromptCheckBoxStateChanged(int)));	
	connect( xmlParserCheckBox, SIGNAL(stateChanged(int)),SLOT(xmlParserCheckBoxCheckBoxStateChanged(int)));	

	roomNameAnsiColor->setText( Config().m_roomNameColor );
	roomDescAnsiColor->setText( Config().m_roomDescColor );
}

void ParserPage::roomNameAnsiColorTextChanged(const QString& str)
{
	if (str.startsWith("[") && str.endsWith("m"))
		Config().m_roomNameColor = str;
}

void ParserPage::roomDescAnsiColorTextChanged(const QString& str)
{
	if (str.startsWith("[") && str.endsWith("m"))
		Config().m_roomDescColor = str;	
}

void ParserPage::xmlParserCheckBoxCheckBoxStateChanged(int)
{
	Config().m_useXmlParser = xmlParserCheckBox->isChecked();		
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
	bool matches = FALSE;
	QRegExp rx; 

	if ((pattern)[0] != '#') {
	}
	else{
		switch ((int)(pattern[1]).toAscii()){
		case 33:  // !
			rx.setPattern((pattern).remove(0,2));
			if(rx.exactMatch(str)) matches = TRUE;
			break;
		case 60:  // <
			if(str.startsWith((pattern).remove(0,2))) matches = TRUE;
			break;
		case 61:  // =
			if( str==((pattern).remove(0,2)) ) matches = TRUE;
			break;
		case 62:  // >
			if(str.endsWith((pattern).remove(0,2))) matches = TRUE;
			break;
		case 63:  // ?
			if(str.contains((pattern).remove(0,2))) matches = TRUE;
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

void ParserPage::roomDescColorChanged(const QString& s)
{
	comboChanged(s, labelRoomDesc);
}

void ParserPage::roomNameColorChanged(const QString& s)
{
	comboChanged(s, labelRoomColor);
}

void ParserPage::comboChanged(const QString& colString, QLabel *p)
{
	AnsiCombo::makeWidgetColoured(p, colString);
}
