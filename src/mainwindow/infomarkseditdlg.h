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

#ifndef INFOMARKSEDITDLG_H
#define INFOMARKSEDITDLG_H

#include <QDialog>
#include "ui_infomarkseditdlg.h"
#include "infomark.h"
#include "coordinate.h"

class MapData;
class MapCanvas;

class InfoMarksEditDlg : public QDialog, private Ui::InfoMarksEditDlg
{
    Q_OBJECT

signals:
	void mapChanged();
	void closeEventReceived();

public slots:
	void objectListCurrentIndexChanged(const QString&);
	void objectTypeCurrentIndexChanged(const QString&);
	 
	void objectNameTextChanged(QString);
	void objectTextChanged(QString);
	void x1ValueChanged(double);
	void y1ValueChanged(double);
	void x2ValueChanged(double);
	void y2ValueChanged(double);
	void layerValueChanged(int);
	void createClicked();
	void modifyClicked();
	void deleteClicked();	
	void onMoveNorthClicked();
	void onMoveSouthClicked();
	void onMoveEastClicked();
	void onMoveWestClicked();
	void onMoveUpClicked();
	void onMoveDownClicked();
	void onDeleteAllClicked();
	
	 
public:
    InfoMarksEditDlg(MapData* mapData, QWidget *parent = 0);
    ~InfoMarksEditDlg();
    
    void setPoints(double x1, double y1, double x2, double y2, int layer);
    
    void readSettings();
	void writeSettings();
	
protected:
	void closeEvent(QCloseEvent*);

    
private:

	MapData* m_mapData;

	double m_selX1, m_selY1, m_selX2, m_selY2;
	int m_selLayer;

	void connectAll();
	void disconnectAll();

	InfoMarkType getType();
	InfoMark* getInfoMark(QString name);
	InfoMark* getCurrentInfoMark();
	void setCurrentInfoMark(InfoMark* m);

	void updateMarkers();
	void updateDialog();
};


#endif
