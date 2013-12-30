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

#include "mainwindow.h"
#include "mapwindow.h"
#include "mapcanvas.h"
#include "mapdata.h"
#include "roomeditattrdlg.h"
#include "mapstorage.h"
#include "connectionlistener.h"
#include "configuration.h"
#include "mmapper2pathmachine.h"
#include "roomselection.h"
#include "connectionselection.h"
#include "configdialog.h"
#include "customaction.h"
#include "prespammedpath.h"
#include "roompropertysetter.h"
#include "aboutdialog.h"
#include "findroomsdlg.h"
#include "CGroup.h"
#include "CGroupCommunicator.h"
#include "progresscounter.h"
#include "mapstorage/filesaver.h"

#include <QMessageBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QDesktopServices>
#include <QCloseEvent>

#include <memory>

DockWidget::DockWidget ( const QString & title, QWidget * parent, Qt::WFlags flags )
    :QDockWidget(title, parent, flags)
{}

StackedWidget::StackedWidget ( QWidget * parent )
    :QStackedWidget(parent)
{}

QSize StackedWidget::minimumSizeHint() const
{
  return QSize(200, 200);
};

QSize StackedWidget::sizeHint() const
{
  return QSize(500, 800);
};

QSize DockWidget::minimumSizeHint() const
{
  return QSize(200, 0);
};

QSize DockWidget::sizeHint() const
{
  return QSize(500, 130);
};

MainWindow::MainWindow(QWidget *parent, Qt::WFlags flags)
    : QMainWindow(parent, flags)
{
  setObjectName("MainWindow");
  setWindowTitle("MMapper: MUME Mapper");
  setWindowIcon(QIcon(":/icons/m.png"));

  qRegisterMetaType<IncomingData>("IncomingData");
  qRegisterMetaType<CommandQueue>("CommandQueue");
  qRegisterMetaType<DirectionType>("DirectionType");
  qRegisterMetaType<DoorActionType>("DoorActionType");

  m_roomSelection = NULL;
  m_connectionSelection = NULL;

  m_mapData = new MapData();
  m_mapData->setObjectName("MapData");
  m_mapData->start();

  m_prespammedPath = new PrespammedPath(this);
  m_groupManager = new CGroup(Config().m_groupManagerCharName, m_mapData, this);
  m_groupManager->setObjectName("GroupManager");

  m_stackedWidget = new StackedWidget();
  m_stackedWidget->setObjectName("StackedWidget");
  m_stackedWidget->addWidget(new MapWindow(m_mapData, m_prespammedPath, m_groupManager));
  setCentralWidget(m_stackedWidget);

  m_pathMachine = new Mmapper2PathMachine();
  m_pathMachine->setObjectName("Mmapper2PathMachine");
  m_pathMachine->start();

  m_propertySetter = new RoomPropertySetter();
  m_propertySetter->setObjectName("RoomPropertySetter");
  m_propertySetter->start();

  m_dockDialogLog = new DockWidget(tr("Log View"), this);
  m_dockDialogLog->setObjectName("DockWidgetLog");
  m_dockDialogLog->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
  m_dockDialogLog->setFeatures(QDockWidget::AllDockWidgetFeatures);
  addDockWidget(Qt::BottomDockWidgetArea, m_dockDialogLog);

  logWindow = new QTextBrowser(m_dockDialogLog);
  logWindow->setObjectName("LogWindow");
  m_dockDialogLog->setWidget(logWindow);

  m_dockDialogGroup = new DockWidget(tr("Group Manager View"), this);
  m_dockDialogGroup->setObjectName("DockWidgetGroup");
  m_dockDialogGroup->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
  m_dockDialogGroup->setFeatures(QDockWidget::AllDockWidgetFeatures);
  addDockWidget(Qt::TopDockWidgetArea, m_dockDialogGroup);
  m_dockDialogGroup->setWidget(m_groupManager);

  m_groupCommunicator = m_groupManager->getCommunicator();
  m_groupCommunicator->setObjectName("CCommunicator");
  m_findRoomsDlg = new FindRoomsDlg(m_mapData, this);
  m_findRoomsDlg->setObjectName("FindRoomsDlg");

  createActions();
  setupMenuBar();
  setupToolBars();
  setupStatusBar();

  setCorner(Qt::TopLeftCorner, Qt::TopDockWidgetArea);
  setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
  setCorner(Qt::TopRightCorner, Qt::TopDockWidgetArea);
  setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);


  ConnectionListener* server = new ConnectionListener(m_mapData, m_pathMachine, m_commandEvaluator, m_prespammedPath, m_groupManager, this);
  server->setMaxPendingConnections (1);
  server->setRemoteHost(Config().m_remoteServerName);
  server->setRemotePort(Config().m_remotePort);


  if (!server->listen(QHostAddress::Any, Config().m_localPort))
  {
    QMessageBox::critical(this, tr("MMapper2"),
                          tr("Unable to start the server (switching to offline mode): %1.")
                          .arg(server->errorString()));
  }
  else
  {
    log("ConnectionListener", tr("Server bound on localhost to port: %2.").arg(Config().m_localPort));
  }


  //update connections
  currentMapWindowChanged();
  readSettings();

  if (Config().m_mapMode == 0)
  {
    playModeAct->setChecked(true);
    onPlayMode();
  }
  else
    if (Config().m_mapMode == 1)
    {
      mapModeAct->setChecked(true);
      onMapMode();
    }
  else
    if (Config().m_mapMode == 2)
    {
      offlineModeAct->setChecked(true);
      onOfflineMode();
    }

}

MainWindow::~MainWindow()
{}

void MainWindow::readSettings()
{
  resize(Config().windowSize);
  move(Config().windowPosition);
  restoreState(Config().windowState);
  alwaysOnTopAct->setChecked(Config().alwaysOnTop);
  if (Config().alwaysOnTop) {
    alwaysOnTop();
  }
  else
  {
    if (pos().x() < 0) pos().setX(0);
    if (pos().y() < 0) pos().setY(0);
    move(pos());
  }
}

void MainWindow::writeSettings()
{
  Config().setWindowPosition(pos() );
  Config().setWindowSize(size() );
  Config().setWindowState(saveState() );
  Config().setAlwaysOnTop((bool)(windowFlags() & Qt::WindowStaysOnTopHint));
}


