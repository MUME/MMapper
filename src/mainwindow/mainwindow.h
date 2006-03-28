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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>

class QMenu;
class MapWindow;
class MainWindowCtrl;
class Mmapper2PathMachine;
class CommandEvaluator;
class PrespammedPath;
class MapData;
class RoomSelection;
class ConnectionSelection;	
	
class DockWidget : public QDockWidget
{
    Q_OBJECT
public:
	DockWidget ( const QString & title, QWidget * parent = 0, Qt::WFlags flags = 0 );

	virtual QSize minimumSizeHint() const;
	virtual QSize sizeHint() const;
	
};

class StackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
	StackedWidget ( QWidget * parent = 0 );

	virtual QSize minimumSizeHint() const;	
	virtual QSize sizeHint() const;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~MainWindow();

    MapWindow *getCurrentMapWindow();
    
    Mmapper2PathMachine *getPathMachine(){return m_pathMachine;};

    void loadFile(const QString &fileName);
    bool saveFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);

public slots:
    void newFile();
    void open();
    bool save();
    bool saveAs();
    void about();

    void nextWindow();
    void prevWindow();
    
    void percentageChanged(quint32);
    
    void log(const QString&, const QString&);
    
    void currentMapWindowChanged();
    
    void onModeConnectionSelect();
    void onModeRoomSelect();
    void onModeMoveSelect();
    void onModeInfoMarkEdit();
    void onModeCreateRoomSelect();
    void onModeCreateConnectionSelect();
    void onModeCreateOnewayConnectionSelect();
    void onLayerUp();
    void onLayerDown();
    void onEditRoomSelection();
    void onEditConnectionSelection();
    void onDeleteRoomSelection();
    void onDeleteConnectionSelection();
    void onMoveUpRoomSelection();
    void onMoveDownRoomSelection();
    void onMergeUpRoomSelection();
    void onMergeDownRoomSelection();
 	void onPreferences();
    
    void newRoomSelection(const RoomSelection*);
	void newConnectionSelection(ConnectionSelection*);
    
protected:
    void closeEvent(QCloseEvent *event);

private:
    StackedWidget *m_stackedWidget;
    QTextBrowser   *logWindow;
    DockWidget *m_dockDialog;
    
    Mmapper2PathMachine *m_pathMachine;
    MapData *m_mapData;
	CommandEvaluator *m_commandEvaluator;
    PrespammedPath *m_prespammedPath;
    
	const RoomSelection* m_roomSelection;
	ConnectionSelection* m_connectionSelection;

    QProgressDialog *progressDlg;

    QToolBar *fileToolBar;
    QToolBar *editToolBar;
    QToolBar *modeToolBar;
    QToolBar *viewToolBar;
    QToolBar *roomToolBar;
    QToolBar *connectionToolBar;
    QToolBar *settingsToolBar;

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *roomMenu;
    QMenu *connectionMenu;    
    QMenu *viewMenu;
    QMenu *searchMenu;
    QMenu *settingsMenu;
    QMenu *helpMenu;

    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *prevWindowAct;
    QAction *nextWindowAct;
    QAction *zoomInAct;
    QAction *zoomOutAct;
    QAction *preferencesAct;

    QAction *layerUpAct;
    QAction *layerDownAct;

    QAction *modeConnectionSelectAct;
    QAction *modeRoomSelectAct;
    QAction *modeMoveSelectAct;
    QAction *modeInfoMarkEditAct;

    QAction *createRoomAct;
	QAction *createConnectionAct;
    QAction *createOnewayConnectionAct;

	QActionGroup *modeActGroup;
	QActionGroup *roomActGroup;
	QActionGroup *connectionActGroup;
	
	QAction *editRoomSelectionAct;
	QAction *editConnectionSelectionAct;	
    QAction *deleteRoomSelectionAct;
    QAction *deleteConnectionSelectionAct;

    QAction *moveUpRoomSelectionAct;
    QAction *moveDownRoomSelectionAct;
    QAction *mergeUpRoomSelectionAct;
    QAction *mergeDownRoomSelectionAct;
	
	QAction *releaseAllPathsAct;
	
    void createActions();
    void disableActions(bool value);
    void setupMenuBar();
    void setupToolBars();
    void setupStatusBar();

    void readSettings();
    void writeSettings();

    QString strippedName(const QString &fullFileName);
    bool maybeSave();
};

#endif
