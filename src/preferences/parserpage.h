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

