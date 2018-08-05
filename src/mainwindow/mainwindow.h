#pragma once
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

#include <memory>
#include <QActionGroup>
#include <QDockWidget>
#include <QFileDialog>
#include <QMainWindow>
#include <QProgressDialog>
#include <QSize>
#include <QString>
#include <QTextBrowser>
#include <QtCore>
#include <QtGlobal>

#include "../pandoragroup/mmapper2group.h"

class ClientWidget;
class CommandEvaluator;
class ConnectionListener;
class ConnectionSelection;
class FindRoomsDlg;
class GroupWidget;
class MapData;
class MapWindow;
class Mmapper2Group;
class Mmapper2PathMachine;
class MumeClock;
class PrespammedPath;
class QAction;
class QActionGroup;
class QCloseEvent;
class QFileDialog;
class QMenu;
class QObject;
class QPoint;
class QProgressDialog;
class QTextBrowser;
class QToolBar;
class QWidget;
class RoomSelection;
class WelcomeWidget;

class DockWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit DockWidget(const QString &title, QWidget *parent = 0, Qt::WindowFlags flags = 0);

    virtual QSize minimumSizeHint() const override;
    virtual QSize sizeHint() const override;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~MainWindow();

    enum class SaveMode { SAVEM_FULL, SAVEM_BASEMAP };
    enum class SaveFormat { SAVEF_MM2, SAVEF_WEB };
    bool saveFile(const QString &fileName, SaveMode mode, SaveFormat format);
    void loadFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);

signals:
    void setGroupManagerType(GroupManagerState);

public slots:
    void newFile();
    void open();
    void reload();
    void merge();
    bool save();
    bool saveAs();
    bool exportBaseMap();
    bool exportWebMap();
    void about();

    void percentageChanged(quint32);

    void log(const QString &, const QString &);

    void onModeConnectionSelect();
    void onModeRoomSelect();
    void onModeMoveSelect();
    void onModeInfoMarkEdit();
    void onModeCreateRoomSelect();
    void onModeCreateConnectionSelect();
    void onModeCreateOnewayConnectionSelect();
    void onLayerUp();
    void onLayerDown();
    void onCreateRoom();
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
    void onLaunchClient();
    void onPreferences();
    void onPlayMode();
    void onMapMode();
    void onOfflineMode();
    void alwaysOnTop();

    void newRoomSelection(const RoomSelection *);
    void newConnectionSelection(ConnectionSelection *);
    void showContextMenu(const QPoint &);

    void groupOff();
    void groupClient();
    void groupServer();
    void groupManagerOff();

    void onCheckForUpdate();
    void voteForMUMEOnTMC();
    void openMumeWebsite();
    void openMumeForum();
    void openMumeWiki();
    void openSettingUpMmapper();
    void openNewbieHelp();

protected:
    void closeEvent(QCloseEvent *event) override;
    virtual QSize minimumSizeHint() const override;
    virtual QSize sizeHint() const override;

