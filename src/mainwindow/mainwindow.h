#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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

#include "../mapdata/roomselection.h"
#include "../pandoragroup/mmapper2group.h"

class ClientWidget;
class ConnectionListener;
class ConnectionSelection;
class InfoMarkSelection;
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
class UpdateDialog;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    ~MainWindow() override;

    enum class SaveModeEnum { FULL, BASEMAP };
    enum class SaveFormatEnum { MM2, WEB, MMP };
    bool saveFile(const QString &fileName, SaveModeEnum mode, SaveFormatEnum format);
    void loadFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    void startServices();

signals:
    void setGroupMode(GroupManagerStateEnum);
    void startGroupNetwork();
    void stopGroupNetwork();

public slots:
    void newFile();
    void open();
    void reload();
    void merge();
    bool save();
    bool saveAs();
    bool exportBaseMap();
    bool exportWebMap();
    bool exportMmpMap();
    void about();

    void percentageChanged(quint32);

    void log(const QString &, const QString &);

    void onModeConnectionSelect();
    void onModeRoomSelect();
    void onModeMoveSelect();
    void onModeInfoMarkSelect();
    void onModeCreateInfoMarkSelect();
    void onModeCreateRoomSelect();
    void onModeCreateConnectionSelect();
    void onModeCreateOnewayConnectionSelect();
    void onLayerUp();
    void onLayerDown();
    void onLayerReset();
    void onCreateRoom();
    void onEditRoomSelection();
    void onEditInfoMarkSelection();
    void onDeleteInfoMarkSelection();
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

    void newRoomSelection(const SigRoomSelection &);
    void newConnectionSelection(ConnectionSelection *);
    void newInfoMarkSelection(InfoMarkSelection *);
    void showContextMenu(const QPoint &);

    void onModeGroupOff();
    void onModeGroupClient();
    void onModeGroupServer();
    void groupNetworkStatus(bool toggle);

    void onCheckForUpdate();
    void voteForMUMEOnTMC();
    void openMumeWebsite();
    void openMumeForum();
    void openMumeWiki();
    void openSettingUpMmapper();
    void openNewbieHelp();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    MapWindow *m_mapWindow = nullptr;
    QTextBrowser *logWindow = nullptr;
    QDockWidget *m_dockDialogLog = nullptr;
    QDockWidget *m_dockDialogGroup = nullptr;
    QDockWidget *m_dockLaunch = nullptr;

    ConnectionListener *m_listener = nullptr;
    Mmapper2PathMachine *m_pathMachine = nullptr;
    MapData *m_mapData = nullptr;
    PrespammedPath *m_prespammedPath = nullptr;
    MumeClock *m_mumeClock = nullptr;

    // Pandora Ported
    FindRoomsDlg *m_findRoomsDlg = nullptr;
    Mmapper2Group *m_groupManager = nullptr;
    GroupWidget *m_groupWidget = nullptr;

    ClientWidget *m_client = nullptr;
    WelcomeWidget *m_launchWidget = nullptr;
    UpdateDialog *m_updateDialog = nullptr;

    SharedRoomSelection m_roomSelection;
    ConnectionSelection *m_connectionSelection = nullptr;
    std::shared_ptr<InfoMarkSelection> m_infoMarkSelection;

    QProgressDialog *progressDlg = nullptr;

    QToolBar *fileToolBar = nullptr;
    QToolBar *mouseModeToolBar = nullptr;
    QToolBar *mapperModeToolBar = nullptr;
    QToolBar *viewToolBar = nullptr;
    QToolBar *pathMachineToolBar = nullptr;
    QToolBar *roomToolBar = nullptr;
    QToolBar *connectionToolBar = nullptr;
    QToolBar *groupToolBar = nullptr;
    QToolBar *settingsToolBar = nullptr;

    QMenu *fileMenu = nullptr;
    QMenu *editMenu = nullptr;
    QMenu *modeMenu = nullptr;
    QMenu *roomMenu = nullptr;
    QMenu *connectionMenu = nullptr;
    QMenu *viewMenu = nullptr;
    QMenu *settingsMenu = nullptr;
    QMenu *helpMenu = nullptr;
    QMenu *mumeMenu = nullptr;
    QMenu *onlineTutorialsMenu = nullptr;
    QMenu *groupMenu = nullptr;
    QMenu *groupModeMenu = nullptr;

    QAction *newAct = nullptr;
    QAction *openAct = nullptr;
    QAction *mergeAct = nullptr;
    QAction *reloadAct = nullptr;
    QAction *saveAct = nullptr;
    QAction *saveAsAct = nullptr;
    QAction *exportBaseMapAct = nullptr;
    QAction *exportWebMapAct = nullptr;
    QAction *exportMmpMapAct = nullptr;
    QAction *exitAct = nullptr;
    QAction *voteAct = nullptr;
    QAction *mmapperCheckForUpdateAct = nullptr;
    QAction *mumeWebsiteAct = nullptr;
    QAction *mumeForumAct = nullptr;
    QAction *mumeWikiAct = nullptr;
    QAction *settingUpMmapperAct = nullptr;
    QAction *newbieAct = nullptr;
    QAction *aboutAct = nullptr;
    QAction *aboutQtAct = nullptr;
    QAction *zoomInAct = nullptr;
    QAction *zoomOutAct = nullptr;
    QAction *zoomResetAct = nullptr;
    QAction *alwaysOnTopAct = nullptr;
    QAction *preferencesAct = nullptr;

    QAction *layerUpAct = nullptr;
    QAction *layerDownAct = nullptr;
    QAction *layerResetAct = nullptr;

    struct MouseModeActions
    {
        QActionGroup *mouseModeActGroup = nullptr;
        QAction *modeConnectionSelectAct = nullptr;
        QAction *modeRoomSelectAct = nullptr;
        QAction *modeMoveSelectAct = nullptr;
        QAction *modeInfoMarkSelectAct = nullptr;
        QAction *modeCreateInfoMarkAct = nullptr;
        QAction *modeCreateRoomAct = nullptr;
        QAction *modeCreateConnectionAct = nullptr;
        QAction *modeCreateOnewayConnectionAct = nullptr;
    } mouseMode{};

    struct MapperModeActions
    {
        QActionGroup *mapModeActGroup = nullptr;
        QAction *playModeAct = nullptr;
        QAction *mapModeAct = nullptr;
        QAction *offlineModeAct = nullptr;
    } mapperMode{};

    QActionGroup *selectedRoomActGroup = nullptr;
    QActionGroup *selectedConnectionActGroup = nullptr;

    struct InfoMarkActions
    {
        QActionGroup *infoMarkGroup = nullptr;
        QAction *deleteInfoMarkAct = nullptr;
        QAction *editInfoMarkAct = nullptr;
    } infoMarkActions{};

    struct GroupModeActions
    {
        QActionGroup *groupModeGroup = nullptr;
        QAction *groupOffAct = nullptr;
        QAction *groupClientAct = nullptr;
        QAction *groupServerAct = nullptr;
    } groupMode{};

    struct GroupNetworkActions
    {
        QActionGroup *groupNetworkGroup = nullptr;
        QAction *networkStartAct = nullptr;
        QAction *networkStopAct = nullptr;
    } groupNetwork{};

    QAction *createRoomAct = nullptr;
    QAction *editRoomSelectionAct = nullptr;
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

    bool maybeSave();
    std::unique_ptr<QFileDialog> createDefaultSaveDialog();
};