void MainWindow::currentMapWindowChanged()
{
  //m_pathMachine->disconnect();
  QObject::connect(m_pathMachine, SIGNAL(log( const QString&, const QString& )), this, SLOT(log( const QString&, const QString& )));

  //handled by: onMapMode, onPlayMode
  //QObject::connect(m_pathMachine, SIGNAL(createRoom(ParseEvent*, const Coordinate & )), m_mapData, SLOT(createRoom(ParseEvent*, const Coordinate & )));
  //QObject::connect(m_pathMachine, SIGNAL(scheduleAction(MapAction *)), m_mapData, SLOT(scheduleAction(MapAction *)));

  QObject::connect(m_pathMachine, SIGNAL(lookingForRooms(RoomRecipient*, const Coordinate & )), m_mapData, SLOT(lookingForRooms(RoomRecipient*, const Coordinate & )));
  QObject::connect(m_pathMachine, SIGNAL(lookingForRooms(RoomRecipient*, ParseEvent* )), m_mapData, SLOT(lookingForRooms(RoomRecipient*, ParseEvent* )));
  QObject::connect(m_pathMachine, SIGNAL(lookingForRooms(RoomRecipient*, uint )), m_mapData, SLOT(lookingForRooms(RoomRecipient*, uint )));
  QObject::connect(m_mapData, SIGNAL(clearingMap()), m_pathMachine, SLOT(releaseAllPaths()));
  //QObject::connect(..., m_pathMachine, SLOT(deleteMostLikelyRoom()));
  QObject::connect(m_pathMachine, SIGNAL(playerMoved(const Coordinate & )),getCurrentMapWindow()->getCanvas(), SLOT(moveMarker(const Coordinate &)));

  QObject::connect(getCurrentMapWindow()->getCanvas(), SIGNAL(setCurrentRoom(uint)), m_pathMachine, SLOT(setCurrentRoom(uint)));
  QObject::connect(getCurrentMapWindow()->getCanvas(), SIGNAL(charMovedEvent(ParseEvent*)), m_pathMachine, SLOT(event(ParseEvent*)));

  //moved to mapwindow
  QObject::connect(m_mapData, SIGNAL(mapSizeChanged(const Coordinate &, const Coordinate &)), getCurrentMapWindow(), SLOT(setScrollBars(const Coordinate &, const Coordinate &)));
  QObject::connect(getCurrentMapWindow()->getCanvas(), SIGNAL(roomPositionChanged()), m_pathMachine, SLOT(retry()));

  connect(m_prespammedPath, SIGNAL(update()), getCurrentMapWindow()->getCanvas(), SLOT(update()));

  //QObject::connect(... , m_pathMachine, SLOT(event(ParseEvent* )));
  //QObject::connect(... , m_mapData, SLOT(insertPredefinedRoom(ParseEvent*, const Coordinate &, int )));
  //QObject::connect(... , m_mapData, SLOT(lookingForRooms(RoomRecipient*, const Coordinate &, const Coordinate & )));
  //...

  connect (m_mapData, SIGNAL(log( const QString&, const QString& )), this, SLOT(log( const QString&, const QString& )));
  connect (getCurrentMapWindow()->getCanvas(), SIGNAL(log( const QString&, const QString& )), this, SLOT(log( const QString&, const QString& )));

  connect (m_mapData, SIGNAL(onDataLoaded()), getCurrentMapWindow()->getCanvas(), SLOT(dataLoaded()) );

  connect(zoomInAct, SIGNAL(triggered()), getCurrentMapWindow()->getCanvas(), SLOT(zoomIn()));
  connect(zoomOutAct, SIGNAL(triggered()), getCurrentMapWindow()->getCanvas(), SLOT(zoomOut()));

  connect(getCurrentMapWindow()->getCanvas(), SIGNAL(newRoomSelection(const RoomSelection*)), this, SLOT(newRoomSelection(const RoomSelection*)));
  connect(getCurrentMapWindow()->getCanvas(), SIGNAL(newConnectionSelection(ConnectionSelection*)), this, SLOT(newConnectionSelection(ConnectionSelection*)));

  // Group
  connect(m_groupManager, SIGNAL(log(const QString&, const QString&)), this, SLOT(log(const QString&, const QString&)));
  connect(m_pathMachine, SIGNAL(setCharPosition(uint)), m_groupManager, SLOT(setCharPosition(uint)));
  connect(m_groupManager, SIGNAL(drawCharacters()), getCurrentMapWindow()->getCanvas(), SLOT(update()));
}


void MainWindow::log(const QString& module, const QString& message)
{
  logWindow->append("[" + module + "] " + message);
  logWindow->update();
}


