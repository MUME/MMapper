#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../display/CanvasMouseModeEnum.h"
#include "../global/Signal2.h"
#include "../global/macros.h"
#include "../group/mmapper2group.h"
#include "../mapdata/roomselection.h"
#include "../mapstorage/MapDestination.h"
#include "../mapstorage/MapSource.h"
#include "AsyncTypes.h"

#include <functional>
#include <memory>

#include <QPointer>
#include <QString>
#include <QtCore>
#include <QtWidgets>

class AbstractMapStorage;
class AdventureTracker;
class AdventureWidget;
class AnsiOstream;
class AudioManager;
class AutoLogger;
class ClientWidget;
class ConfigDialog;
class ConnectionListener;
class ConnectionSelection;
class FindRoomsDlg;
class GameObserver;
class GroupWidget;
class HotkeyManager;
class InfomarkSelection;
class MapCanvas;
class MapData;
class MapWindow;
class Mmapper2Group;
class Mmapper2PathMachine;
class MumeClock;
class PrespammedPath;
class CTimers;
class QAction;
class QActionGroup;
class QCloseEvent;
class QFileDialog;
class QMenu;
class QObject;
class QPoint;
class QProgressDialog;
class QShowEvent;
class QTextBrowser;
class QToolBar;
class QWidget;
class RoomEditAttrDlg;
class RoomManager;
class RoomSelection;
class RoomWidget;
class UpdateDialog;
class DescriptionWidget;
class MediaLibrary;
class TimerWidget;
class MapDestination;

struct MapLoadData;

class NODISCARD_QOBJECT MainWindow final : public QMainWindow
{
    Q_OBJECT

private:
    static inline MainWindow *g_mainWindow = nullptr;

private:
    MapWindow *m_mapWindow = nullptr;
    QTextBrowser *m_logWindow = nullptr;

    QDockWidget *m_dockDialogRoom = nullptr;
    QDockWidget *m_dockDialogLog = nullptr;
    QDockWidget *m_dockDialogClient = nullptr;
    QDockWidget *m_dockDialogGroup = nullptr;
    QDockWidget *m_dockDialogAdventure = nullptr;
    QDockWidget *m_dockDialogDescription = nullptr;
    QDockWidget *m_dockDialogTimers = nullptr;
    QDockWidget *m_dockDialogAsync = nullptr;

    std::unique_ptr<GameObserver> m_gameObserver;
    AutoLogger *m_logger = nullptr;
    ConnectionListener *m_listener = nullptr;
    Mmapper2PathMachine *m_pathMachine = nullptr;
    MapData *m_mapData = nullptr;
    PrespammedPath *m_prespammedPath = nullptr;
    MumeClock *m_mumeClock = nullptr;
    CTimers *m_timers = nullptr;

    // Pandora Ported
    FindRoomsDlg *m_findRoomsDlg = nullptr;
    Mmapper2Group *m_groupManager = nullptr;
    GroupWidget *m_groupWidget = nullptr;

    RoomWidget *m_roomWidget = nullptr;
    RoomManager *m_roomManager = nullptr;

    ClientWidget *m_clientWidget = nullptr;
    UpdateDialog *m_updateDialog = nullptr;

    AdventureTracker *m_adventureTracker = nullptr;
    AdventureWidget *m_adventureWidget = nullptr;
    MediaLibrary *m_mediaLibrary = nullptr;
    AudioManager *m_audioManager = nullptr;

    DescriptionWidget *m_descriptionWidget = nullptr;
    TimerWidget *m_timerWidget = nullptr;
    std::unique_ptr<HotkeyManager> m_hotkeyManager;

    QPointer<QMenu> m_contextMenu;

    SharedRoomSelection m_roomSelection;
    std::shared_ptr<ConnectionSelection> m_connectionSelection;
    std::shared_ptr<InfomarkSelection> m_infoMarkSelection;

    std::unique_ptr<RoomEditAttrDlg> m_roomEditAttrDlg;

    QToolBar *fileToolBar = nullptr;
    QToolBar *mouseModeToolBar = nullptr;
    QToolBar *mapperModeToolBar = nullptr;
    QToolBar *viewToolBar = nullptr;
    QToolBar *pathMachineToolBar = nullptr;
    QToolBar *roomToolBar = nullptr;
    QToolBar *connectionToolBar = nullptr;
    QToolBar *settingsToolBar = nullptr;
    QToolBar *audioToolBar = nullptr;

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

