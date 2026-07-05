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
#include "../proxy/ProxyHost.h"
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
class MapCanvasWindow;
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
class QLabel;
class QMenu;
class QObject;
class QPoint;
class QProgressDialog;
class QShowEvent;
class QTextBrowser;
class QToolBar;
class QToolButton;
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
class RemoteEdit;

struct MapLoadData;

class NODISCARD_QOBJECT MainWindow final : public QMainWindow, public ProxyHost
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

    std::unique_ptr<ConfigDialog> m_configDialog;

    AdventureTracker *m_adventureTracker = nullptr;
    AdventureWidget *m_adventureWidget = nullptr;
    MediaLibrary *m_mediaLibrary = nullptr;
    AudioManager *m_audioManager = nullptr;

    DescriptionWidget *m_descriptionWidget = nullptr;
    TimerWidget *m_timerWidget = nullptr;
    std::unique_ptr<HotkeyManager> m_hotkeyManager;
    RemoteEdit *m_remoteEdit = nullptr;

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
    // Compact-layout only (see setCompactLayout()): a row of buttons in the
    // status bar for the map operations touch has no wheel or keyboard
    // for (layers, centering). Not a QToolBar, so it stays out of
    // saveState() and does not cost a row of its own.
    QWidget *m_compactActionBar = nullptr;
    // "\u2630" in the status bar while compact: m_appMenu.
    QToolButton *m_menuButton = nullptr;
    QLabel *m_pathMachineStatus = nullptr;

    QMenu *fileMenu = nullptr;
    QMenu *editMenu = nullptr;
    QMenu *modeMenu = nullptr;
    QMenu *roomMenu = nullptr;
    QMenu *connectionMenu = nullptr;
    QMenu *viewMenu = nullptr;
    QMenu *windowMenu = nullptr;
    QMenu *settingsMenu = nullptr;
    QMenu *helpMenu = nullptr;
    // The top-level menus above as submenus of one menu: the "\u2630" button
    // while compact, and an entry of the map's context menu whenever neither
    // the menu bar nor that button is on screen, so that a touch user (no
    // hover to "peek" a hidden menu bar with) can always reach them by a
    // long-press on the map. A QAction can sit in several widgets at once,
    // so the menus stay in the menu bar as well.
    QMenu *m_appMenu = nullptr;
    QMenu *mumeMenu = nullptr;
    QMenu *onlineTutorialsMenu = nullptr;
    // Compact layout for small windows; see setCompactLayout(). While
    // compact, the menu bar is hidden and the top-level menus above are
    // reached through m_menuButton, every dock
    // is tabified into one group, and the toolbars are hidden.
    // The two layouts each keep their own saveState(): m_expandedState /
    // m_compactState hold the one not currently applied, and both are
    // persisted (Configuration::general.windowState / windowStateCompact).
    // m_layoutRestored gates the switch until the first show has realized
    // the layout readSettings() restored, so nothing earlier can capture
    // or clobber a layout.
    QByteArray m_expandedState;
    QByteArray m_compactState;
    // The constructor's arrangement, for "Reset Window Layout".
    QByteArray m_defaultExpandedState;
    bool m_compact = false;
    bool m_layoutRestored = false;

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
    QAction *newcomerGuideAct = nullptr;
    QAction *newbieAct = nullptr;
    QAction *actionReportIssue = nullptr;
    QAction *aboutAct = nullptr;
    QAction *aboutQtAct = nullptr;
    QAction *zoomInAct = nullptr;
    QAction *zoomOutAct = nullptr;
    QAction *zoomResetAct = nullptr;
    QAction *alwaysOnTopAct = nullptr;
    QAction *showStatusBarAct = nullptr;
    QAction *compactLayoutAct = nullptr;
    QAction *resetWindowLayoutAct = nullptr;
    QAction *showScrollBarsAct = nullptr;
    QAction *showMenuBarAct = nullptr;
    QAction *preferencesAct = nullptr;

    QAction *layerUpAct = nullptr;
    QAction *layerDownAct = nullptr;
    QAction *layerResetAct = nullptr;
    QAction *centerOnPlayerAct = nullptr;

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
    NODISCARD RemoteEdit &getRemoteEdit() const { return deref(m_remoteEdit); }

private:
    // ProxyHost
    void virt_log(const QString &mod, const QString &msg) final { slot_log(mod, msg); }
    void virt_setMode(const MapModeEnum mode) final { slot_setMode(mode); }
    NODISCARD HotkeyManager &virt_getHotkeyManager() const final { return getHotkeyManager(); }
    NODISCARD QObject &virt_asQObject() final { return *this; }

public:
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
    void setCompactLayout(bool compact);
    void buildDefaultCompactLayout();
    void fitClientDockToTerminal();
    void slot_resetWindowLayout();
    void applyCompactMenuBar(bool compact);
    NODISCARD QWidget *createCompactActionBar();
    NODISCARD QToolButton *createMenuButton();
    void applyCompactChrome(bool compact);
    void applyPanelScrollGesture(bool compact);
    NODISCARD QSize compactLayoutProbeSize() const;
    void fitCompactWindowToScreen();

private:
    void startServices();
    void forceNewFile();
    void promptOpenFile();
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
    NODISCARD static QList<QAction *> collectActions(const QMenu &menu);
    void setupToolBars();
    void setupStatusBar();

    void readSettings();
    void writeSettings();

    // Prompts to save a modified map, then runs onProceed unless the user
    // cancels (or saving fails). Non-blocking: the prompt uses
    // QMessageBox::open(), since nested event loops are unavailable on wasm.
    void maybeSave(std::function<void()> onProceed);
    // Blocking variant for closeEvent(), which cannot be deferred. Desktop only.
    NODISCARD bool maybeSaveBlocking();
    NODISCARD bool handleMaybeSaveResult(int result);

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

    NODISCARD MapCanvasWindow *getCanvas() const;
    void mapChanged() const;
    void setCanvasMouseMode(CanvasMouseModeEnum mode);
    void setMapModified(bool);
    void updateMapModified();

private:
    // Pushes the given room's description into m_descriptionWidget.
    void updateDescriptionRoom(const RoomHandle &room);
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
    void slot_aboutQt();

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