void MainWindow::createActions()
{
  newAct = new QAction(QIcon(":/icons/new.png"), tr("&New"), this);
  newAct->setShortcut(tr("Ctrl+N"));
  newAct->setStatusTip(tr("Create a new file"));
  connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

  openAct = new QAction(QIcon(":/icons/open.png"), tr("&Open..."), this);
  openAct->setShortcut(tr("Ctrl+O"));
  openAct->setStatusTip(tr("Open an existing file"));
  connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

  reloadAct = new QAction(QIcon(":/icons/reload.png"), tr("&Reload"), this);
  reloadAct->setShortcut(tr("Ctrl+R"));
  reloadAct->setStatusTip(tr("Reload the current map"));
  connect(reloadAct, SIGNAL(triggered()), this, SLOT(reload()));

  saveAct = new QAction(QIcon(":/icons/save.png"), tr("&Save"), this);
  saveAct->setShortcut(tr("Ctrl+S"));
  saveAct->setStatusTip(tr("Save the document to disk"));
  connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

  saveAsAct = new QAction(tr("Save &As..."), this);
  saveAsAct->setStatusTip(tr("Save the document under a new name"));
  connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

  exportBaseMapAct = new QAction(tr("Export &Base Map As..."), this);
  exportBaseMapAct->setStatusTip(tr("Save a copy of the map with no secrets"));
  connect(exportBaseMapAct, SIGNAL(triggered()), this, SLOT(exportBaseMap()));

  mergeAct = new QAction(QIcon(":/icons/merge.png"), tr("&Merge..."), this);
  //mergeAct->setShortcut(tr("Ctrl+M"));
  mergeAct->setStatusTip(tr("Merge an existing file into current map"));
  connect(mergeAct, SIGNAL(triggered()), this, SLOT(merge()));

  exitAct = new QAction(tr("E&xit"), this);
  exitAct->setShortcut(tr("Ctrl+Q"));
  exitAct->setStatusTip(tr("Exit the application"));
  connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

  cutAct = new QAction(QIcon(":/icons/cut.png"), tr("Cu&t"), this);
  cutAct->setShortcut(tr("Ctrl+X"));
  cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                          "clipboard"));

  copyAct = new QAction(QIcon(":/icons/copy.png"), tr("&Copy"), this);
  copyAct->setShortcut(tr("Ctrl+C"));
  copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                           "clipboard"));

  pasteAct = new QAction(QIcon(":/icons/paste.png"), tr("&Paste"), this);
  pasteAct->setShortcut(tr("Ctrl+V"));
  pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                            "selection"));
  //connect(pasteAct, SIGNAL(triggered()), textEdit, SLOT(paste()));

  preferencesAct = new QAction(QIcon(":/icons/preferences.png"), tr("MMapper configuration"), this);
  preferencesAct->setShortcut(tr("Ctrl+P"));
  preferencesAct->setStatusTip(tr("MMapper2 configuration"));
  connect(preferencesAct, SIGNAL(triggered()), this, SLOT(onPreferences()));

  mmapperHomePageAct = new QAction(tr("MMapper &Homepage"), this);
  connect(mmapperHomePageAct, SIGNAL(triggered()), this, SLOT(openMmapperHomepage()));
  mumeWebsiteAct = new QAction(tr("&Website"), this);
  connect(mumeWebsiteAct, SIGNAL(triggered()), this, SLOT(openMumeWebsite()));
  mumeForumAct = new QAction(tr("&Forum"), this);
  connect(mumeForumAct, SIGNAL(triggered()), this, SLOT(openMumeForum()));
  mumeWikiAct = new QAction(tr("W&iki"), this);
  connect(mumeWikiAct, SIGNAL(triggered()), this, SLOT(openMumeWiki()));
  settingUpMmapperAct = new QAction(tr("&Setting up MMapper"), this);
  connect(settingUpMmapperAct, SIGNAL(triggered()), this, SLOT(openSettingUpMmapper()));
  newbieAct = new QAction(tr("&Information for Newcomers"), this);
  newbieAct->setStatusTip("Newbie help on the MUME website");
  connect(newbieAct, SIGNAL(triggered()), this, SLOT(openNewbieHelp()));
  aboutAct = new QAction(tr("About &MMapper"), this);
  aboutAct->setStatusTip(tr("Show the application's About box"));
  connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
  aboutQtAct = new QAction(tr("About &Qt"), this);
  aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
  connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
  /*
      nextWindowAct = new QAction(tr("Ne&xt Window"), this);
      nextWindowAct->setStatusTip(tr("Show the Next Window"));
      connect(nextWindowAct, SIGNAL(triggered()), this, SLOT(nextWindow()));

      prevWindowAct = new QAction(tr("Prev&ious Window"), this);
      prevWindowAct->setStatusTip(tr("Show the Previous Window"));
      connect(prevWindowAct, SIGNAL(triggered()), this, SLOT(prevWindow()));
  */
  zoomInAct = new QAction(QIcon(":/icons/viewmag+.png"), tr("Zoom In"), this);
  alwaysOnTopAct = new QAction(tr("Always on top"), this);
  alwaysOnTopAct->setCheckable(true);
  connect(alwaysOnTopAct, SIGNAL(triggered()), this, SLOT(alwaysOnTop()));

  zoomInAct->setStatusTip(tr("Zooms In current map"));
  zoomOutAct = new QAction(QIcon(":/icons/viewmag-.png"), tr("Zoom Out"), this);
  zoomOutAct->setStatusTip(tr("Zooms Out current map"));
  layerUpAct = new QAction(QIcon(":/icons/layerup.png"), tr("Layer Up"), this);
  layerUpAct->setStatusTip(tr("Layer Up"));
  connect(layerUpAct, SIGNAL(triggered()), this, SLOT(onLayerUp()));
  layerDownAct = new QAction(QIcon(":/icons/layerdown.png"), tr("Layer Down"), this);
  layerDownAct->setStatusTip(tr("Layer Down"));
  connect(layerDownAct, SIGNAL(triggered()), this, SLOT(onLayerDown()));

  modeConnectionSelectAct = new QAction(QIcon(":/icons/connectionselection.png"), tr("Select Connection"), this);
  modeConnectionSelectAct->setStatusTip(tr("Select Connection"));
  modeConnectionSelectAct->setCheckable(true);
  connect(modeConnectionSelectAct, SIGNAL(triggered()), this, SLOT(onModeConnectionSelect()));
  modeRoomSelectAct = new QAction(QIcon(":/icons/roomselection.png"), tr("Select Rooms"), this);
  modeRoomSelectAct->setStatusTip(tr("Select Rooms"));
  modeRoomSelectAct->setCheckable(true);
  connect(modeRoomSelectAct, SIGNAL(triggered()), this, SLOT(onModeRoomSelect()));
  modeMoveSelectAct = new QAction(QIcon(":/icons/mapmove.png"), tr("Move map"), this);
  modeMoveSelectAct->setStatusTip(tr("Move Map"));
  modeMoveSelectAct->setCheckable(true);
  connect(modeMoveSelectAct, SIGNAL(triggered()), this, SLOT(onModeMoveSelect()));
  modeInfoMarkEditAct = new QAction(QIcon(":/icons/infomarksedit.png"), tr("Edit Info Marks"), this);
  modeInfoMarkEditAct->setStatusTip(tr("Edit Info Marks"));
  modeInfoMarkEditAct->setCheckable(true);
  connect(modeInfoMarkEditAct, SIGNAL(triggered()), this, SLOT(onModeInfoMarkEdit()));

  createRoomAct = new QAction(QIcon(":/icons/roomcreate.png"), tr("Create New Rooms"), this);
  createRoomAct->setStatusTip(tr("Create New Rooms"));
  createRoomAct->setCheckable(true);
  connect(createRoomAct, SIGNAL(triggered()), this, SLOT(onModeCreateRoomSelect()));

  createConnectionAct = new QAction(QIcon(":/icons/connectioncreate.png"), tr("Create New Connection"), this);
  createConnectionAct->setStatusTip(tr("Create New Connection"));
  createConnectionAct->setCheckable(true);
  connect(createConnectionAct, SIGNAL(triggered()), this, SLOT(onModeCreateConnectionSelect()));

  createOnewayConnectionAct = new QAction(QIcon(":/icons/onewayconnectioncreate.png"), tr("Create New Oneway Connection"), this);
  createOnewayConnectionAct->setStatusTip(tr("Create New Oneway Connection"));
  createOnewayConnectionAct->setCheckable(true);
  connect(createOnewayConnectionAct, SIGNAL(triggered()), this, SLOT(onModeCreateOnewayConnectionSelect()));

  modeActGroup = new QActionGroup(this);
  modeActGroup->setExclusive(true);
  modeActGroup->addAction(modeMoveSelectAct);
  modeActGroup->addAction(modeRoomSelectAct);
  modeActGroup->addAction(modeConnectionSelectAct);
  modeActGroup->addAction(createRoomAct);
  modeActGroup->addAction(createConnectionAct);
  modeActGroup->addAction(createOnewayConnectionAct);
  modeActGroup->addAction(modeInfoMarkEditAct);
  modeMoveSelectAct->setChecked(true);

  editRoomSelectionAct = new QAction(QIcon(":/icons/roomedit.png"), tr("Edit Selected Rooms"), this);
  editRoomSelectionAct->setStatusTip(tr("Edit Selected Rooms"));
  editRoomSelectionAct->setShortcut(tr("Ctrl+E"));
  connect(editRoomSelectionAct, SIGNAL(triggered()), this, SLOT(onEditRoomSelection()));

  deleteRoomSelectionAct = new QAction(QIcon(":/icons/roomdelete.png"), tr("Delete Selected Rooms"), this);
  deleteRoomSelectionAct->setStatusTip(tr("Delete Selected Rooms"));
  connect(deleteRoomSelectionAct, SIGNAL(triggered()), this, SLOT(onDeleteRoomSelection()));

  moveUpRoomSelectionAct = new QAction(QIcon(":/icons/roommoveup.png"), tr("Move Up Selected Rooms"), this);
  moveUpRoomSelectionAct->setStatusTip(tr("Move Up Selected Rooms"));
  connect(moveUpRoomSelectionAct, SIGNAL(triggered()), this, SLOT(onMoveUpRoomSelection()));
  moveDownRoomSelectionAct = new QAction(QIcon(":/icons/roommovedown.png"), tr("Move Down Selected Rooms"), this);
  moveDownRoomSelectionAct->setStatusTip(tr("Move Down Selected Rooms"));
  connect(moveDownRoomSelectionAct, SIGNAL(triggered()), this, SLOT(onMoveDownRoomSelection()));
  mergeUpRoomSelectionAct = new QAction(QIcon(":/icons/roommergeup.png"), tr("Merge Up Selected Rooms"), this);
  mergeUpRoomSelectionAct->setStatusTip(tr("Merge Up Selected Rooms"));
  connect(mergeUpRoomSelectionAct, SIGNAL(triggered()), this, SLOT(onMergeUpRoomSelection()));
  mergeDownRoomSelectionAct = new QAction(QIcon(":/icons/roommergedown.png"), tr("Merge Down Selected Rooms"), this);
  mergeDownRoomSelectionAct->setStatusTip(tr("Merge Down Selected Rooms"));
  connect(mergeDownRoomSelectionAct, SIGNAL(triggered()), this, SLOT(onMergeDownRoomSelection()));
  connectToNeighboursRoomSelectionAct = new QAction(QIcon(":/icons/roomconnecttoneighbours.png"), tr("Connect room(s) to its neighbour rooms"), this);
  connectToNeighboursRoomSelectionAct->setStatusTip(tr("Connect room(s) to its neighbour rooms"));
  connect(connectToNeighboursRoomSelectionAct, SIGNAL(triggered()), this, SLOT(onConnectToNeighboursRoomSelection()));

  findRoomsAct = new QAction(QIcon(":/icons/roomfind.png"), tr("&Find Rooms"), this);
  findRoomsAct->setStatusTip(tr("Find Matching Rooms"));
  findRoomsAct->setShortcut(tr("Ctrl+F"));
  connect(findRoomsAct, SIGNAL(triggered()), this, SLOT(onFindRoom()));

  releaseAllPathsAct = new QAction(QIcon(":/icons/cancel.png"), tr("Release All Paths"), this);
  releaseAllPathsAct->setStatusTip(tr("Release All Paths"));
  releaseAllPathsAct->setCheckable(false);
  connect(releaseAllPathsAct, SIGNAL(triggered()), m_pathMachine, SLOT(releaseAllPaths()));

  forceRoomAct = new QAction(QIcon(":/icons/force.png"), tr("Force Path Machine to selected Room"), this);
  forceRoomAct->setStatusTip(tr("Force Path Machine to selected Room"));
  forceRoomAct->setCheckable(false);
  forceRoomAct->setEnabled(FALSE);
  connect(forceRoomAct, SIGNAL(triggered()), getCurrentMapWindow()->getCanvas(), SLOT(forceMapperToRoom()));

  roomActGroup = new QActionGroup(this);
  roomActGroup->setExclusive(false);
  roomActGroup->addAction(editRoomSelectionAct);
  roomActGroup->addAction(deleteRoomSelectionAct);
  roomActGroup->addAction(moveUpRoomSelectionAct);
  roomActGroup->addAction(moveDownRoomSelectionAct);
  roomActGroup->addAction(mergeUpRoomSelectionAct);
  roomActGroup->addAction(mergeDownRoomSelectionAct);
  roomActGroup->addAction(connectToNeighboursRoomSelectionAct);
  roomActGroup->setEnabled(FALSE);

  //editConnectionSelectionAct = new QAction(QIcon(":/icons/connectionedit.png"), tr("Edit Selected Connection"), this);
  //editConnectionSelectionAct->setStatusTip(tr("Edit Selected Connection"));
  //connect(editConnectionSelectionAct, SIGNAL(triggered()), this, SLOT(onEditConnectionSelection()));

  deleteConnectionSelectionAct = new QAction(QIcon(":/icons/connectiondelete.png"), tr("Delete Selected Connection"), this);
  deleteConnectionSelectionAct->setStatusTip(tr("Delete Selected Connection"));
  connect(deleteConnectionSelectionAct, SIGNAL(triggered()), this, SLOT(onDeleteConnectionSelection()));

  connectionActGroup = new QActionGroup(this);
  connectionActGroup->setExclusive(false);
  //connectionActGroup->addAction(editConnectionSelectionAct);
  connectionActGroup->addAction(deleteConnectionSelectionAct);
  connectionActGroup->setEnabled(FALSE);

  playModeAct = new QAction(QIcon(":/icons/online.png"), tr("Switch to play mode"), this);
  playModeAct->setStatusTip(tr("Switch to play mode - no new rooms are created"));
  playModeAct->setCheckable(true);
  connect(playModeAct, SIGNAL(triggered()), this, SLOT(onPlayMode()));

  mapModeAct = new QAction(QIcon(":/icons/map.png"), tr("Switch to mapping mode"), this);
  mapModeAct->setStatusTip(tr("Switch to mapping mode - new rooms are created when moving"));
  mapModeAct->setCheckable(true);
  connect(mapModeAct, SIGNAL(triggered()), this, SLOT(onMapMode()));

  offlineModeAct = new QAction(QIcon(":/icons/play.png"), tr("Switch to offline emulation mode"), this);
  offlineModeAct->setStatusTip(tr("Switch to offline emulation mode - you can learn areas offline"));
  offlineModeAct->setCheckable(true);
  connect(offlineModeAct, SIGNAL(triggered()), this, SLOT(onOfflineMode()));

  mapModeActGroup = new QActionGroup(this);
  mapModeActGroup->setExclusive(true);
  mapModeActGroup->addAction(playModeAct);
  mapModeActGroup->addAction(mapModeAct);
  mapModeActGroup->addAction(offlineModeAct);
  mapModeActGroup->setEnabled(true);

  cutAct->setEnabled(false);
  copyAct->setEnabled(false);

    // Find Room Dialog Connections
  connect(m_findRoomsDlg, SIGNAL(center(qint32, qint32)), getCurrentMapWindow(), SLOT(center(qint32, qint32)));
  connect(m_findRoomsDlg, SIGNAL(log( const QString&, const QString& )), this, SLOT(log( const QString&, const QString& )));

      // group Manager
  groupOffAct = new QAction(QIcon(":/icons/groupoff.png"), tr("Switch to &Off mode"), this );
  groupOffAct->setShortcut(tr("Ctrl+G"));
  groupOffAct->setCheckable(true);
  connect(groupOffAct, SIGNAL(toggled(bool)), this, SLOT(groupOff(bool)), Qt::QueuedConnection);

  groupClientAct = new QAction(QIcon(":/icons/groupclient.png"),tr("Switch to &Client mode"), this );
  groupClientAct->setCheckable(true);
  connect(groupClientAct, SIGNAL(toggled(bool)), this, SLOT(groupClient(bool)), Qt::QueuedConnection);

  groupServerAct = new QAction(QIcon(":/icons/groupserver.png"),tr("Switch to &Server mode"), this );
  groupServerAct->setCheckable(true);
  connect(groupServerAct, SIGNAL(toggled(bool)), this, SLOT(groupServer(bool)), Qt::QueuedConnection);

  groupManagerGroup = new QActionGroup(this);
  groupManagerGroup->addAction(groupOffAct);
  groupManagerGroup->addAction(groupClientAct);
  groupManagerGroup->addAction(groupServerAct);

  // Group Manager/Communicator
  connect(m_groupCommunicator, SIGNAL(typeChanged(int)), this, SLOT(groupManagerTypeChanged(int)), Qt::QueuedConnection);
  groupManagerTypeChanged( Config().m_groupManagerState );
}