    QAction *newAct = nullptr;
    QAction *openAct = nullptr;
    QAction *mergeAct = nullptr;
    QAction *reloadAct = nullptr;
    QAction *saveAct = nullptr;
    QAction *saveAsAct = nullptr;
    QAction *exportBaseMapAct = nullptr;
    QAction *exportMm2xmlMapAct = nullptr;
    QAction *exportWebMapAct = nullptr;
    QAction *exportMmpMapAct = nullptr;
    QAction *exitAct = nullptr;

    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;

    QAction *voteAct = nullptr;
    QAction *mmapperCheckForUpdateAct = nullptr;
    QAction *mumeWebsiteAct = nullptr;
    QAction *mumeForumAct = nullptr;
    QAction *mumeWikiAct = nullptr;
    QAction *settingUpMmapperAct = nullptr;
    QAction *newbieAct = nullptr;
    QAction *actionReportIssue = nullptr;
    QAction *aboutAct = nullptr;
    QAction *aboutQtAct = nullptr;
    QAction *zoomInAct = nullptr;
    QAction *zoomOutAct = nullptr;
    QAction *zoomResetAct = nullptr;
    QAction *alwaysOnTopAct = nullptr;
    QAction *showStatusBarAct = nullptr;
    QAction *showScrollBarsAct = nullptr;
    QAction *showMenuBarAct = nullptr;
    QAction *preferencesAct = nullptr;

    QAction *layerUpAct = nullptr;
    QAction *layerDownAct = nullptr;
    QAction *layerResetAct = nullptr;

    struct NODISCARD MouseModeActions final
    {
        QActionGroup *mouseModeActGroup = nullptr;
        QAction *modeConnectionSelectAct = nullptr;
        QAction *modeRoomRaypickAct = nullptr;
        QAction *modeRoomSelectAct = nullptr;
        QAction *modeMoveSelectAct = nullptr;
        QAction *modeInfomarkSelectAct = nullptr;
        QAction *modeCreateInfomarkAct = nullptr;
        QAction *modeCreateRoomAct = nullptr;
        QAction *modeCreateConnectionAct = nullptr;
        QAction *modeCreateOnewayConnectionAct = nullptr;
    } mouseMode{};

    struct NODISCARD MapperModeActions final
    {
        QActionGroup *mapModeActGroup = nullptr;
        QAction *playModeAct = nullptr;
        QAction *mapModeAct = nullptr;
        QAction *offlineModeAct = nullptr;
    } mapperMode{};

    QActionGroup *selectedRoomActGroup = nullptr;
    QActionGroup *selectedConnectionActGroup = nullptr;

    struct NODISCARD InfomarkActions final
    {
        QActionGroup *infomarkGroup = nullptr;
        QAction *deleteInfomarkAct = nullptr;
        QAction *editInfomarkAct = nullptr;
    } infomarkActions{};

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
    QAction *saveLogAct = nullptr;
    QAction *saveLogAsHtmlAct = nullptr;

    QAction *gotoRoomAct = nullptr;
    QAction *forceRoomAct = nullptr;
    QAction *releaseAllPathsAct = nullptr;
    QAction *rebuildMeshesAct = nullptr;

    std::unique_ptr<ConfigDialog> m_configDialog;

    struct AsyncBase;
    struct AsyncIO;
    struct AsyncLoader;
    struct AsyncMerge;
    struct AsyncSaver;
    std::unique_ptr<AsyncIO> m_asyncIO;
    NODISCARD AsyncIO &getAsyncIO() { return deref(m_asyncIO); }
    friend QDebug &operator<<(QDebug &debug, const AsyncIO &task);

    Signal2Lifetime m_lifetime;

public:
    explicit MainWindow();
    ~MainWindow() final;

    NODISCARD HotkeyManager &getHotkeyManager() const { return deref(m_hotkeyManager); }
    NODISCARD CTimers &getTimers() const { return deref(m_timers); }

    NODISCARD bool saveFile(const QString &fileName, SaveModeEnum mode, SaveFormatEnum format);
    void loadFile(std::shared_ptr<MapSource> source);
    void setCurrentFile(const QString &fileName);

private:
    void asyncTaskEnded(const QString &taskName);
    void showAsyncFailure(const QString &fileName, AsyncIOTypeEnum mode, bool wasCanceled);
    NODISCARD std::unique_ptr<AbstractMapStorage> getLoadOrMergeMapStorage(
        std::shared_ptr<MapSource> &source);

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;
    NODISCARD bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void startServices();
    void forceNewFile();
    void showWarning(const QString &s);
    void showStatusInternal(const QString &txt, int duration);
    void showStatusShort(const QString &txt) { showStatusInternal(txt, 2000); }
    void showStatusLong(const QString &txt) { showStatusInternal(txt, 5000); }
    void showStatusForever(const QString &txt) { showStatusInternal(txt, 0); }
    NODISCARD bool tryStartNewAsync();

private:
    void wireConnections();