private:
    MapWindow *m_mapWindow = nullptr;
    QTextBrowser *logWindow = nullptr;
    DockWidget *m_dockDialogLog = nullptr;
    DockWidget *m_dockDialogGroup = nullptr;
    DockWidget *m_dockWelcome = nullptr;

    ConnectionListener *m_listener = nullptr;
    Mmapper2PathMachine *m_pathMachine = nullptr;
    MapData *m_mapData = nullptr;
    CommandEvaluator *m_commandEvaluator = nullptr;
    PrespammedPath *m_prespammedPath = nullptr;
    MumeClock *m_mumeClock = nullptr;

    // Pandora Ported
    FindRoomsDlg *m_findRoomsDlg = nullptr;
    Mmapper2Group *m_groupManager = nullptr;
    GroupWidget *m_groupWidget = nullptr;

    ClientWidget *m_client = nullptr;
    WelcomeWidget *m_welcomeWidget = nullptr;

    const RoomSelection *m_roomSelection = nullptr;
    ConnectionSelection *m_connectionSelection = nullptr;

    QProgressDialog *progressDlg = nullptr;

    QToolBar *fileToolBar = nullptr;
    QToolBar *editToolBar = nullptr;
    QToolBar *mouseModeToolBar = nullptr;
    QToolBar *mapModeToolBar = nullptr;
    QToolBar *viewToolBar = nullptr;
    QToolBar *pathMachineToolBar = nullptr;
    QToolBar *roomToolBar = nullptr;
    QToolBar *connectionToolBar = nullptr;
    QToolBar *groupToolBar = nullptr;
    QToolBar *settingsToolBar = nullptr;

    QMenu *fileMenu = nullptr;
    QMenu *editMenu = nullptr;
    QMenu *roomMenu = nullptr;
    QMenu *connectionMenu = nullptr;
    QMenu *viewMenu = nullptr;
    QMenu *searchMenu = nullptr;
    QMenu *settingsMenu = nullptr;
    QMenu *helpMenu = nullptr;
    QMenu *mumeMenu = nullptr;
    QMenu *onlineTutorialsMenu = nullptr;
    QMenu *groupMenu = nullptr;

    QAction *groupOffAct = nullptr;
    QAction *groupClientAct = nullptr;
    QAction *groupServerAct = nullptr;
    QAction *groupShowHideAct = nullptr;
    QAction *groupSettingsAct = nullptr;

    QAction *newAct = nullptr;
    QAction *openAct = nullptr;
    QAction *mergeAct = nullptr;
    QAction *reloadAct = nullptr;
    QAction *saveAct = nullptr;
    QAction *saveAsAct = nullptr;
    QAction *exportBaseMapAct = nullptr;
    QAction *exportWebMapAct = nullptr;
    QAction *exitAct = nullptr;
    //QAction *cutAct;
    //QAction *copyAct;
    //QAction *pasteAct;
    QAction *voteAct = nullptr;
    QAction *mmapperCheckForUpdateAct = nullptr;
    QAction *mumeWebsiteAct = nullptr;
    QAction *mumeForumAct = nullptr;
    QAction *mumeWikiAct = nullptr;
    QAction *settingUpMmapperAct = nullptr;
    QAction *newbieAct = nullptr;
    QAction *aboutAct = nullptr;
    QAction *aboutQtAct = nullptr;
    QAction *prevWindowAct = nullptr;
    QAction *nextWindowAct = nullptr;
    QAction *zoomInAct = nullptr;
    QAction *zoomOutAct = nullptr;
    QAction *alwaysOnTopAct = nullptr;
    QAction *preferencesAct = nullptr;

    QAction *layerUpAct = nullptr;
    QAction *layerDownAct = nullptr;

    QAction *modeConnectionSelectAct = nullptr;
    QAction *modeRoomSelectAct = nullptr;
    QAction *modeMoveSelectAct = nullptr;
    QAction *modeInfoMarkEditAct = nullptr;

    QAction *modeCreateRoomAct = nullptr;
    QAction *modeCreateConnectionAct = nullptr;
    QAction *modeCreateOnewayConnectionAct = nullptr;

    QAction *playModeAct = nullptr;
    QAction *mapModeAct = nullptr;
    QAction *offlineModeAct = nullptr;

    QActionGroup *mapModeActGroup = nullptr;
    QActionGroup *modeActGroup = nullptr;
    QActionGroup *roomActGroup = nullptr;
    QActionGroup *connectionActGroup = nullptr;
    QActionGroup *groupManagerGroup = nullptr;

    QAction *createRoomAct = nullptr;
    QAction *editRoomSelectionAct = nullptr;
    QAction *editConnectionSelectionAct = nullptr;
    QAction *deleteRoomSelectionAct = nullptr;
    QAction *deleteConnectionSelectionAct = nullptr;

    QAction *moveUpRoomSelectionAct = nullptr;
    QAction *moveDownRoomSelectionAct = nullptr;
    QAction *mergeUpRoomSelectionAct = nullptr;
    QAction *mergeDownRoomSelectionAct = nullptr;
    QAction *connectToNeighboursRoomSelectionAct = nullptr;

    QAction *findRoomsAct = nullptr;
    QAction *clientAct = nullptr;

    QAction *forceRoomAct = nullptr;
    QAction *releaseAllPathsAct = nullptr;

    void wireConnections();

    void createActions();
    void disableActions(bool value);
    void setupMenuBar();
    void setupToolBars();
    void setupStatusBar();

    void readSettings();
    void writeSettings();

    QString strippedName(const QString &fullFileName);
    bool maybeSave();
    std::unique_ptr<QFileDialog> createDefaultSaveDialog();
};

#endif