void MainWindow::onPlayMode()
{
  QObject::disconnect(m_pathMachine, SIGNAL(createRoom(ParseEvent*, const Coordinate & )), m_mapData, SLOT(createRoom(ParseEvent*, const Coordinate & )));
  QObject::disconnect(m_pathMachine, SIGNAL(scheduleAction(MapAction *)), m_mapData, SLOT(scheduleAction(MapAction *)));
  Config().m_mapMode = 0;
}

void MainWindow::onMapMode()
{
  log("MainWindow","Map mode selected - new rooms are created when entering not mapped area.");
  QObject::connect(m_pathMachine, SIGNAL(createRoom(ParseEvent*, const Coordinate & )), m_mapData, SLOT(createRoom(ParseEvent*, const Coordinate & )));
  QObject::connect(m_pathMachine, SIGNAL(scheduleAction(MapAction *)), m_mapData, SLOT(scheduleAction(MapAction *)));
  Config().m_mapMode = 1;
}

void MainWindow::onOfflineMode()
{
  log("MainWindow","Offline emulation mode selected - you can learn areas 'safe' way.");
  QObject::disconnect(m_pathMachine, SIGNAL(createRoom(ParseEvent*, const Coordinate & )), m_mapData, SLOT(createRoom(ParseEvent*, const Coordinate & )));
  QObject::disconnect(m_pathMachine, SIGNAL(scheduleAction(MapAction *)), m_mapData, SLOT(scheduleAction(MapAction *)));
  Config().m_mapMode = 2;
}

