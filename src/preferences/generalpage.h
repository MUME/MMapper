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

#ifndef GENERALPAGE_H
#define GENERALPAGE_H

#include <QWidget>
#include "ui_generalpage.h"

class GeneralPage : public QWidget, private Ui::GeneralPage
{
    Q_OBJECT

public slots:

	void remoteNameTextChanged(const QString&);
	void remotePortValueChanged(int);
	void localPortValueChanged(int);

	void acceptBestRelativeDoubleSpinBoxValueChanged(double);
	void acceptBestAbsoluteDoubleSpinBoxValueChanged(double);
	void newRoomPenaltyDoubleSpinBoxValueChanged(double);
	void correctPositionBonusDoubleSpinBoxValueChanged(double);
	void multipleConnectionsPenaltyDoubleSpinBoxValueChanged(double);
	void maxPathsValueChanged(int);
	void matchingToleranceSpinBoxValueChanged(int);

	void briefStateChanged(int);
	void emulatedExitsStateChanged(int);
	void updatedStateChanged(int);
	void drawNotMappedExitsStateChanged(int);
	void drawUpperLayersTexturedStateChanged(int);

	void autoLoadFileNameTextChanged(const QString&);
	void autoLoadCheckStateChanged(int);
	void logFileNameTextChanged(const QString&);
	void logCheckStateChanged(int);
	
	void sellectWorldFileButtonClicked();
	void sellectLogFileButtonClicked();
	

public:
    GeneralPage(QWidget *parent = 0);
};


#endif

