/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDockWidget>
#include <QStackedWidget>
#include <QMainWindow>
#include <QTextBrowser>
#include <QProgressDialog>
#include <QFileDialog>
#include <QPointer>
#include <QActionGroup>

class QMenu;
class MapWindow;
class MainWindowCtrl;
class Mmapper2PathMachine;
class CommandEvaluator;
class PrespammedPath;
class MapData;
class RoomSelection;
class ConnectionSelection;
class RoomPropertySetter;
class FindRoomsDlg;
class CGroup;
class CGroupCommunicator;

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
  CGroup *getGroupManager() {return m_groupManager;};

  void loadFile(const QString &fileName);
  bool saveFile(const QString &fileName, bool baseMapOnly );
  void setCurrentFile(const QString &fileName);

public slots:
  void newFile();
  void open();
  void reload();
  void merge();
  bool save();
  bool saveAs();
  bool exportBaseMap();
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
  void onConnectToNeighboursRoomSelection();
  void onFindRoom();
  void onPreferences();
  void onPlayMode();
  void onMapMode();
  void onOfflineMode();
  void alwaysOnTop();

  void newRoomSelection(const RoomSelection*);
  void newConnectionSelection(ConnectionSelection*);

  void groupOff(bool);
  void groupClient(bool);
  void groupServer(bool);
  void groupManagerTypeChanged(int);

  void openMmapperHomepage();
  void openMumeWebsite();
  void openMumeForum();
  void openMumeWiki();
  void openSettingUpMmapper();
  void openNewbieHelp();

protected:
  void closeEvent(QCloseEvent *event);

private:
  StackedWidget *m_stackedWidget;
  QTextBrowser   *logWindow;
  DockWidget *m_dockDialogLog;
  DockWidget *m_dockDialogGroup;

  Mmapper2PathMachine *m_pathMachine;
  MapData *m_mapData;
  RoomPropertySetter * m_propertySetter;
  CommandEvaluator *m_commandEvaluator;
  PrespammedPath *m_prespammedPath;

  // Pandora Ported
  FindRoomsDlg *m_findRoomsDlg;
  CGroup *m_groupManager;
  CGroupCommunicator *m_groupCommunicator;

  const RoomSelection* m_roomSelection;
  ConnectionSelection* m_connectionSelection;

  QProgressDialog *progressDlg;

  QToolBar *fileToolBar;
  QToolBar *editToolBar;
  QToolBar *modeToolBar;
  QToolBar *mapModeToolBar;
  QToolBar *viewToolBar;
  QToolBar *pathMachineToolBar;
  QToolBar *roomToolBar;
  QToolBar *connectionToolBar;
  QToolBar *groupToolBar;
  QToolBar *settingsToolBar;

  QMenu *fileMenu;
  QMenu *editMenu;
  QMenu *roomMenu;
  QMenu *connectionMenu;
  QMenu *viewMenu;
  QMenu *searchMenu;
  QMenu *settingsMenu;
  QMenu *helpMenu;
  QMenu *mumeMenu;
  QMenu *onlineTutorialsMenu;
  QMenu *groupMenu;

  QAction               *groupOffAct;
  QAction               *groupClientAct;
  QAction               *groupServerAct;
  QAction               *groupShowHideAct;
  QAction               *groupSettingsAct;

  QAction *newAct;
  QAction *openAct;
  QAction *mergeAct;
  QAction *reloadAct;
  QAction *saveAct;
  QAction *saveAsAct;
  QAction *exportBaseMapAct;
  QAction *exitAct;
  QAction *cutAct;
  QAction *copyAct;
  QAction *pasteAct;
  QAction *mmapperHomePageAct;
  QAction *mumeWebsiteAct;
  QAction *mumeForumAct;
  QAction *mumeWikiAct;
  QAction *settingUpMmapperAct;
  QAction *newbieAct;
  QAction *aboutAct;
  QAction *aboutQtAct;
  QAction *prevWindowAct;
  QAction *nextWindowAct;
  QAction *zoomInAct;
  QAction *zoomOutAct;
  QAction *alwaysOnTopAct;
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

  QAction *playModeAct;
  QAction *mapModeAct;
  QAction *offlineModeAct;

  QActionGroup *mapModeActGroup;
  QActionGroup *modeActGroup;
  QActionGroup *roomActGroup;
  QActionGroup *connectionActGroup;
  QActionGroup *groupManagerGroup;

  QAction *editRoomSelectionAct;
  QAction *editConnectionSelectionAct;
  QAction *deleteRoomSelectionAct;
  QAction *deleteConnectionSelectionAct;

  QAction *moveUpRoomSelectionAct;
  QAction *moveDownRoomSelectionAct;
  QAction *mergeUpRoomSelectionAct;
  QAction *mergeDownRoomSelectionAct;
  QAction *connectToNeighboursRoomSelectionAct;

  QAction *findRoomsAct;

  QAction *forceRoomAct;
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
  QPointer<QFileDialog> defaultSaveDialog();
};

#endif