void MainWindow::disableActions(bool value)
{
  newAct->setDisabled(value);
  openAct->setDisabled(value);
  mergeAct->setDisabled(value);
  reloadAct->setDisabled(value);
  saveAct->setDisabled(value);
  saveAsAct->setDisabled(value);
  exportBaseMapAct->setDisabled(value);
  exitAct->setDisabled(value);
  cutAct->setDisabled(value);
  copyAct->setDisabled(value);
  pasteAct->setDisabled(value);
  aboutAct->setDisabled(value);
  aboutQtAct->setDisabled(value);
  //    nextWindowAct->setDisabled(value);
  //    prevWindowAct->setDisabled(value);
  zoomInAct->setDisabled(value);
  zoomOutAct->setDisabled(value);
  playModeAct->setDisabled(value);
  mapModeAct->setDisabled(value);
  modeRoomSelectAct->setDisabled(value);
  modeConnectionSelectAct->setDisabled(value);
  modeMoveSelectAct->setDisabled(value);
  modeInfoMarkEditAct->setDisabled(value);
  layerUpAct->setDisabled(value);
  layerDownAct->setDisabled(value);
  createRoomAct->setDisabled(value);
  createConnectionAct->setDisabled(value);
  createOnewayConnectionAct->setDisabled(value);
  releaseAllPathsAct->setDisabled(value);
  alwaysOnTopAct->setDisabled(value);
}

void MainWindow::setupMenuBar()
{
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(newAct);
  fileMenu->addAction(openAct);
  fileMenu->addAction(reloadAct);
  fileMenu->addAction(saveAct);
  fileMenu->addAction(saveAsAct);
  fileMenu->addAction(exportBaseMapAct);
  fileMenu->addAction(mergeAct);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAct);

  editMenu = menuBar()->addMenu(tr("&Edit"));
  //editMenu->addAction(cutAct);
  //editMenu->addAction(copyAct);
  //editMenu->addAction(pasteAct);

  editMenu->addAction(playModeAct);
  editMenu->addAction(mapModeAct);
  editMenu->addAction(offlineModeAct);
  editMenu->addSeparator();
  editMenu->addAction(modeRoomSelectAct);
  editMenu->addAction(modeConnectionSelectAct);
  editMenu->addAction(modeMoveSelectAct);
  editMenu->addAction(modeInfoMarkEditAct);

  //editMenu->addAction(createRoomAct);
  //editMenu->addAction(createConnectionAct);

  roomMenu = menuBar()->addMenu(tr("&Rooms"));
  roomMenu->addAction(findRoomsAct);
  roomMenu->addSeparator();
  roomMenu->addAction(createRoomAct);
  roomMenu->addAction(editRoomSelectionAct);
  roomMenu->addAction(deleteRoomSelectionAct);
  roomMenu->addAction(moveUpRoomSelectionAct);
  roomMenu->addAction(moveDownRoomSelectionAct);
  roomMenu->addAction(mergeUpRoomSelectionAct);
  roomMenu->addAction(mergeDownRoomSelectionAct);
  roomMenu->addAction(connectToNeighboursRoomSelectionAct);

  connectionMenu = menuBar()->addMenu(tr("&Connections"));
  connectionMenu->addAction(createConnectionAct);
  connectionMenu->addAction(createOnewayConnectionAct);
  //connectionMenu->addAction(editConnectionSelectionAct);
  connectionMenu->addAction(deleteConnectionSelectionAct);

  menuBar()->addSeparator();

  viewMenu = menuBar()->addMenu(tr("&View"));
  viewMenu->addAction(m_dockDialogLog->toggleViewAction());
  viewMenu->addAction(m_dockDialogGroup->toggleViewAction());
  viewMenu->addAction(alwaysOnTopAct);
  viewMenu->addSeparator();
  viewMenu->addAction(zoomInAct);
  viewMenu->addAction(zoomOutAct);
  viewMenu->addSeparator();
  viewMenu->addAction(layerUpAct);
  viewMenu->addAction(layerDownAct);
  viewMenu->addAction(releaseAllPathsAct);
  //    viewMenu->addAction(nextWindowAct);
  //    viewMenu->addAction(prevWindowAct);
  //    viewMenu->addSeparator();

  groupMenu = menuBar()->addMenu(tr("&Group") );
  groupMenu->addAction(groupOffAct);
  groupMenu->addAction(groupClientAct);
  groupMenu->addAction(groupServerAct);


  settingsMenu = menuBar()->addMenu(tr("&Preferences"));
  settingsMenu->addAction(preferencesAct);

  helpMenu = menuBar()->addMenu(tr("&Help"));
  helpMenu->addAction(mmapperHomePageAct);
  mumeMenu = helpMenu->addMenu(tr("M&UME"));
  mumeMenu->addAction(mumeWebsiteAct);
  mumeMenu->addAction(mumeForumAct);
  mumeMenu->addAction(mumeWikiAct);
  onlineTutorialsMenu = helpMenu->addMenu(tr("Online &Tutorials"));
  onlineTutorialsMenu->addAction(newbieAct);
  onlineTutorialsMenu->addAction(settingUpMmapperAct);
  helpMenu->addSeparator();
  helpMenu->addAction(aboutAct);
  helpMenu->addAction(aboutQtAct);

  /*
  searchMenu = menuBar()->addMenu(tr("Sea&rch"));
  settingsMenu = menuBar()->addMenu(tr("&Settings"));
  */
}

void MainWindow::alwaysOnTop()
{
  setWindowFlags(windowFlags() ^ Qt::WindowStaysOnTopHint);
  show();
}