    void createActions();
    void setupMenuBar();
    void setupToolBars();
    void setupStatusBar();

    void readSettings();
    void writeSettings();

    NODISCARD bool maybeSave();

    struct ActionDisabler;
    void disableActions(bool value);

    struct NODISCARD CanvasHider final
    {
    private:
        MainWindow &m_self;

    public:
        explicit CanvasHider(MainWindow &self)
            : m_self(self)
        {
            self.hideCanvas(true);
        }
        ~CanvasHider() { m_self.hideCanvas(false); }

    public:
        DELETE_CTORS_AND_ASSIGN_OPS(CanvasHider);
    };
    void hideCanvas(bool hide);

    NODISCARD MapCanvas *getCanvas() const;
    void mapChanged() const;
    void setCanvasMouseMode(CanvasMouseModeEnum mode);
    void setMapModified(bool);
    void updateMapModified();

private:
    void applyGroupAction(const std::function<Change(const RawRoom &)> &getChange);
    void onSuccessfulLoad(const MapLoadData &mapLoadData);
    void onSuccessfulMerge(const Map &map);
    void onSuccessfulSave(SaveModeEnum mode, SaveFormatEnum format, const QString &fileName);

public slots:
    void slot_newFile();
    void slot_open();
    void slot_reload();
    void slot_merge();
    NODISCARD bool slot_save();
    NODISCARD bool slot_saveAs();
    NODISCARD bool slot_exportBaseMap();
    NODISCARD bool slot_exportMm2xmlMap();
    NODISCARD bool slot_exportWebMap();
    NODISCARD bool slot_exportMmpMap();
    void slot_about();

    NODISCARD bool slot_generateBaseMap();

    void slot_log(const QString &, const QString &);

    void slot_onModeConnectionSelect();
    void slot_onModeRoomRaypick();
    void slot_onModeRoomSelect();
    void slot_onModeMoveSelect();
    void slot_onModeInfomarkSelect();
    void slot_onModeCreateInfomarkSelect();
    void slot_onModeCreateRoomSelect();
    void slot_onModeCreateConnectionSelect();
    void slot_onModeCreateOnewayConnectionSelect();
    void slot_onLayerUp();
    void slot_onLayerDown();
    void slot_onLayerReset();
    void slot_onCreateRoom();
    void slot_onEditRoomSelection();
    void slot_onEditInfomarkSelection();
    void slot_onDeleteInfomarkSelection();
    void slot_onDeleteRoomSelection();
    void slot_onDeleteConnectionSelection();
    NODISCARD bool slot_moveRoomSelection(Coordinate offset);
    void slot_onMoveUpRoomSelection();
    void slot_onMoveDownRoomSelection();
    NODISCARD bool slot_mergeRoomSelection(Coordinate offset);
    void slot_onMergeUpRoomSelection();
    void slot_onMergeDownRoomSelection();
    void slot_onConnectToNeighboursRoomSelection();
    void slot_forceMapperToRoom();
    void slot_onFindRoom();
    void slot_onLaunchClient();
    void slot_onPreferences();
    void slot_onPlayMode();
    void slot_onMapMode();
    void slot_onOfflineMode();
    void slot_setMode(MapModeEnum mode);
    void slot_alwaysOnTop();
    void slot_setShowStatusBar();
    void slot_setShowScrollBars();
    void slot_setShowMenuBar();

    void slot_newRoomSelection(const SigRoomSelection &);
    void slot_newConnectionSelection(ConnectionSelection *);
    void slot_newInfomarkSelection(InfomarkSelection *);
    void slot_showContextMenu(const QPoint &);
    void slot_closeContextMenu();

    void slot_onCheckForUpdate();
    void slot_voteForMUME();
    void slot_openMumeWebsite();
    void slot_openMumeForum();
    void slot_openMumeWiki();
    void slot_openSettingUpMmapper();
    void slot_openNewbieHelp();
    void onReportIssueTriggered();
};