void MainWindow::setupToolBars()
{
  fileToolBar = addToolBar(tr("File"));
  fileToolBar->setObjectName("FileToolBar");
  fileToolBar->addAction(newAct);
  fileToolBar->addAction(openAct);
//  fileToolBar->addAction(mergeAct);
//  fileToolBar->addAction(reloadAct);
  fileToolBar->addAction(saveAct);
  /*
      editToolBar = addToolBar(tr("Edit"));
      editToolBar->addAction(cutAct);
      editToolBar->addAction(copyAct);
      editToolBar->addAction(pasteAct);
  */

  mapModeToolBar = addToolBar(tr("Map Mode"));
  mapModeToolBar->setObjectName("MapModeToolBar");
  mapModeToolBar->addAction(playModeAct);
  mapModeToolBar->addAction(mapModeAct);
  mapModeToolBar->addAction(offlineModeAct);

  modeToolBar = addToolBar(tr("Mouse Mode"));
  modeToolBar->setObjectName("ModeToolBar");
  modeToolBar->addAction(modeMoveSelectAct);
  modeToolBar->addAction(modeRoomSelectAct);
  modeToolBar->addAction(findRoomsAct);
  modeToolBar->addAction(modeConnectionSelectAct);
  modeToolBar->addAction(createRoomAct);
  modeToolBar->addAction(createConnectionAct);
  modeToolBar->addAction(createOnewayConnectionAct);
  modeToolBar->addAction(modeInfoMarkEditAct);

  groupToolBar = addToolBar(tr("Group Mode") );
  groupToolBar->setObjectName("GroupManagerToolBar");
  groupToolBar->addAction(groupOffAct);
  groupToolBar->addAction(groupClientAct);
  groupToolBar->addAction(groupServerAct);

  viewToolBar = addToolBar(tr("View"));
  viewToolBar->setObjectName("ViewToolBar");
  viewToolBar->addAction(zoomInAct);
  viewToolBar->addAction(zoomOutAct);
  viewToolBar->addAction(layerUpAct);
  viewToolBar->addAction(layerDownAct);

  pathMachineToolBar = addToolBar(tr("Path Machine"));
  pathMachineToolBar->setObjectName("PathMachineToolBar");
  pathMachineToolBar->addAction(releaseAllPathsAct);
  pathMachineToolBar->addAction(forceRoomAct);
  //viewToolBar->addAction(m_dockDialog->toggleViewAction());

  roomToolBar = addToolBar(tr("Rooms"));
  roomToolBar->setObjectName("RoomsToolBar");
  roomToolBar->addAction(editRoomSelectionAct);
  roomToolBar->addAction(deleteRoomSelectionAct);
  roomToolBar->addAction(moveUpRoomSelectionAct);
  roomToolBar->addAction(moveDownRoomSelectionAct);
  roomToolBar->addAction(mergeUpRoomSelectionAct);
  roomToolBar->addAction(mergeDownRoomSelectionAct);
  roomToolBar->addAction(connectToNeighboursRoomSelectionAct);

  connectionToolBar = addToolBar(tr("Connections"));
  connectionToolBar->setObjectName("ConnectionsToolBar");
  //connectionToolBar->addAction(editConnectionSelectionAct);
  connectionToolBar->addAction(deleteConnectionSelectionAct);

  settingsToolBar = addToolBar(tr("Preferences"));
  settingsToolBar->setObjectName("PreferencesToolBar");
  settingsToolBar->addAction(preferencesAct);
}

void MainWindow::setupStatusBar()
{
  statusBar()->showMessage(tr("Welcome to MMapper ..."));
}

void MainWindow::onPreferences()
{
  ConfigDialog dialog(m_groupManager);
  dialog.exec();
}

void MainWindow::newRoomSelection(const RoomSelection* rs)
{
  forceRoomAct->setEnabled(FALSE);
  m_roomSelection = rs;
  if (m_roomSelection)
  {
    roomActGroup->setEnabled(TRUE);
    if (m_roomSelection->size() == 1) {
	forceRoomAct->setEnabled(TRUE);
    }
  }
  else
  {
    roomActGroup->setEnabled(FALSE);
  }
}

void MainWindow::newConnectionSelection(ConnectionSelection* cs)
{
  m_connectionSelection = cs;
  if (m_connectionSelection)
  {
    connectionActGroup->setEnabled(TRUE);
  }
  else
  {
    connectionActGroup->setEnabled(FALSE);
  }
}


void MainWindow::nextWindow()
{
  if (m_stackedWidget == NULL) return;
  int i = (int)m_stackedWidget->currentIndex();
  int y = (int)m_stackedWidget->count();
  i++;
  if (i==y) i = 0;
  m_stackedWidget->setCurrentIndex(i);
}

void MainWindow::prevWindow()
{
  if (m_stackedWidget == NULL) return;
  int i = (int)m_stackedWidget->currentIndex();
  i--;
  if (i==-1) i = (int)m_stackedWidget->count()-1;
  m_stackedWidget->setCurrentIndex(i);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
  writeSettings();
  if (maybeSave())
  {
    event->accept();
  }
  else
  {
    event->ignore();
  }
  groupManagerTypeChanged(CGroupCommunicator::Off);
}

void MainWindow::newFile()
{
  getCurrentMapWindow()->getCanvas()->clearRoomSelection();
  getCurrentMapWindow()->getCanvas()->clearConnectionSelection();

  if (maybeSave())
  {
    AbstractMapStorage *storage = (AbstractMapStorage*) new MapStorage(*m_mapData , "");
    connect(storage, SIGNAL(onNewData()), getCurrentMapWindow()->getCanvas(), SLOT(dataLoaded()));
    connect(storage, SIGNAL(log(const QString&, const QString&)), this, SLOT(log(const QString&, const QString&)));
    storage->newData();
    delete(storage);
    setCurrentFile("");
  }
}

void MainWindow::merge()
{
  QString fileName = QFileDialog::getOpenFileName(this,"Choose map file ...","","MMapper2 (*.mm2);;MMapper (*.map)");
  if (!fileName.isEmpty())
  {
    QFile *file = new QFile(fileName);

    if (!file->open(QFile::ReadOnly))
    {
      QMessageBox::warning(this, tr("Application"),
                           tr("Cannot read file %1:\n%2.")
                           .arg(fileName)
                           .arg(file->errorString()));

      getCurrentMapWindow()->getCanvas()->setEnabled(true);
      delete file;
      return;
    }

    //MERGE
    progressDlg = new QProgressDialog(this);
    QPushButton *cb = new QPushButton("Abort ...");
    cb->setEnabled(false);
    progressDlg->setCancelButton ( cb );
    progressDlg->setLabelText("Importing map...");
    progressDlg->setCancelButtonText("Abort");
    progressDlg->setMinimum(0);
    progressDlg->setMaximum(100);
    progressDlg->setValue(0);
    progressDlg->show();

    getCurrentMapWindow()->getCanvas()->clearRoomSelection();
    getCurrentMapWindow()->getCanvas()->clearConnectionSelection();

    AbstractMapStorage *storage = new MapStorage(*m_mapData , fileName, file);
    connect(storage, SIGNAL(onDataLoaded()), getCurrentMapWindow()->getCanvas(), SLOT(dataLoaded()));
    connect(storage->progressCounter(), SIGNAL(onPercentageChanged(quint32)), this, SLOT(percentageChanged(quint32)));
    connect(storage, SIGNAL(log(const QString&, const QString&)), this, SLOT(log(const QString&, const QString&)));

    disableActions(true);
    getCurrentMapWindow()->getCanvas()->hide();
    if (storage->canLoad()) storage->mergeData();
    getCurrentMapWindow()->getCanvas()->show();
    disableActions(false);
    cutAct->setEnabled(false);
    copyAct->setEnabled(false);
    pasteAct->setEnabled(false);

    delete(storage);
    delete progressDlg;

    statusBar()->showMessage(tr("File merged"), 2000);
    delete file;
  }
}

void MainWindow::open()
{
  if (maybeSave())
  {
    QString fileName = QFileDialog::getOpenFileName(this,"Choose map file ...","","MMapper2 (*.mm2);;MMapper (*.map)");
    if (!fileName.isEmpty())
      loadFile(fileName);
  }
}

void MainWindow::reload()
{
  if (maybeSave())
  {
      loadFile(m_mapData->getFileName());
  }
}

bool MainWindow::save()
{
  if (m_mapData->getFileName().isEmpty())
  {
    return saveAs();
  }
  else
  {
    return saveFile( m_mapData->getFileName(), false );
  }
}

QPointer<QFileDialog> MainWindow::defaultSaveDialog()
{
  QPointer<QFileDialog> save = new QFileDialog( this, "Choose map file name ..." );
  save->setFileMode(QFileDialog::AnyFile);
  save->setDirectory(QDir::current());
  save->setFilter("MMapper2 (*.mm2)");
  save->setDefaultSuffix("mm2");
  save->setAcceptMode(QFileDialog::AcceptSave);

  return save;
}

bool MainWindow::saveAs()
{
  QPointer<QFileDialog> save = defaultSaveDialog();
  QStringList fileNames;
  if (save->exec()) {
    fileNames = save->selectedFiles();
  }

  if (fileNames.isEmpty()) {
    statusBar()->showMessage(tr("No filename provided"), 2000);
    return false;
  }

  return saveFile(fileNames[0], false);
}

bool MainWindow::exportBaseMap()
{
  QPointer<QFileDialog> save = defaultSaveDialog();

  QFileInfo currentFile( m_mapData->getFileName() );
  if ( currentFile.exists() )
  {
    save->setDirectory( currentFile.absoluteDir() );
    QString fileName = currentFile.fileName();
    save->selectFile( fileName.replace( QRegExp( "\\.mm2$" ), "-base.mm2" ) );
  }

  QStringList fileNames;
  if (save->exec()) {
    fileNames = save->selectedFiles();
  }

  if (fileNames.isEmpty()) {
    statusBar()->showMessage(tr("No filename provided"), 2000);
    return false;
  }

  return saveFile(fileNames[0], true);
}

void MainWindow::about()
{
  AboutDialog about(this);
  about.exec();
}

bool MainWindow::maybeSave()
{
  if ( m_mapData->dataChanged() )
  {
    int ret = QMessageBox::warning(this, tr("mmapper"),
                                   tr("The document has been modified.\n"
                                      "Do you want to save your changes?"),
                                   QMessageBox::Yes | QMessageBox::Default,
                                   QMessageBox::No,
                                   QMessageBox::Cancel | QMessageBox::Escape);
    if (ret == QMessageBox::Yes)
      return save();
    else if (ret == QMessageBox::Cancel)
      return false;
  }
  return true;
}

void MainWindow::loadFile(const QString &fileName)
{
  //getCurrentMapWindow()->getCanvas()->setEnabled(false);

  QFile *file = new QFile(fileName);
  //QIODevice *file = KFilterDev::deviceForFile( filename, "application/x-gzip", TRUE );

  if (!file->open(QFile::ReadOnly))
  {
    QMessageBox::warning(this, tr("Application"),
                         tr("Cannot read file %1:\n%2.")
                         .arg(fileName)
                         .arg(file->errorString()));

    getCurrentMapWindow()->getCanvas()->setEnabled(true);
    delete file;
    return;
  }

  //LOAD
  progressDlg = new QProgressDialog(this);
  QPushButton *cb = new QPushButton("Abort ...");
  cb->setEnabled(false);
  progressDlg->setCancelButton ( cb );
  progressDlg->setLabelText("Loading map...");
  progressDlg->setCancelButtonText("Abort");
  progressDlg->setMinimum(0);
  progressDlg->setMaximum(100);
  progressDlg->setValue(0);
  progressDlg->show();

  getCurrentMapWindow()->getCanvas()->clearRoomSelection();
  getCurrentMapWindow()->getCanvas()->clearConnectionSelection();

  AbstractMapStorage *storage = (AbstractMapStorage*) new MapStorage(*m_mapData , fileName, file);
  connect(storage, SIGNAL(onDataLoaded()), getCurrentMapWindow()->getCanvas(), SLOT(dataLoaded()));
  connect(storage->progressCounter(), SIGNAL(onPercentageChanged(quint32)), this, SLOT(percentageChanged(quint32)));
  connect(storage, SIGNAL(log(const QString&, const QString&)), this, SLOT(log(const QString&, const QString&)));

  disableActions(true);
  getCurrentMapWindow()->getCanvas()->hide();
  if (storage->canLoad()) storage->loadData();
  getCurrentMapWindow()->getCanvas()->show();
  disableActions(false);
  cutAct->setEnabled(false);
  copyAct->setEnabled(false);
  pasteAct->setEnabled(false);

  delete(storage);
  delete progressDlg;

  setCurrentFile(fileName);
  statusBar()->showMessage(tr("File loaded"), 2000);
  delete file;
  //getCurrentMapWindow()->getCanvas()->setEnabled(true);
}

void MainWindow::percentageChanged(quint32 p)
{
  if(!progressDlg) return;
  progressDlg->setValue(p);

  qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
  //qApp->processEvents();
}

bool MainWindow::saveFile(const QString &fileName, bool baseMapOnly )
{
  getCurrentMapWindow()->getCanvas()->setEnabled(false);

  FileSaver saver;
  try
  {
    saver.open( fileName );
  }
  catch ( std::exception &e )
  {
    QMessageBox::warning(NULL, tr("Application"),
                         tr("Cannot write file %1:\n%2.")
                         .arg(fileName)
                         .arg(e.what()));
    getCurrentMapWindow()->getCanvas()->setEnabled(true);
    return false;
  }


  //SAVE
  progressDlg = new QProgressDialog(this);
  QPushButton *cb = new QPushButton("Abort ...");
  cb->setEnabled(false);
  progressDlg->setCancelButton ( cb );
  progressDlg->setLabelText("Saving map...");
  progressDlg->setCancelButtonText("Abort");
  progressDlg->setMinimum(0);
  progressDlg->setMaximum(100);
  progressDlg->setValue(0);
  progressDlg->show();

  std::auto_ptr<AbstractMapStorage> storage( new MapStorage(*m_mapData , fileName, &saver.file()) );
  connect(storage->progressCounter(), SIGNAL(onPercentageChanged(quint32)), this, SLOT(percentageChanged(quint32)));
  connect(storage.get(), SIGNAL(log(const QString&, const QString&)), this, SLOT(log(const QString&, const QString&)));

  disableActions(true);
  //getCurrentMapWindow()->getCanvas()->hide();
  if (storage->canSave()) storage->saveData( baseMapOnly );
  //getCurrentMapWindow()->getCanvas()->show();
  disableActions(false);
  cutAct->setEnabled(false);
  copyAct->setEnabled(false);
  pasteAct->setEnabled(false);

  delete progressDlg;

  try
  {
    saver.close();
  }
  catch ( std::exception &e )
  {
    QMessageBox::warning(NULL, tr("Application"),
                         tr("Cannot write file %1:\n%2.")
                         .arg(fileName)
                         .arg(e.what()));
    getCurrentMapWindow()->getCanvas()->setEnabled(true);
    return false;
  }

  if ( !baseMapOnly )
    setCurrentFile(fileName);
  statusBar()->showMessage(tr("File saved"), 2000);
  getCurrentMapWindow()->getCanvas()->setEnabled(true);

  return true;
}

void MainWindow::onFindRoom()
{
    m_findRoomsDlg->show();
}

void MainWindow::groupManagerTypeChanged(int type)
{
  Config().m_groupManagerState = type;
  if (type == CGroupCommunicator::Server)
  {
    groupServerAct->setChecked(true);
    m_groupManager->show();
  }
  if (type == CGroupCommunicator::Client)
  {
    groupClientAct->setChecked(true);
    m_groupManager->show();
  }
  if (type == CGroupCommunicator::Off)
  {
    groupOffAct->setChecked(true);
    m_groupManager->hide();
  }

}


void MainWindow::groupOff(bool b)
{
  if (b && m_groupCommunicator->getType() != CGroupCommunicator::Off)
    m_groupManager->setType(CGroupCommunicator::Off);
}


void MainWindow::groupClient(bool b)
{
  if (b && m_groupCommunicator->getType() != CGroupCommunicator::Client)
    m_groupManager->setType(CGroupCommunicator::Client);
}


void MainWindow::groupServer(bool b)
{
  if (b && m_groupCommunicator->getType() != CGroupCommunicator::Server)
    m_groupManager->setType(CGroupCommunicator::Server);
}

void MainWindow::setCurrentFile(const QString &fileName)
{
  QString shownName;
  if (fileName.isEmpty())
    shownName = "untitled.mm2";
  else
    shownName = strippedName(fileName);

  setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("MMapper")));
}

QString MainWindow::strippedName(const QString &fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}

MapWindow *MainWindow::getCurrentMapWindow()
{
  return (dynamic_cast<MapWindow*>(m_stackedWidget->widget(m_stackedWidget->currentIndex())));
}

void MainWindow::onLayerUp()
{
  getCurrentMapWindow()->getCanvas()->layerUp();
}

void MainWindow::onLayerDown()
{
  getCurrentMapWindow()->getCanvas()->layerDown();
}

void MainWindow::onModeConnectionSelect()
{
  getCurrentMapWindow()->getCanvas()->setCanvasMouseMode(MapCanvas::CMM_SELECT_CONNECTIONS);
}

void MainWindow::onModeRoomSelect()
{
  getCurrentMapWindow()->getCanvas()->setCanvasMouseMode(MapCanvas::CMM_SELECT_ROOMS);
}

void MainWindow::onModeMoveSelect()
{
  getCurrentMapWindow()->getCanvas()->setCanvasMouseMode(MapCanvas::CMM_MOVE);
}

void MainWindow::onModeCreateRoomSelect()
{
  getCurrentMapWindow()->getCanvas()->setCanvasMouseMode(MapCanvas::CMM_CREATE_ROOMS);
}

void MainWindow::onModeCreateConnectionSelect()
{
  getCurrentMapWindow()->getCanvas()->setCanvasMouseMode(MapCanvas::CMM_CREATE_CONNECTIONS);
}

void MainWindow::onModeCreateOnewayConnectionSelect()
{
  getCurrentMapWindow()->getCanvas()->setCanvasMouseMode(MapCanvas::CMM_CREATE_ONEWAY_CONNECTIONS);
}

void MainWindow::onModeInfoMarkEdit()
{
  getCurrentMapWindow()->getCanvas()->setCanvasMouseMode(MapCanvas::CMM_EDIT_INFOMARKS);
}

void MainWindow::onEditRoomSelection()
{
  if (m_roomSelection)
  {
    RoomEditAttrDlg m_roomEditDialog;
    m_roomEditDialog.setRoomSelection(m_roomSelection, m_mapData, getCurrentMapWindow()->getCanvas());
    m_roomEditDialog.exec();
  }
}

void MainWindow::onEditConnectionSelection()
{

  if (m_connectionSelection)
  {
    /*RoomConnectionsDlg connectionsDlg;
    connectionsDlg.setRoom(static_cast<Room*>(m_connectionSelection->getFirst().room),
    								m_mapData,
    								static_cast<Room*>(m_connectionSelection->getSecond().room),
    								m_connectionSelection->getFirst().direction,
    								m_connectionSelection->getSecond().direction);
    connect(&connectionsDlg, SIGNAL(connectionChanged()), getCurrentMapWindow()->getCanvas(), SLOT(update()));

    connectionsDlg.exec();
    */
  }
}

void MainWindow::onDeleteRoomSelection()
{
  if (m_roomSelection)
  {
    m_mapData->execute(new GroupAction(new Remove(), m_roomSelection), m_roomSelection);
    getCurrentMapWindow()->getCanvas()->clearRoomSelection();
    getCurrentMapWindow()->getCanvas()->update();
  }
}

void MainWindow::onDeleteConnectionSelection()
{
  if (m_connectionSelection)
  {
    const Room *r1 = m_connectionSelection->getFirst().room;
    ExitDirection dir1 = m_connectionSelection->getFirst().direction;
    const Room *r2 = m_connectionSelection->getSecond().room;
    ExitDirection dir2 = m_connectionSelection->getSecond().direction;

    if (r2)
    {
      const RoomSelection *tmpSel = m_mapData->select();
      m_mapData->getRoom(r1->getId(), tmpSel);
      m_mapData->getRoom(r2->getId(), tmpSel);

      getCurrentMapWindow()->getCanvas()->clearConnectionSelection();

      m_mapData->execute(new RemoveTwoWayExit(r1->getId(), r2->getId(), dir1, dir2), tmpSel);
      //m_mapData->execute(new RemoveExit(r2->getId(), r1->getId(), dir2), tmpSel);

      m_mapData->unselect(tmpSel);
    }
  }

  getCurrentMapWindow()->getCanvas()->update();
}

void MainWindow::onMoveUpRoomSelection()
{
  if (!m_roomSelection) return;
  Coordinate moverel(0, 0, 1);
  m_mapData->execute(new GroupAction(new MoveRelative(moverel), m_roomSelection), m_roomSelection);
  onLayerUp();
  getCurrentMapWindow()->getCanvas()->update();
}

void MainWindow::onMoveDownRoomSelection()
{
  if (!m_roomSelection) return;
  Coordinate moverel(0, 0, -1);
  m_mapData->execute(new GroupAction(new MoveRelative(moverel), m_roomSelection), m_roomSelection);
  onLayerDown();
  getCurrentMapWindow()->getCanvas()->update();
}

void MainWindow::onMergeUpRoomSelection()
{
  if (!m_roomSelection) return;
  Coordinate moverel(0, 0, 1);
  m_mapData->execute(new GroupAction(new MergeRelative(moverel), m_roomSelection), m_roomSelection);
  onLayerUp();
  onModeRoomSelect();
}

void MainWindow::onMergeDownRoomSelection()
{
  if (!m_roomSelection) return;
  Coordinate moverel(0, 0, -1);
  m_mapData->execute(new GroupAction(new MergeRelative(moverel), m_roomSelection), m_roomSelection);
  onLayerDown();
  onModeRoomSelect();
}

void MainWindow::onConnectToNeighboursRoomSelection()
{
  if (!m_roomSelection) return;
  m_mapData->execute(new GroupAction(new ConnectToNeighbours, m_roomSelection), m_roomSelection);
  getCurrentMapWindow()->getCanvas()->update();
}

void MainWindow::openMmapperHomepage()
{
        QDesktopServices::openUrl(QUrl("http://sourceforge.net/projects/mmapper/"));
}

void MainWindow::openMumeWebsite()
{
        QDesktopServices::openUrl(QUrl("http://mume.org/"));
}

void MainWindow::openMumeForum()
{
        QDesktopServices::openUrl(QUrl("http://mume.org/forum/"));
}

void MainWindow::openMumeWiki()
{
        QDesktopServices::openUrl(QUrl("http://mume.org/wiki/"));
}

void MainWindow::openSettingUpMmapper()
{
        QDesktopServices::openUrl(QUrl("http://mume.org/wiki/index.php/Guide_to_install_mmapper2_on_Windows"));
}

void MainWindow::openNewbieHelp() {
  QDesktopServices::openUrl(QUrl("http://mume.org/newbie.php"));
}
