// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mainwindow.h"

#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <QActionGroup>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHostAddress>
#include <QIcon>
#include <QProgressDialog>
#include <QSize>
#include <QString>
#include <QTextBrowser>
#include <QtWidgets>

#include "../client/ClientWidget.h"
#include "../clock/mumeclock.h"
#include "../clock/mumeclockwidget.h"
#include "../configuration/configuration.h"
#include "../display/InfoMarkSelection.h"
#include "../display/MapCanvasData.h"
#include "../display/connectionselection.h"
#include "../display/mapcanvas.h"
#include "../display/mapwindow.h"
#include "../display/prespammedpath.h"
#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/parseevent.h"
#include "../expandoracommon/room.h"
#include "../global/Debug.h"
#include "../global/NullPointerException.h"
#include "../global/SignalBlocker.h"
#include "../global/Version.h"
#include "../global/roomid.h"
#include "../logger/autologger.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/customaction.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomselection.h"
#include "../mapfrontend/mapaction.h"
#include "../mapfrontend/mapfrontend.h"
#include "../mapstorage/MmpMapStorage.h"
#include "../mapstorage/PandoraMapStorage.h"
#include "../mapstorage/XmlMapStorage.h"
#include "../mapstorage/abstractmapstorage.h"
#include "../mapstorage/filesaver.h"
#include "../mapstorage/jsonmapstorage.h"
#include "../mapstorage/mapstorage.h"
#include "../mapstorage/progresscounter.h"
#include "../pandoragroup/groupwidget.h"
#include "../pandoragroup/mmapper2group.h"
#include "../parser/DoorAction.h"
#include "../parser/abstractparser.h"
#include "../pathmachine/mmapper2pathmachine.h"
#include "../pathmachine/pathmachine.h"
#include "../preferences/configdialog.h"
#include "../proxy/connectionlistener.h"
#include "../proxy/telnetfilter.h"
#include "../roompanel/RoomManager.h"
#include "../roompanel/RoomWidget.h"
#include "UpdateDialog.h"
#include "aboutdialog.h"
#include "findroomsdlg.h"
#include "infomarkseditdlg.h"
#include "roomeditattrdlg.h"

class RoomRecipient;

class NODISCARD CanvasDisabler final
{
private:
    MapCanvas &canvas;

public:
    explicit CanvasDisabler(MapCanvas &in_canvas)
        : canvas{in_canvas}
    {
        canvas.setEnabled(false);
    }
    ~CanvasDisabler() { canvas.setEnabled(true); }
    DELETE_CTORS_AND_ASSIGN_OPS(CanvasDisabler);
};

static void addApplicationFont()
{
    const auto id = QFontDatabase::addApplicationFont(":/fonts/DejaVuSansMono.ttf");
    const auto family = QFontDatabase::applicationFontFamilies(id);
    if (family.isEmpty()) {
        qWarning() << "Unable to load bundled DejaVuSansMono font";
    } else {
        // Utilize the application font here because we can gaurantee that resources have been loaded
        // REVISIT: Move this to the configuration?
        if (getConfig().integratedClient.font.isEmpty()) {
            QFont defaultClientFont;
            defaultClientFont.setFamily(family.front());
            if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac)
                defaultClientFont.setPointSize(12);
            else
                defaultClientFont.setPointSize(10);
            defaultClientFont.setStyleStrategy(QFont::PreferAntialias);
            setConfig().integratedClient.font = defaultClientFont.toString();
        }
    }
}

class MapZoomSlider final : public QSlider
{
private:
    static constexpr float SCALE = 100.f;
    static constexpr float INV_SCALE = 1.f / SCALE;

    // can't get this to work as constexpr, so we'll just inline the static min/max.
    static int calcPos(const float zoom) noexcept
    {
        static const float INV_DIVISOR = 1.f / std::log2(ScaleFactor::ZOOM_STEP);
        return static_cast<int>(std::lround(SCALE * std::log2(zoom) * INV_DIVISOR));
    }
    static inline const int min = calcPos(ScaleFactor::MIN_VALUE);
    static inline const int max = calcPos(ScaleFactor::MAX_VALUE);
    MapWindow &m_map;

public:
    explicit MapZoomSlider(MapWindow &map, QWidget *const parent)
        : QSlider(Qt::Orientation::Horizontal, parent)
        , m_map{map}
    {
        setRange(min, max);
        setFromActual();

        connect(this, &QSlider::valueChanged, this, [this](int /*value*/) {
            requestChange();
            setFromActual();
        });

        connect(&map, &MapWindow::sig_zoomChanged, this, [this](float) { setFromActual(); });
        setToolTip("Zoom");
    }
    ~MapZoomSlider() final;

public:
    void requestChange()
    {
        const float desiredZoomSteps = static_cast<float>(clamp(value())) * INV_SCALE;
        {
            const SignalBlocker block{*this};
            m_map.setZoom(std::pow(ScaleFactor::ZOOM_STEP, desiredZoomSteps));
        }
        m_map.slot_graphicsSettingsChanged();
    }

    void setFromActual()
    {
        const float actualZoom = m_map.getZoom();
        const int rounded = calcPos(actualZoom);
        {
            const SignalBlocker block{*this};
            setValue(clamp(rounded));
        }
    }

private:
    static int clamp(int val) { return std::clamp(val, min, max); }
};

MapZoomSlider::~MapZoomSlider() = default;

MainWindow::MainWindow()
    : QMainWindow(nullptr, Qt::WindowFlags{})
{
    setObjectName("MainWindow");
    setWindowIcon(QIcon(":/icons/m.png"));
    addApplicationFont();

    qRegisterMetaType<RoomId>("RoomId");
    qRegisterMetaType<TelnetData>("TelnetData");
    qRegisterMetaType<CommandQueue>("CommandQueue");
    qRegisterMetaType<DoorActionEnum>("DoorActionEnum");
    qRegisterMetaType<ExitDirEnum>("ExitDirEnum");
    qRegisterMetaType<GroupManagerStateEnum>("GroupManagerStateEnum");
    qRegisterMetaType<SigParseEvent>("SigParseEvent");
    qRegisterMetaType<SigRoomSelection>("SigRoomSelection");

    m_mapData = new MapData(this);
    auto &mapData = *m_mapData;

    m_mapData->setObjectName("MapData");
    setCurrentFile("");

    m_prespammedPath = new PrespammedPath(this);

    m_groupManager = new Mmapper2Group(this);
    m_groupManager->setObjectName("GroupManager");

    m_mapWindow = new MapWindow(mapData, deref(m_prespammedPath), deref(m_groupManager), this);
    setCentralWidget(m_mapWindow);

    m_pathMachine = new Mmapper2PathMachine(m_mapData, this);
    m_pathMachine->setObjectName("Mmapper2PathMachine");

    m_clientWidget = new ClientWidget(this);
    m_clientWidget->setObjectName("InternalMudClientWidget");
    m_dockDialogClient = new QDockWidget("Client Panel", this);
    m_dockDialogClient->setObjectName("DockWidgetClient");
    m_dockDialogClient->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_dockDialogClient->setFeatures(QDockWidget::DockWidgetMovable
                                    | QDockWidget::DockWidgetFloatable
                                    | QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::LeftDockWidgetArea, m_dockDialogClient);
    m_dockDialogClient->setWidget(m_clientWidget);

    m_dockDialogLog = new QDockWidget(tr("Log Panel"), this);
    m_dockDialogLog->setObjectName("DockWidgetLog");
    m_dockDialogLog->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    m_dockDialogLog->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable
                                 | QDockWidget::DockWidgetClosable);
    m_dockDialogLog->toggleViewAction()->setShortcut(tr("Ctrl+L"));

    addDockWidget(Qt::BottomDockWidgetArea, m_dockDialogLog);

    logWindow = new QTextBrowser(m_dockDialogLog);
    logWindow->setObjectName("LogWindow");
    m_dockDialogLog->setWidget(logWindow);
    m_dockDialogLog->hide();

    m_groupWidget = new GroupWidget(m_groupManager, m_mapData, this);
    m_dockDialogGroup = new QDockWidget(tr("Group Panel"), this);
    m_dockDialogGroup->setObjectName("DockWidgetGroup");
    m_dockDialogGroup->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    m_dockDialogGroup->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable
                                   | QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::TopDockWidgetArea, m_dockDialogGroup);
    m_dockDialogGroup->setWidget(m_groupWidget);
    m_dockDialogGroup->hide();
    connect(m_groupWidget, &GroupWidget::sig_center, m_mapWindow, &MapWindow::slot_centerOnWorldPos);

    m_roomManager = new RoomManager(this);
    m_roomManager->setObjectName("RoomManager");
    m_roomWidget = new RoomWidget(deref(m_roomManager), this);
    m_dockDialogRoom = new QDockWidget(tr("Room Panel"), this);
    m_dockDialogRoom->setObjectName("DockWidgetRoom");
    m_dockDialogRoom->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    m_dockDialogRoom->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable
                                  | QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::BottomDockWidgetArea, m_dockDialogRoom);
    m_dockDialogRoom->setWidget(m_roomWidget);
    m_dockDialogRoom->hide();

    m_findRoomsDlg = new FindRoomsDlg(mapData, this);
    m_findRoomsDlg->setObjectName("FindRoomsDlg");

    m_mumeClock = new MumeClock(getConfig().mumeClock.startEpoch, this);
    if constexpr (!NO_UPDATER)
        m_updateDialog = new UpdateDialog(this);

    createActions();
    setupToolBars();
    setupMenuBar();
    setupStatusBar();

    setCorner(Qt::TopLeftCorner, Qt::TopDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::TopDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    m_logger = new AutoLogger(this);

    m_listener = new ConnectionListener(mapData,
                                        deref(m_pathMachine),
                                        deref(m_prespammedPath),
                                        deref(m_groupManager),
                                        deref(m_roomManager),
                                        deref(m_mumeClock),
                                        deref(m_logger),
                                        deref(getCanvas()),
                                        this);

    // update connections
    wireConnections();
    setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                    Qt::AlignCenter,
                                    size(),
                                    qApp->primaryScreen()->availableGeometry()));

    switch (getConfig().general.mapMode) {
    case MapModeEnum::PLAY:
        mapperMode.playModeAct->setChecked(true);
        slot_onPlayMode();
        break;
    case MapModeEnum::MAP:
        mapperMode.mapModeAct->setChecked(true);
        slot_onMapMode();
        break;
    case MapModeEnum::OFFLINE:
        mapperMode.offlineModeAct->setChecked(true);
        slot_onOfflineMode();
        break;
    }

    if (getConfig().general.alwaysOnTop) {
        alwaysOnTopAct->setChecked(true);
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }
}

// depth-first recursively disconnect all children
static void rdisconnect(QObject *const obj)
{
    auto &o = deref(obj);
    for (QObject *const child : o.children()) {
        rdisconnect(child);
    }
    o.disconnect();
}

MainWindow::~MainWindow()
{
    forceNewFile();
    rdisconnect(this);
}

void MainWindow::startServices()
{
    try {
        m_listener->listen();
        slot_log("ConnectionListener",
                 tr("Server bound on localhost to port: %1.").arg(getConfig().connection.localPort));
    } catch (const std::exception &e) {
        const QString errorMsg = QString(
                                     "Unable to start the server (switching to offline mode): %1.")
                                     .arg(QString::fromLatin1(e.what()));
        QMessageBox::critical(this, tr("mmapper"), errorMsg);
    }

    m_groupManager->start();
    const auto &groupConfig = getConfig().groupManager;
    groupNetwork.networkStopAct->setChecked(true);
    switch (groupConfig.state) {
    case GroupManagerStateEnum::Off:
        groupMode.groupOffAct->setChecked(true);
        slot_onModeGroupOff();
        break;
    case GroupManagerStateEnum::Client:
        groupMode.groupClientAct->setChecked(true);
        slot_onModeGroupClient();
        break;
    case GroupManagerStateEnum::Server:
        groupMode.groupServerAct->setChecked(true);
        slot_onModeGroupServer();
        break;
    }
    if (groupConfig.state != GroupManagerStateEnum::Off && groupConfig.autoStart)
        groupNetwork.networkStartAct->trigger();

    if constexpr (!NO_UPDATER) {
        // Raise the update dialog if an update is found
        if (getConfig().general.checkForUpdate)
            m_updateDialog->open();
    }
}

void MainWindow::readSettings()
{
    const auto &settings = getConfig().general;
    if (settings.firstRun) {
        adjustSize();
        setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                        Qt::AlignCenter,
                                        size(),
                                        qApp->primaryScreen()->availableGeometry()));

    } else {
        if (!restoreGeometry(settings.windowGeometry))
            qWarning() << "Unable to restore window geometry";
        if (!restoreState(settings.windowState))
            qWarning() << "Unable to restore toolbars and dockwidgets state";

        // Check if the window was moved to a screen with a different DPI
        getCanvas()->screenChanged();
    }

    m_dockDialogClient->setHidden(settings.noClientPanel);
}

void MainWindow::writeSettings()
{
    auto &savedConfig = setConfig().general;
    savedConfig.windowGeometry = saveGeometry();
    savedConfig.windowState = saveState();
}

void MainWindow::wireConnections()
{
    connect(m_pathMachine, &Mmapper2PathMachine::sig_log, this, &MainWindow::slot_log);

    connect(m_pathMachine,
            QOverload<RoomRecipient &, const Coordinate &>::of(
                &Mmapper2PathMachine::sig_lookingForRooms),
            m_mapData,
            QOverload<RoomRecipient &, const Coordinate &>::of(&MapData::lookingForRooms));
    connect(m_pathMachine,
            QOverload<RoomRecipient &, const SigParseEvent &>::of(
                &Mmapper2PathMachine::sig_lookingForRooms),
            m_mapData,
            QOverload<RoomRecipient &, const SigParseEvent &>::of(&MapData::lookingForRooms));
    connect(m_pathMachine,
            QOverload<RoomRecipient &, RoomId>::of(&Mmapper2PathMachine::sig_lookingForRooms),
            m_mapData,
            QOverload<RoomRecipient &, RoomId>::of(&MapData::lookingForRooms));
    connect(m_mapData,
            &MapFrontend::sig_clearingMap,
            m_pathMachine,
            &PathMachine::slot_releaseAllPaths);

    MapCanvas *const canvas = getCanvas();
    connect(m_mapData, &MapFrontend::sig_clearingMap, canvas, &MapCanvas::slot_clearAllSelections);

    connect(m_pathMachine,
            &Mmapper2PathMachine::sig_playerMoved,
            canvas,
            &MapCanvas::slot_moveMarker);

    connect(canvas,
            &MapCanvas::sig_setCurrentRoom,
            m_pathMachine,
            &PathMachine::slot_setCurrentRoom);

    // moved to mapwindow
    connect(m_mapData, &MapData::sig_mapSizeChanged, m_mapWindow, &MapWindow::slot_setScrollBars);

    connect(m_prespammedPath, &PrespammedPath::sig_update, canvas, &MapCanvas::slot_requestUpdate);

    connect(m_mapData, &MapData::sig_log, this, &MainWindow::slot_log);
    connect(canvas, &MapCanvas::sig_log, this, &MainWindow::slot_log);

    connect(m_mapData, &MapData::sig_onDataChanged, this, [this]() {
        setWindowModified(true);
        saveAct->setEnabled(true);
    });

    connect(zoomInAct, &QAction::triggered, canvas, &MapCanvas::slot_zoomIn);
    connect(zoomOutAct, &QAction::triggered, canvas, &MapCanvas::slot_zoomOut);
    connect(zoomResetAct, &QAction::triggered, canvas, &MapCanvas::slot_zoomReset);

    connect(canvas, &MapCanvas::sig_newRoomSelection, this, &MainWindow::slot_newRoomSelection);
    connect(canvas,
            &MapCanvas::sig_newConnectionSelection,
            this,
            &MainWindow::slot_newConnectionSelection);
    connect(canvas,
            &MapCanvas::sig_newInfoMarkSelection,
            this,
            &MainWindow::slot_newInfoMarkSelection);
    connect(canvas, &QWidget::customContextMenuRequested, this, &MainWindow::slot_showContextMenu);

    // Group
    connect(m_groupManager, &Mmapper2Group::sig_log, this, &MainWindow::slot_log);
    connect(m_pathMachine,
            &PathMachine::sig_setCharPosition,
            m_groupManager,
            &Mmapper2Group::slot_setCharacterRoomId,
            Qt::QueuedConnection);
    connect(m_groupManager,
            &Mmapper2Group::sig_updateMapCanvas,
            canvas,
            &MapCanvas::slot_requestUpdate,
            Qt::QueuedConnection);
    connect(this,
            &MainWindow::sig_setGroupMode,
            m_groupManager,
            &Mmapper2Group::slot_setMode,
            Qt::QueuedConnection);
    connect(m_groupManager,
            &Mmapper2Group::sig_networkStatus,
            this,
            &MainWindow::slot_groupNetworkStatus,
            Qt::QueuedConnection);

    connect(m_mapData, &MapFrontend::sig_clearingMap, m_groupWidget, &GroupWidget::slot_mapUnloaded);

    connect(m_mumeClock, &MumeClock::sig_log, this, &MainWindow::slot_log);

    connect(m_listener, &ConnectionListener::sig_log, this, &MainWindow::slot_log);
    connect(m_dockDialogClient,
            &QDockWidget::visibilityChanged,
            m_clientWidget,
            &ClientWidget::slot_onVisibilityChanged);
    connect(m_listener, &ConnectionListener::sig_clientSuccessfullyConnected, this, [this]() {
        if (!m_clientWidget->isUsingClient())
            m_dockDialogClient->hide();
    });
    connect(m_clientWidget, &ClientWidget::sig_relayMessage, this, [this](const QString &message) {
        statusBar()->showMessage(message, 2000);
    });

    // Find Room Dialog Connections
    connect(m_findRoomsDlg,
            &FindRoomsDlg::sig_newRoomSelection,
            canvas,
            &MapCanvas::slot_setRoomSelection);
    connect(m_findRoomsDlg,
            &FindRoomsDlg::sig_center,
            m_mapWindow,
            &MapWindow::slot_centerOnWorldPos);
    connect(m_findRoomsDlg, &FindRoomsDlg::sig_log, this, &MainWindow::slot_log);
    connect(m_findRoomsDlg,
            &FindRoomsDlg::sig_editSelection,
            this,
            &MainWindow::slot_onEditRoomSelection);
}

void MainWindow::slot_log(const QString &module, const QString &message)
{
    logWindow->append("[" + module + "] " + message);
    logWindow->moveCursor(QTextCursor::MoveOperation::End);
    logWindow->ensureCursorVisible();
    logWindow->update();
}

// TODO: clean up all this copy/paste by using helper functions and X-macros
void MainWindow::createActions()
{
    newAct = new QAction(QIcon::fromTheme("document-new", QIcon(":/icons/new.png")),
                         tr("&New"),
                         this);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, &QAction::triggered, this, &MainWindow::slot_newFile);

    openAct = new QAction(QIcon::fromTheme("document-open", QIcon(":/icons/open.png")),
                          tr("&Open..."),
                          this);
    openAct->setShortcut(tr("Ctrl+O"));
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::slot_open);

    reloadAct = new QAction(QIcon::fromTheme("document-open-recent", QIcon(":/icons/reload.png")),
                            tr("&Reload"),
                            this);
    reloadAct->setShortcut(tr("Ctrl+R"));
    reloadAct->setStatusTip(tr("Reload the current map"));
    connect(reloadAct, &QAction::triggered, this, &MainWindow::slot_reload);

    saveAct = new QAction(QIcon::fromTheme("document-save", QIcon(":/icons/save.png")),
                          tr("&Save"),
                          this);
    saveAct->setShortcut(tr("Ctrl+S"));
    saveAct->setStatusTip(tr("Save the document to disk"));
    saveAct->setEnabled(false);
    connect(saveAct, &QAction::triggered, this, &MainWindow::slot_save);

    saveAsAct = new QAction(QIcon::fromTheme("document-save-as"), tr("Save &As..."), this);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::slot_saveAs);

    exportBaseMapAct = new QAction(tr("Export MMapper2 &Base Map As..."), this);
    exportBaseMapAct->setStatusTip(tr("Save a copy of the map with no secrets"));
    connect(exportBaseMapAct, &QAction::triggered, this, &MainWindow::slot_exportBaseMap);

    exportMm2xmlMapAct = new QAction(tr("Export MMapper2 &XML Map As..."), this);
    exportMm2xmlMapAct->setStatusTip(tr("Save a copy of the map in the MM2XML format"));
    connect(exportMm2xmlMapAct, &QAction::triggered, this, &MainWindow::slot_exportMm2xmlMap);

    exportWebMapAct = new QAction(tr("Export &Web Map As..."), this);
    exportWebMapAct->setStatusTip(tr("Save a copy of the map for webclients"));
    connect(exportWebMapAct, &QAction::triggered, this, &MainWindow::slot_exportWebMap);

    exportMmpMapAct = new QAction(tr("Export &MMP Map As..."), this);
    exportMmpMapAct->setStatusTip(tr("Save a copy of the map in the MMP format"));
    connect(exportMmpMapAct, &QAction::triggered, this, &MainWindow::slot_exportMmpMap);

    mergeAct = new QAction(QIcon(":/icons/merge.png"), tr("&Merge..."), this);
    mergeAct->setStatusTip(tr("Merge an existing file into current map"));
    connect(mergeAct, &QAction::triggered, this, &MainWindow::slot_merge);

    exitAct = new QAction(QIcon::fromTheme("application-exit"), tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    preferencesAct = new QAction(QIcon::fromTheme("preferences-desktop",
                                                  QIcon(":/icons/preferences.png")),
                                 tr("&Preferences"),
                                 this);
    preferencesAct->setShortcut(tr("Ctrl+P"));
    preferencesAct->setStatusTip(tr("MMapper preferences"));
    connect(preferencesAct, &QAction::triggered, this, &MainWindow::slot_onPreferences);

    if constexpr (!NO_UPDATER) {
        mmapperCheckForUpdateAct = new QAction(QIcon(":/icons/m.png"),
                                               tr("Check for &update"),
                                               this);
        connect(mmapperCheckForUpdateAct,
                &QAction::triggered,
                this,
                &MainWindow::slot_onCheckForUpdate);
    }
    mumeWebsiteAct = new QAction(tr("&Website"), this);
    connect(mumeWebsiteAct, &QAction::triggered, this, &MainWindow::slot_openMumeWebsite);
    voteAct = new QAction(QIcon::fromTheme("applications-games"), tr("V&ote for Mume"), this);
    voteAct->setStatusTip(tr("Please vote for MUME on \"The Mud Connector\""));
    connect(voteAct, &QAction::triggered, this, &MainWindow::slot_voteForMUME);
    mumeWebsiteAct = new QAction(tr("&Website"), this);
    connect(mumeWebsiteAct, &QAction::triggered, this, &MainWindow::slot_openMumeWebsite);
    mumeForumAct = new QAction(tr("&Forum"), this);
    connect(mumeForumAct, &QAction::triggered, this, &MainWindow::slot_openMumeForum);
    mumeWikiAct = new QAction(tr("W&iki"), this);
    connect(mumeWikiAct, &QAction::triggered, this, &MainWindow::slot_openMumeWiki);
    settingUpMmapperAct = new QAction(QIcon::fromTheme("help-faq"), tr("Get &Help"), this);
    connect(settingUpMmapperAct, &QAction::triggered, this, &MainWindow::slot_openSettingUpMmapper);
    newbieAct = new QAction(tr("&Information for Newcomers"), this);
    newbieAct->setStatusTip("Newbie help on the MUME website");
    connect(newbieAct, &QAction::triggered, this, &MainWindow::slot_openNewbieHelp);
    aboutAct = new QAction(QIcon::fromTheme("help-about"), tr("About &MMapper"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, &QAction::triggered, this, &MainWindow::slot_about);
    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

    zoomInAct = new QAction(QIcon::fromTheme("zoom-in", QIcon(":/icons/viewmag+.png")),
                            tr("Zoom In"),
                            this);
    zoomInAct->setStatusTip(tr("Zooms In current map"));
    zoomInAct->setShortcut(tr("Ctrl++"));
    zoomOutAct = new QAction(QIcon::fromTheme("zoom-out", QIcon(":/icons/viewmag-.png")),
                             tr("Zoom Out"),
                             this);
    zoomOutAct->setShortcut(tr("Ctrl+-"));
    zoomOutAct->setStatusTip(tr("Zooms Out current map"));
    zoomResetAct = new QAction(QIcon::fromTheme("zoom-original", QIcon(":/icons/viewmagfit.png")),
                               tr("Zoom Reset"),
                               this);
    zoomResetAct->setShortcut(tr("Ctrl+0"));
    zoomResetAct->setStatusTip(tr("Zoom to original size"));

    alwaysOnTopAct = new QAction(tr("Always on top"), this);
    alwaysOnTopAct->setCheckable(true);
    connect(alwaysOnTopAct, &QAction::triggered, this, &MainWindow::slot_alwaysOnTop);

    showStatusBarAct = new QAction(tr("Status Bar"), this);
    showStatusBarAct->setCheckable(true);
    showStatusBarAct->setChecked(true);
    connect(showStatusBarAct, &QAction::triggered, this, &MainWindow::slot_setShowStatusBar);

    showScrollBarsAct = new QAction(tr("Scrollbars"), this);
    showScrollBarsAct->setCheckable(true);
    showScrollBarsAct->setChecked(true);
    connect(showScrollBarsAct, &QAction::triggered, this, &MainWindow::slot_setShowScrollBars);

    layerUpAct = new QAction(QIcon::fromTheme("go-up", QIcon(":/icons/layerup.png")),
                             tr("Layer Up"),
                             this);
    layerUpAct->setShortcut(tr([]() -> const char * {
        // Technically tr() could convert Ctrl to Meta, right?
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac)
            return "Meta+Tab";
        return "Ctrl+Tab";
    }()));
    layerUpAct->setStatusTip(tr("Layer Up"));
    connect(layerUpAct, &QAction::triggered, this, &MainWindow::slot_onLayerUp);
    layerDownAct = new QAction(QIcon::fromTheme("go-down", QIcon(":/icons/layerdown.png")),
                               tr("Layer Down"),
                               this);

    layerDownAct->setShortcut(tr([]() -> const char * {
        // Technically tr() could convert Ctrl to Meta, right?
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac)
            return "Meta+Shift+Tab";
        return "Ctrl+Shift+Tab";
    }()));
    layerDownAct->setStatusTip(tr("Layer Down"));
    connect(layerDownAct, &QAction::triggered, this, &MainWindow::slot_onLayerDown);

    layerResetAct = new QAction(QIcon::fromTheme("go-home", QIcon(":/icons/layerhome.png")),
                                tr("Layer Reset"),
                                this);
    layerResetAct->setStatusTip(tr("Layer Reset"));
    connect(layerResetAct, &QAction::triggered, this, &MainWindow::slot_onLayerReset);

    mouseMode.modeConnectionSelectAct = new QAction(QIcon(":/icons/connectionselection.png"),
                                                    tr("Select Connection"),
                                                    this);
    mouseMode.modeConnectionSelectAct->setStatusTip(tr("Select Connection"));
    mouseMode.modeConnectionSelectAct->setCheckable(true);
    connect(mouseMode.modeConnectionSelectAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onModeConnectionSelect);

    mouseMode.modeRoomRaypickAct = new QAction(QIcon(":/icons/raypick.png"),
                                               tr("Ray-pick Rooms"),
                                               this);
    mouseMode.modeRoomRaypickAct->setStatusTip(tr("Ray-pick Rooms"));
    mouseMode.modeRoomRaypickAct->setCheckable(true);
    connect(mouseMode.modeRoomRaypickAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onModeRoomRaypick);

    mouseMode.modeRoomSelectAct = new QAction(QIcon(":/icons/roomselection.png"),
                                              tr("Select Rooms"),
                                              this);
    mouseMode.modeRoomSelectAct->setStatusTip(tr("Select Rooms"));
    mouseMode.modeRoomSelectAct->setCheckable(true);
    connect(mouseMode.modeRoomSelectAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onModeRoomSelect);

    mouseMode.modeMoveSelectAct = new QAction(QIcon(":/icons/mapmove.png"), tr("Move map"), this);
    mouseMode.modeMoveSelectAct->setStatusTip(tr("Move Map"));
    mouseMode.modeMoveSelectAct->setCheckable(true);
    connect(mouseMode.modeMoveSelectAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onModeMoveSelect);
    mouseMode.modeInfoMarkSelectAct = new QAction(QIcon(":/icons/infomarkselection.png"),
                                                  tr("Select Markers"),
                                                  this);
    mouseMode.modeInfoMarkSelectAct->setStatusTip(tr("Select Info Markers"));
    mouseMode.modeInfoMarkSelectAct->setCheckable(true);
    connect(mouseMode.modeInfoMarkSelectAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onModeInfoMarkSelect);
    mouseMode.modeCreateInfoMarkAct = new QAction(QIcon(":/icons/infomarkcreate.png"),
                                                  tr("Create New Markers"),
                                                  this);
    mouseMode.modeCreateInfoMarkAct->setStatusTip(tr("Create New Info Markers"));
    mouseMode.modeCreateInfoMarkAct->setCheckable(true);
    connect(mouseMode.modeCreateInfoMarkAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onModeCreateInfoMarkSelect);
    mouseMode.modeCreateRoomAct = new QAction(QIcon(":/icons/roomcreate.png"),
                                              tr("Create New Rooms"),
                                              this);
    mouseMode.modeCreateRoomAct->setStatusTip(tr("Create New Rooms"));
    mouseMode.modeCreateRoomAct->setCheckable(true);
    connect(mouseMode.modeCreateRoomAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onModeCreateRoomSelect);

    mouseMode.modeCreateConnectionAct = new QAction(QIcon(":/icons/connectioncreate.png"),
                                                    tr("Create New Connection"),
                                                    this);
    mouseMode.modeCreateConnectionAct->setStatusTip(tr("Create New Connection"));
    mouseMode.modeCreateConnectionAct->setCheckable(true);
    connect(mouseMode.modeCreateConnectionAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onModeCreateConnectionSelect);

    mouseMode.modeCreateOnewayConnectionAct = new QAction(QIcon(
                                                              ":/icons/onewayconnectioncreate.png"),
                                                          tr("Create New Oneway Connection"),
                                                          this);
    mouseMode.modeCreateOnewayConnectionAct->setStatusTip(tr("Create New Oneway Connection"));
    mouseMode.modeCreateOnewayConnectionAct->setCheckable(true);
    connect(mouseMode.modeCreateOnewayConnectionAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onModeCreateOnewayConnectionSelect);

    mouseMode.mouseModeActGroup = new QActionGroup(this);
    mouseMode.mouseModeActGroup->setExclusive(true);
    mouseMode.mouseModeActGroup->addAction(mouseMode.modeMoveSelectAct);
    mouseMode.mouseModeActGroup->addAction(mouseMode.modeRoomRaypickAct);
    mouseMode.mouseModeActGroup->addAction(mouseMode.modeRoomSelectAct);
    mouseMode.mouseModeActGroup->addAction(mouseMode.modeConnectionSelectAct);
    mouseMode.mouseModeActGroup->addAction(mouseMode.modeCreateRoomAct);
    mouseMode.mouseModeActGroup->addAction(mouseMode.modeCreateConnectionAct);
    mouseMode.mouseModeActGroup->addAction(mouseMode.modeCreateOnewayConnectionAct);
    mouseMode.mouseModeActGroup->addAction(mouseMode.modeInfoMarkSelectAct);
    mouseMode.mouseModeActGroup->addAction(mouseMode.modeCreateInfoMarkAct);
    mouseMode.modeMoveSelectAct->setChecked(true);

    createRoomAct = new QAction(QIcon(":/icons/roomcreate.png"), tr("Create New Room"), this);
    createRoomAct->setStatusTip(tr("Create a new room under the cursor"));
    connect(createRoomAct, &QAction::triggered, this, &MainWindow::slot_onCreateRoom);

    editRoomSelectionAct = new QAction(QIcon(":/icons/roomedit.png"),
                                       tr("Edit Selected Rooms"),
                                       this);
    editRoomSelectionAct->setStatusTip(tr("Edit Selected Rooms"));
    editRoomSelectionAct->setShortcut(tr("Ctrl+E"));
    connect(editRoomSelectionAct, &QAction::triggered, this, &MainWindow::slot_onEditRoomSelection);

    deleteRoomSelectionAct = new QAction(QIcon(":/icons/roomdelete.png"),
                                         tr("Delete Selected Rooms"),
                                         this);
    deleteRoomSelectionAct->setStatusTip(tr("Delete Selected Rooms"));
    connect(deleteRoomSelectionAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onDeleteRoomSelection);

    moveUpRoomSelectionAct = new QAction(QIcon(":/icons/roommoveup.png"),
                                         tr("Move Up Selected Rooms"),
                                         this);
    moveUpRoomSelectionAct->setStatusTip(tr("Move Up Selected Rooms"));
    connect(moveUpRoomSelectionAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onMoveUpRoomSelection);
    moveDownRoomSelectionAct = new QAction(QIcon(":/icons/roommovedown.png"),
                                           tr("Move Down Selected Rooms"),
                                           this);
    moveDownRoomSelectionAct->setStatusTip(tr("Move Down Selected Rooms"));
    connect(moveDownRoomSelectionAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onMoveDownRoomSelection);
    mergeUpRoomSelectionAct = new QAction(QIcon(":/icons/roommergeup.png"),
                                          tr("Merge Up Selected Rooms"),
                                          this);
    mergeUpRoomSelectionAct->setStatusTip(tr("Merge Up Selected Rooms"));
    connect(mergeUpRoomSelectionAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onMergeUpRoomSelection);
    mergeDownRoomSelectionAct = new QAction(QIcon(":/icons/roommergedown.png"),
                                            tr("Merge Down Selected Rooms"),
                                            this);
    mergeDownRoomSelectionAct->setStatusTip(tr("Merge Down Selected Rooms"));
    connect(mergeDownRoomSelectionAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onMergeDownRoomSelection);
    connectToNeighboursRoomSelectionAct = new QAction(QIcon(":/icons/roomconnecttoneighbours.png"),
                                                      tr("Connect room(s) to its neighbour rooms"),
                                                      this);
    connectToNeighboursRoomSelectionAct->setStatusTip(tr("Connect room(s) to its neighbour rooms"));
    connect(connectToNeighboursRoomSelectionAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onConnectToNeighboursRoomSelection);

    findRoomsAct = new QAction(QIcon(":/icons/roomfind.png"), tr("&Find Rooms"), this);
    findRoomsAct->setStatusTip(tr("Find matching rooms"));
    findRoomsAct->setShortcut(tr("Ctrl+F"));
    connect(findRoomsAct, &QAction::triggered, this, &MainWindow::slot_onFindRoom);

    clientAct = new QAction(QIcon(":/icons/online.png"), tr("&Launch mud client"), this);
    clientAct->setStatusTip(tr("Launch the integrated mud client"));
    connect(clientAct, &QAction::triggered, this, &MainWindow::slot_onLaunchClient);

    saveLogAct = new QAction(QIcon::fromTheme("document-save", QIcon(":/icons/save.png")),
                             tr("&Save log as..."),
                             this);
    connect(saveLogAct, &QAction::triggered, m_clientWidget, &ClientWidget::slot_saveLog);
    saveLogAct->setStatusTip(tr("Save log as file"));

    releaseAllPathsAct = new QAction(QIcon(":/icons/cancel.png"), tr("Release All Paths"), this);
    releaseAllPathsAct->setStatusTip(tr("Release all paths"));
    releaseAllPathsAct->setCheckable(false);
    connect(releaseAllPathsAct,
            &QAction::triggered,
            m_pathMachine,
            &PathMachine::slot_releaseAllPaths);

    forceRoomAct = new QAction(QIcon(":/icons/force.png"),
                               tr("Force update selected room with last movement"),
                               this);
    forceRoomAct->setStatusTip(tr("Force update selected room with last movement"));
    forceRoomAct->setCheckable(false);
    forceRoomAct->setEnabled(false);
    connect(forceRoomAct, &QAction::triggered, getCanvas(), &MapCanvas::slot_forceMapperToRoom);

    selectedRoomActGroup = new QActionGroup(this);
    selectedRoomActGroup->setExclusive(false);
    selectedRoomActGroup->addAction(editRoomSelectionAct);
    selectedRoomActGroup->addAction(deleteRoomSelectionAct);
    selectedRoomActGroup->addAction(moveUpRoomSelectionAct);
    selectedRoomActGroup->addAction(moveDownRoomSelectionAct);
    selectedRoomActGroup->addAction(mergeUpRoomSelectionAct);
    selectedRoomActGroup->addAction(mergeDownRoomSelectionAct);
    selectedRoomActGroup->addAction(connectToNeighboursRoomSelectionAct);
    selectedRoomActGroup->setEnabled(false);

    deleteConnectionSelectionAct = new QAction(QIcon(":/icons/connectiondelete.png"),
                                               tr("Delete Selected Connection"),
                                               this);
    deleteConnectionSelectionAct->setStatusTip(tr("Delete Selected Connection"));
    connect(deleteConnectionSelectionAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onDeleteConnectionSelection);

    selectedConnectionActGroup = new QActionGroup(this);
    selectedConnectionActGroup->setExclusive(false);
    selectedConnectionActGroup->addAction(deleteConnectionSelectionAct);
    selectedConnectionActGroup->setEnabled(false);

    infoMarkActions.editInfoMarkAct = new QAction(QIcon(":/icons/infomarkedit.png"),
                                                  tr("Edit Selected Markers"),
                                                  this);
    infoMarkActions.editInfoMarkAct->setStatusTip(tr("Edit Selected Info Markers"));
    infoMarkActions.editInfoMarkAct->setCheckable(true);
    connect(infoMarkActions.editInfoMarkAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onEditInfoMarkSelection);
    infoMarkActions.deleteInfoMarkAct = new QAction(QIcon(":/icons/infomarkdelete.png"),
                                                    tr("Delete Selected Markers"),
                                                    this);
    infoMarkActions.deleteInfoMarkAct->setStatusTip(tr("Delete Selected Info Markers"));
    infoMarkActions.deleteInfoMarkAct->setCheckable(true);
    connect(infoMarkActions.deleteInfoMarkAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onDeleteInfoMarkSelection);

    infoMarkActions.infoMarkGroup = new QActionGroup(this);
    infoMarkActions.infoMarkGroup->setExclusive(false);
    infoMarkActions.infoMarkGroup->addAction(infoMarkActions.deleteInfoMarkAct);
    infoMarkActions.infoMarkGroup->addAction(infoMarkActions.editInfoMarkAct);
    infoMarkActions.infoMarkGroup->setEnabled(false);

    mapperMode.playModeAct = new QAction(QIcon(":/icons/online.png"),
                                         tr("Switch to play mode"),
                                         this);
    mapperMode.playModeAct->setStatusTip(tr("Switch to play mode - no new rooms are created"));
    mapperMode.playModeAct->setCheckable(true);
    connect(mapperMode.playModeAct, &QAction::triggered, this, &MainWindow::slot_onPlayMode);

    mapperMode.mapModeAct = new QAction(QIcon(":/icons/map.png"),
                                        tr("Switch to mapping mode"),
                                        this);
    mapperMode.mapModeAct->setStatusTip(
        tr("Switch to mapping mode - new rooms are created when moving"));
    mapperMode.mapModeAct->setCheckable(true);
    connect(mapperMode.mapModeAct, &QAction::triggered, this, &MainWindow::slot_onMapMode);

    mapperMode.offlineModeAct = new QAction(QIcon(":/icons/play.png"),
                                            tr("Switch to offline emulation mode"),
                                            this);
    mapperMode.offlineModeAct->setStatusTip(
        tr("Switch to offline emulation mode - you can learn areas offline"));
    mapperMode.offlineModeAct->setCheckable(true);
    connect(mapperMode.offlineModeAct, &QAction::triggered, this, &MainWindow::slot_onOfflineMode);

    mapperMode.mapModeActGroup = new QActionGroup(this);
    mapperMode.mapModeActGroup->setExclusive(true);
    mapperMode.mapModeActGroup->addAction(mapperMode.playModeAct);
    mapperMode.mapModeActGroup->addAction(mapperMode.mapModeAct);
    mapperMode.mapModeActGroup->addAction(mapperMode.offlineModeAct);
    mapperMode.mapModeActGroup->setEnabled(true);

    // Group Manager
    groupMode.groupOffAct = new QAction(QIcon(":/icons/groupoff.png"),
                                        tr("Switch to &offline mode"),
                                        this);
    groupMode.groupOffAct->setCheckable(true);
    groupMode.groupOffAct->setStatusTip(tr("Switch to offline mode - Group Manager is disabled"));
    connect(groupMode.groupOffAct, &QAction::triggered, this, &MainWindow::slot_onModeGroupOff);

    groupMode.groupClientAct = new QAction(QIcon(":/icons/groupclient.png"),
                                           tr("Switch to &client mode"),
                                           this);
    groupMode.groupClientAct->setCheckable(true);
    groupMode.groupClientAct->setStatusTip(tr("Switch to client mode - connect to a friend's map"));
    connect(groupMode.groupClientAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onModeGroupClient);

    groupMode.groupServerAct = new QAction(QIcon(":/icons/groupserver.png"),
                                           tr("Switch to &host mode"),
                                           this);
    groupMode.groupServerAct->setCheckable(true);
    groupMode.groupServerAct->setStatusTip(
        tr("Switch to host mode - allow friends to connect to your map"));
    connect(groupMode.groupServerAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onModeGroupServer);

    groupMode.groupModeGroup = new QActionGroup(this);
    groupMode.groupModeGroup->setExclusive(true);
    groupMode.groupModeGroup->addAction(groupMode.groupOffAct);
    groupMode.groupModeGroup->addAction(groupMode.groupClientAct);
    groupMode.groupModeGroup->addAction(groupMode.groupServerAct);

    groupNetwork.networkStartAct = new QAction(QIcon(":/icons/online.png"), tr("Start"), this);
    groupNetwork.networkStartAct->setCheckable(true);
    groupNetwork.networkStartAct->setShortcut(tr("Ctrl+G"));
    groupNetwork.networkStartAct->setStatusTip(tr("Start the Group Manager"));
    groupNetwork.networkStopAct = new QAction(QIcon(":/icons/offline.png"), tr("Stop"), this);
    groupNetwork.networkStopAct->setCheckable(true);
    groupNetwork.networkStopAct->setStatusTip(tr("Stop the Group Manager"));
    connect(groupNetwork.networkStartAct,
            &QAction::triggered,
            m_groupManager,
            &Mmapper2Group::slot_startNetwork);
    connect(groupNetwork.networkStopAct,
            &QAction::triggered,
            m_groupManager,
            &Mmapper2Group::slot_stopNetwork);

    groupNetwork.groupNetworkGroup = new QActionGroup(this);
    groupNetwork.groupNetworkGroup->setExclusive(true);
    groupNetwork.groupNetworkGroup->addAction(groupNetwork.networkStartAct);
    groupNetwork.groupNetworkGroup->addAction(groupNetwork.networkStopAct);

    rebuildMeshesAct = new QAction(QIcon(":/icons/graphicscfg.png"), tr("&Rebuild world"), this);
    rebuildMeshesAct->setStatusTip(tr("Reconstruct the world mesh to fix graphical rendering bugs"));
    rebuildMeshesAct->setCheckable(false);
    connect(rebuildMeshesAct, &QAction::triggered, getCanvas(), &MapCanvas::mapAndInfomarksChanged);
}

void MainWindow::slot_onPlayMode()
{
    disconnect(m_pathMachine,
               &Mmapper2PathMachine::sig_createRoom,
               m_mapData,
               &MapData::slot_createRoom);
    disconnect(m_pathMachine,
               &Mmapper2PathMachine::sig_scheduleAction,
               m_mapData,
               &MapData::slot_scheduleAction);
    setConfig().general.mapMode = MapModeEnum::PLAY;
    modeMenu->setIcon(mapperMode.playModeAct->icon());
    // needed so that the menu updates to reflect state set by commands
    mapperMode.playModeAct->setChecked(true);
}

void MainWindow::slot_onMapMode()
{
    slot_log("MainWindow",
             "Map mode selected - new rooms are created when entering unmapped areas.");
    connect(m_pathMachine,
            &Mmapper2PathMachine::sig_createRoom,
            m_mapData,
            &MapData::slot_createRoom);
    connect(m_pathMachine,
            &Mmapper2PathMachine::sig_scheduleAction,
            m_mapData,
            &MapData::slot_scheduleAction);
    setConfig().general.mapMode = MapModeEnum::MAP;
    modeMenu->setIcon(mapperMode.mapModeAct->icon());
    // needed so that the menu updates to reflect state set by commands
    mapperMode.mapModeAct->setChecked(true);
}

void MainWindow::slot_onOfflineMode()
{
    slot_log("MainWindow", "Offline emulation mode selected - learn new areas safely.");
    disconnect(m_pathMachine,
               &Mmapper2PathMachine::sig_createRoom,
               m_mapData,
               &MapData::slot_createRoom);
    disconnect(m_pathMachine,
               &Mmapper2PathMachine::sig_scheduleAction,
               m_mapData,
               &MapData::slot_scheduleAction);
    setConfig().general.mapMode = MapModeEnum::OFFLINE;
    modeMenu->setIcon(mapperMode.offlineModeAct->icon());
    // needed so that the menu updates to reflect state set by commands
    mapperMode.offlineModeAct->setChecked(true);
}

void MainWindow::slot_setMode(MapModeEnum mode)
{
    switch (mode) {
    case MapModeEnum::PLAY:
        slot_onPlayMode();
        break;
    case MapModeEnum::MAP:
        slot_onMapMode();
        break;
    case MapModeEnum::OFFLINE:
        slot_onOfflineMode();
        break;
    }
}

void MainWindow::disableActions(bool value)
{
    newAct->setDisabled(value);
    openAct->setDisabled(value);
    mergeAct->setDisabled(value);
    reloadAct->setDisabled(value);
    saveAsAct->setDisabled(value);
    exportBaseMapAct->setDisabled(value);
    exportMm2xmlMapAct->setDisabled(value);
    exportWebMapAct->setDisabled(value);
    exportMmpMapAct->setDisabled(value);
    exitAct->setDisabled(value);
    aboutAct->setDisabled(value);
    aboutQtAct->setDisabled(value);
    zoomInAct->setDisabled(value);
    zoomOutAct->setDisabled(value);
    zoomResetAct->setDisabled(value);
    mapperMode.playModeAct->setDisabled(value);
    mapperMode.mapModeAct->setDisabled(value);
    mouseMode.modeRoomSelectAct->setDisabled(value);
    mouseMode.modeConnectionSelectAct->setDisabled(value);
    mouseMode.modeMoveSelectAct->setDisabled(value);
    mouseMode.modeInfoMarkSelectAct->setDisabled(value);
    mouseMode.modeCreateInfoMarkAct->setDisabled(value);
    layerUpAct->setDisabled(value);
    layerDownAct->setDisabled(value);
    layerResetAct->setDisabled(value);
    mouseMode.modeCreateRoomAct->setDisabled(value);
    mouseMode.modeCreateConnectionAct->setDisabled(value);
    mouseMode.modeCreateOnewayConnectionAct->setDisabled(value);
    releaseAllPathsAct->setDisabled(value);
    alwaysOnTopAct->setDisabled(value);
}

void MainWindow::hideCanvas(const bool hide)
{
    // REVISIT: It seems that updates don't work if the canvas is hidden,
    // so we may want to save mapChanged() and other similar requests
    // and send them after we show the canvas.
    if (MapCanvas *const canvas = getCanvas()) {
        if (hide)
            canvas->hide();
        else
            canvas->show();
    }
}

void MainWindow::setupMenuBar()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(reloadAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    QMenu *exportMenu = fileMenu->addMenu(QIcon::fromTheme("document-send"), tr("&Export"));
    exportMenu->addAction(exportBaseMapAct);
    exportMenu->addAction(exportMm2xmlMapAct);
    exportMenu->addAction(exportWebMapAct);
    exportMenu->addAction(exportMmpMapAct);
    fileMenu->addAction(mergeAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    modeMenu = editMenu->addMenu(QIcon(":/icons/online.png"), tr("&Mode"));
    modeMenu->addAction(mapperMode.playModeAct);
    modeMenu->addAction(mapperMode.mapModeAct);
    modeMenu->addAction(mapperMode.offlineModeAct);
    editMenu->addSeparator();

    QMenu *infoMarkMenu = editMenu->addMenu(QIcon(":/icons/infomarkselection.png"), tr("M&arkers"));
    infoMarkMenu->setStatusTip("Info markers");
    infoMarkMenu->addAction(mouseMode.modeInfoMarkSelectAct);
    infoMarkMenu->addSeparator();
    infoMarkMenu->addAction(mouseMode.modeCreateInfoMarkAct);
    infoMarkMenu->addAction(infoMarkActions.editInfoMarkAct);
    infoMarkMenu->addAction(infoMarkActions.deleteInfoMarkAct);

    roomMenu = editMenu->addMenu(QIcon(":/icons/roomselection.png"), tr("&Rooms"));
    roomMenu->addAction(mouseMode.modeRoomSelectAct);
    roomMenu->addAction(mouseMode.modeRoomRaypickAct);
    roomMenu->addSeparator();
    roomMenu->addAction(mouseMode.modeCreateRoomAct);
    roomMenu->addAction(editRoomSelectionAct);
    roomMenu->addAction(deleteRoomSelectionAct);
    roomMenu->addAction(moveUpRoomSelectionAct);
    roomMenu->addAction(moveDownRoomSelectionAct);
    roomMenu->addAction(mergeUpRoomSelectionAct);
    roomMenu->addAction(mergeDownRoomSelectionAct);
    roomMenu->addAction(connectToNeighboursRoomSelectionAct);

    connectionMenu = editMenu->addMenu(QIcon(":/icons/connectionselection.png"), tr("&Connections"));
    connectionMenu->addAction(mouseMode.modeConnectionSelectAct);
    connectionMenu->addSeparator();
    connectionMenu->addAction(mouseMode.modeCreateConnectionAct);
    connectionMenu->addAction(mouseMode.modeCreateOnewayConnectionAct);
    connectionMenu->addAction(deleteConnectionSelectionAct);

    editMenu->addSeparator();
    editMenu->addAction(findRoomsAct);
    editMenu->addAction(preferencesAct);

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(mouseMode.modeMoveSelectAct);
    QMenu *toolbars = viewMenu->addMenu(tr("&Toolbars"));
    toolbars->addAction(fileToolBar->toggleViewAction());
    toolbars->addAction(mapperModeToolBar->toggleViewAction());
    toolbars->addAction(mouseModeToolBar->toggleViewAction());
    toolbars->addAction(viewToolBar->toggleViewAction());
    toolbars->addAction(pathMachineToolBar->toggleViewAction());
    toolbars->addAction(roomToolBar->toggleViewAction());
    toolbars->addAction(connectionToolBar->toggleViewAction());
    toolbars->addAction(groupToolBar->toggleViewAction());
    toolbars->addAction(settingsToolBar->toggleViewAction());
    QMenu *sidepanels = viewMenu->addMenu(tr("&Side Panels"));
    sidepanels->addAction(m_dockDialogLog->toggleViewAction());
    sidepanels->addAction(m_dockDialogClient->toggleViewAction());
    sidepanels->addAction(m_dockDialogGroup->toggleViewAction());
    sidepanels->addAction(m_dockDialogRoom->toggleViewAction());
    viewMenu->addSeparator();
    viewMenu->addAction(zoomInAct);
    viewMenu->addAction(zoomOutAct);
    viewMenu->addAction(zoomResetAct);
    viewMenu->addSeparator();
    viewMenu->addAction(layerUpAct);
    viewMenu->addAction(layerDownAct);
    viewMenu->addAction(layerResetAct);
    viewMenu->addSeparator();
    viewMenu->addAction(rebuildMeshesAct);
    viewMenu->addSeparator();
    viewMenu->addAction(showStatusBarAct);
    viewMenu->addAction(showScrollBarsAct);
    viewMenu->addAction(alwaysOnTopAct);

    settingsMenu = menuBar()->addMenu(tr("&Tools"));
    QMenu *clientMenu = settingsMenu->addMenu(QIcon(":/icons/terminal.png"),
                                              tr("&Integrated Mud Client"));
    clientMenu->addAction(clientAct);
    clientMenu->addAction(saveLogAct);
    groupMenu = settingsMenu->addMenu(QIcon(":/icons/groupclient.png"), tr("&Group Manager"));
    groupModeMenu = groupMenu->addMenu(tr("&Mode"));
    groupModeMenu->addAction(groupMode.groupOffAct);
    groupModeMenu->addAction(groupMode.groupClientAct);
    groupModeMenu->addAction(groupMode.groupServerAct);
    groupMenu->addAction(groupNetwork.networkStartAct);
    groupMenu->addAction(groupNetwork.networkStopAct);
    QMenu *pathMachineMenu = settingsMenu->addMenu(QIcon(":/icons/force.png"), tr("&Path Machine"));
    pathMachineMenu->addAction(mouseMode.modeRoomSelectAct);
    pathMachineMenu->addSeparator();
    pathMachineMenu->addAction(forceRoomAct);
    pathMachineMenu->addAction(releaseAllPathsAct);

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(settingUpMmapperAct);
    if constexpr (!NO_UPDATER)
        helpMenu->addAction(mmapperCheckForUpdateAct);
    helpMenu->addSeparator();
    mumeMenu = helpMenu->addMenu(QIcon::fromTheme("help-contents"), tr("M&UME"));
    mumeMenu->addAction(voteAct);
    mumeMenu->addAction(newbieAct);
    mumeMenu->addAction(mumeWebsiteAct);
    mumeMenu->addAction(mumeForumAct);
    mumeMenu->addAction(mumeWikiAct);
    helpMenu->addSeparator();
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::slot_showContextMenu(const QPoint &pos)
{
    QMenu contextMenu(tr("Context menu"), this);
    if (m_connectionSelection != nullptr) {
        // Connections cannot be selected alongside rooms and infomarks
        contextMenu.addAction(deleteConnectionSelectionAct);

    } else {
        // However, both rooms and infomarks can be selected at once
        if (m_roomSelection != nullptr) {
            if (m_roomSelection->empty()) {
                contextMenu.addAction(createRoomAct);
            } else {
                contextMenu.addAction(editRoomSelectionAct);
                contextMenu.addAction(moveUpRoomSelectionAct);
                contextMenu.addAction(moveDownRoomSelectionAct);
                contextMenu.addAction(mergeUpRoomSelectionAct);
                contextMenu.addAction(mergeDownRoomSelectionAct);
                contextMenu.addAction(deleteRoomSelectionAct);
                contextMenu.addAction(connectToNeighboursRoomSelectionAct);
                contextMenu.addSeparator();
                contextMenu.addAction(forceRoomAct);
            }
        }
        if (m_infoMarkSelection != nullptr && !m_infoMarkSelection->empty()) {
            if (m_roomSelection != nullptr)
                contextMenu.addSeparator();
            contextMenu.addAction(infoMarkActions.editInfoMarkAct);
            contextMenu.addAction(infoMarkActions.deleteInfoMarkAct);
        }
    }
    contextMenu.addSeparator();
    QMenu *mouseMenu = contextMenu.addMenu(QIcon::fromTheme("input-mouse"), "Mouse Mode");
    mouseMenu->addAction(mouseMode.modeMoveSelectAct);
    mouseMenu->addAction(mouseMode.modeRoomRaypickAct);
    mouseMenu->addAction(mouseMode.modeRoomSelectAct);
    mouseMenu->addAction(mouseMode.modeInfoMarkSelectAct);
    mouseMenu->addAction(mouseMode.modeConnectionSelectAct);
    mouseMenu->addAction(mouseMode.modeCreateInfoMarkAct);
    mouseMenu->addAction(mouseMode.modeCreateRoomAct);
    mouseMenu->addAction(mouseMode.modeCreateConnectionAct);
    mouseMenu->addAction(mouseMode.modeCreateOnewayConnectionAct);

    contextMenu.exec(getCanvas()->mapToGlobal(pos));
}

void MainWindow::slot_alwaysOnTop()
{
    const bool alwaysOnTop = this->alwaysOnTopAct->isChecked();
    qInfo() << "Setting AlwaysOnTop flag to " << alwaysOnTop;
    setWindowFlag(Qt::WindowStaysOnTopHint, alwaysOnTop);
    setConfig().general.alwaysOnTop = alwaysOnTop;
    show();
}

void MainWindow::slot_setShowStatusBar()
{
    const bool showStatusBar = this->showStatusBarAct->isChecked();
    qInfo() << "Setting showStatusBar to " << showStatusBar;
    statusBar()->setVisible(showStatusBar);
    // TODO CONFIGsetConfig().general.alwaysOnTop = alwaysOnTop;
    show();
}

void MainWindow::slot_setShowScrollBars()
{
    const bool showScrollBars = this->showScrollBarsAct->isChecked();
    qInfo() << "Setting showScrollBars to " << showScrollBars;
    m_mapWindow->setScrollBarsVisible(showScrollBars);
    // TODO Config
    show();
}

void MainWindow::setupToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->setObjectName("FileToolBar");
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);
    fileToolBar->hide();

    mapperModeToolBar = addToolBar(tr("Mapper Mode"));
    mapperModeToolBar->setObjectName("MapperModeToolBar");
    mapperModeToolBar->addAction(mapperMode.playModeAct);
    mapperModeToolBar->addAction(mapperMode.mapModeAct);
    mapperModeToolBar->addAction(mapperMode.offlineModeAct);
    mapperModeToolBar->hide();

    mouseModeToolBar = addToolBar(tr("Mouse Mode"));
    mouseModeToolBar->setObjectName("ModeToolBar");
    mouseModeToolBar->addAction(mouseMode.modeMoveSelectAct);
    mouseModeToolBar->addAction(mouseMode.modeRoomRaypickAct);
    mouseModeToolBar->addAction(mouseMode.modeRoomSelectAct);
    mouseModeToolBar->addAction(mouseMode.modeConnectionSelectAct);
    mouseModeToolBar->addAction(mouseMode.modeCreateRoomAct);
    mouseModeToolBar->addAction(mouseMode.modeCreateConnectionAct);
    mouseModeToolBar->addAction(mouseMode.modeCreateOnewayConnectionAct);
    mouseModeToolBar->addAction(mouseMode.modeInfoMarkSelectAct);
    mouseModeToolBar->addAction(mouseMode.modeCreateInfoMarkAct);
    mouseModeToolBar->hide();

    groupToolBar = addToolBar(tr("Group Manager"));
    groupToolBar->setObjectName("GroupManagerToolBar");
    groupToolBar->addAction(groupMode.groupOffAct);
    groupToolBar->addAction(groupMode.groupClientAct);
    groupToolBar->addAction(groupMode.groupServerAct);
    groupToolBar->addAction(groupNetwork.networkStartAct);
    groupToolBar->addAction(groupNetwork.networkStopAct);
    groupToolBar->hide();

    viewToolBar = addToolBar(tr("View"));
    viewToolBar->setObjectName("ViewToolBar");
    viewToolBar->addAction(zoomInAct);
    viewToolBar->addAction(zoomOutAct);
    viewToolBar->addAction(zoomResetAct);
    viewToolBar->addWidget(new MapZoomSlider(deref(m_mapWindow), this));
    viewToolBar->addAction(layerUpAct);
    viewToolBar->addAction(layerDownAct);
    viewToolBar->addAction(layerResetAct);
    viewToolBar->hide();

    pathMachineToolBar = addToolBar(tr("Path Machine"));
    pathMachineToolBar->setObjectName("PathMachineToolBar");
    pathMachineToolBar->addAction(releaseAllPathsAct);
    pathMachineToolBar->addAction(forceRoomAct);
    pathMachineToolBar->hide();

    roomToolBar = addToolBar(tr("Rooms"));
    roomToolBar->setObjectName("RoomsToolBar");
    roomToolBar->addAction(findRoomsAct);
    roomToolBar->addAction(editRoomSelectionAct);
    roomToolBar->addAction(deleteRoomSelectionAct);
    roomToolBar->addAction(moveUpRoomSelectionAct);
    roomToolBar->addAction(moveDownRoomSelectionAct);
    roomToolBar->addAction(mergeUpRoomSelectionAct);
    roomToolBar->addAction(mergeDownRoomSelectionAct);
    roomToolBar->addAction(connectToNeighboursRoomSelectionAct);
    roomToolBar->hide();

    connectionToolBar = addToolBar(tr("Connections"));
    connectionToolBar->setObjectName("ConnectionsToolBar");
    connectionToolBar->addAction(deleteConnectionSelectionAct);
    connectionToolBar->hide();

    settingsToolBar = addToolBar(tr("Preferences"));
    settingsToolBar->setObjectName("PreferencesToolBar");
    settingsToolBar->addAction(preferencesAct);
    settingsToolBar->hide();
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage(tr("Say friend and enter..."));
    statusBar()->insertPermanentWidget(0, new MumeClockWidget(m_mumeClock, this));
}

void MainWindow::slot_onPreferences()
{
    if (m_configDialog == nullptr) {
        m_configDialog = std::make_unique<ConfigDialog>(m_groupManager, this);
    }

    connect(m_configDialog.get(),
            &ConfigDialog::sig_graphicsSettingsChanged,
            m_mapWindow,
            &MapWindow::slot_graphicsSettingsChanged);
    m_configDialog->show();
}

void MainWindow::slot_newRoomSelection(const SigRoomSelection &rs)
{
    forceRoomAct->setEnabled(false);
    if (rs.isValid())
        m_roomSelection = rs.getShared();
    else
        m_roomSelection.reset();

    if (m_roomSelection != nullptr) {
        statusBar()->showMessage(QString("Selection: %1 room%2")
                                     .arg(m_roomSelection->size())
                                     .arg((m_roomSelection->size() != 1) ? "s" : ""),
                                 5000);
        selectedRoomActGroup->setEnabled(true);
        if (m_roomSelection->size() == 1) {
            forceRoomAct->setEnabled(true);
        }
    } else {
        selectedRoomActGroup->setEnabled(false);
    }
}

void MainWindow::slot_newConnectionSelection(ConnectionSelection *const cs)
{
    m_connectionSelection = (cs != nullptr) ? cs->shared_from_this() : nullptr;
    selectedConnectionActGroup->setEnabled(m_connectionSelection != nullptr);
}

void MainWindow::slot_newInfoMarkSelection(InfoMarkSelection *const is)
{
    m_infoMarkSelection = (is != nullptr) ? is->shared_from_this() : nullptr;
    infoMarkActions.infoMarkGroup->setEnabled(m_infoMarkSelection != nullptr);

    if (m_infoMarkSelection != nullptr) {
        statusBar()->showMessage(QString("Selection: %1 mark%2")
                                     .arg(m_infoMarkSelection->size())
                                     .arg((m_infoMarkSelection->size() != 1) ? "s" : ""),
                                 5000);
        if (m_infoMarkSelection->empty()) {
            // Create a new infomark if its an empty selection
            slot_onEditInfoMarkSelection();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    if (maybeSave()) {
        // REVISIT: Group Manager is not owned by the MainWindow and needs to be terminated
        m_groupManager->stop();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::showEvent(QShowEvent *const event)
{
    // Check screen DPI each time MMapper's window is shown
    getCanvas()->screenChanged();

    static std::once_flag flag;
    std::call_once(flag, [this]() {
        // Read geometry and state settings and start services on startup
        readSettings();
        startServices();

        connect(window()->windowHandle(), &QWindow::screenChanged, this, [this]() {
            MapCanvas &canvas = deref(getCanvas());
            CanvasDisabler canvasDisabler{canvas};
            canvas.screenChanged();
        });
    });
    event->accept();
}

void MainWindow::slot_newFile()
{
    if (maybeSave()) {
        forceNewFile();
    }
}

void MainWindow::forceNewFile()
{
    MapStorage mapStorage(*m_mapData, "", this);
    auto *storage = static_cast<AbstractMapStorage *>(&mapStorage);
    connect(storage, &AbstractMapStorage::sig_onNewData, getCanvas(), &MapCanvas::slot_dataLoaded);
    connect(storage,
            &AbstractMapStorage::sig_onNewData,
            m_groupWidget,
            &GroupWidget::slot_mapLoaded);
    connect(storage, &AbstractMapStorage::sig_onNewData, this, [this]() {
        setWindowModified(false);
        saveAct->setEnabled(false);
    });
    connect(storage, &AbstractMapStorage::sig_log, this, &MainWindow::slot_log);
    storage->newData();
    setCurrentFile("");
    mapChanged();
}

void MainWindow::slot_merge()
{
    CanvasDisabler canvasDisabler{deref(getCanvas())};

    // FIXME: code duplication
    const auto &savedLastMapDir = setConfig().autoLoad.lastMapDirectory;
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Choose map file ...",
                                                    savedLastMapDir,
                                                    "MMapper Maps (*.mm2)");
    if (fileName.isEmpty())
        return;

    QFile file(fileName);

    if (!file.open(QFile::ReadOnly)) {
        showWarning(tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));

        return;
    }

    auto progressDlg = createNewProgressDialog("Importing map...");

    getCanvas()->slot_clearAllSelections();

    MapStorage mapStorage(*m_mapData, fileName, &file, this);
    auto storage = static_cast<AbstractMapStorage *>(&mapStorage);
    connect(storage,
            &AbstractMapStorage::sig_onDataLoaded,
            getCanvas(),
            &MapCanvas::slot_dataLoaded);
    connect(storage,
            &AbstractMapStorage::sig_onDataLoaded,
            m_groupWidget,
            &GroupWidget::slot_mapLoaded);
    connect(storage, &AbstractMapStorage::sig_onDataLoaded, this, [this]() {
        setWindowModified(false);
        saveAct->setEnabled(false);
    });
    connect(&storage->getProgressCounter(),
            &ProgressCounter::sig_onPercentageChanged,
            this,
            &MainWindow::slot_percentageChanged);
    connect(storage, &AbstractMapStorage::sig_log, this, &MainWindow::slot_log);

    const bool merged = [this, &storage]() -> bool {
        ActionDisabler actionDisabler{*this};
        CanvasHider canvasHider{*this};
        return storage->canLoad() && storage->mergeData();
    }();

    if (!merged) {
        showWarning(tr("Failed to merge file %1.").arg(fileName));
    } else {
        mapChanged();
        statusBar()->showMessage(tr("File merged"), 2000);
    }

    progressDlg.reset();
}

void MainWindow::slot_open()
{
    if (!maybeSave())
        return;

    // FIXME: code duplication
    auto &savedLastMapDir = setConfig().autoLoad.lastMapDirectory;
    const QString fileName = QFileDialog::getOpenFileName(
        this,
        "Choose map file ...",
        savedLastMapDir,
        "MMapper2 Maps (*.mm2);;MMapper2 XML Maps (*.mm2xml);;Pandora Maps (*.xml)");
    if (fileName.isEmpty()) {
        statusBar()->showMessage(tr("No filename provided"), 2000);
        return;
    }
    QFileInfo file(fileName);
    savedLastMapDir = file.dir().absolutePath();
    loadFile(file.absoluteFilePath());
}

void MainWindow::slot_reload()
{
    if (maybeSave()) {
        // make a copy of the filename, since it will be modified by loadFile().
        const QString filename = m_mapData->getFileName();
        loadFile(filename);
    }
}

bool MainWindow::slot_save()
{
    if (m_mapData->getFileName().isEmpty() || m_mapData->isFileReadOnly()) {
        return slot_saveAs();
    }
    return saveFile(m_mapData->getFileName(), SaveModeEnum::FULL, SaveFormatEnum::MM2);
}

std::unique_ptr<QFileDialog> MainWindow::createDefaultSaveDialog()
{
    const auto &path = getConfig().autoLoad.lastMapDirectory;
    QDir dir;
    if (dir.mkpath(path))
        dir.setPath(path);
    else
        dir.setPath(QDir::homePath());
    auto save = std::make_unique<QFileDialog>(this, "Choose map file name ...");
    save->setFileMode(QFileDialog::AnyFile);
    save->setDirectory(dir);
    save->setNameFilter("MMapper Maps (*.mm2)");
    save->setDefaultSuffix("mm2");
    save->setAcceptMode(QFileDialog::AcceptSave);
    return save;
}

NODISCARD static QStringList getSaveFileNames(std::unique_ptr<QFileDialog> &&ptr)
{
    if (const auto pSaveDialog = ptr.get()) {
        if (pSaveDialog->exec() == QDialog::Accepted)
            return pSaveDialog->selectedFiles();
        return QStringList{};
    }
    throw NullPointerException();
}

bool MainWindow::slot_saveAs()
{
    const auto makeSaveDialog = [this]() {
        auto saveDialog = createDefaultSaveDialog();
        QFileInfo currentFile(m_mapData->getFileName());
        if (currentFile.exists()) {
            QString suggestedFileName = currentFile.fileName()
                                            .replace(QRegularExpression(R"(\.mm2$)"), "-copy.mm2")
                                            .replace(QRegularExpression(R"(\.xml$)"), "-import.mm2");
            saveDialog->selectFile(suggestedFileName);
        }
        return saveDialog;
    };
    const QStringList fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        statusBar()->showMessage(tr("No filename provided"), 2000);
        return false;
    }

    QFileInfo file(fileNames[0]);
    bool success = saveFile(file.absoluteFilePath(), SaveModeEnum::FULL, SaveFormatEnum::MM2);
    if (success) {
        // Update last directory
        auto &config = setConfig().autoLoad;
        config.lastMapDirectory = file.absoluteDir().absolutePath();

        // Check if this should be the new autoload map
        QMessageBox dlg(QMessageBox::Question,
                        "Autoload Map?",
                        "Autoload this map when MMapper starts?",
                        QMessageBox::StandardButtons{QMessageBox::Yes | QMessageBox::No},
                        this);
        if (dlg.exec() == QMessageBox::Yes) {
            config.autoLoadMap = true;
            config.fileName = file.absoluteFilePath();
        }
    }
    return success;
}

bool MainWindow::slot_exportBaseMap()
{
    const auto makeSaveDialog = [this]() {
        auto saveDialog = createDefaultSaveDialog();
        QFileInfo currentFile(m_mapData->getFileName());
        if (currentFile.exists()) {
            saveDialog->selectFile(
                currentFile.fileName().replace(QRegularExpression(R"(\.mm2$)"), "-base.mm2"));
        }
        return saveDialog;
    };

    const auto fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        statusBar()->showMessage(tr("No filename provided"), 2000);
        return false;
    }

    return saveFile(fileNames[0], SaveModeEnum::BASEMAP, SaveFormatEnum::MM2);
}

bool MainWindow::slot_exportMm2xmlMap()
{
    const auto makeSaveDialog = [this]() {
        // FIXME: code duplication
        auto save = std::make_unique<QFileDialog>(this,
                                                  "Choose map file name ...",
                                                  QDir::current().absolutePath());
        save->setFileMode(QFileDialog::AnyFile);
        save->setDirectory(QDir(getConfig().autoLoad.lastMapDirectory));
        save->setNameFilter("MM2XML Maps (*.mm2xml)");
        save->setDefaultSuffix("mm2xml");
        save->setAcceptMode(QFileDialog::AcceptSave);
        save->selectFile(QFileInfo(m_mapData->getFileName())
                             .fileName()
                             .replace(QRegularExpression(R"(\.mm2$)"), ".mm2xml"));

        return save;
    };

    const auto fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        statusBar()->showMessage(tr("No filename provided"), 2000);
        return false;
    }
    return saveFile(fileNames[0], SaveModeEnum::FULL, SaveFormatEnum::MM2XML);
}

bool MainWindow::slot_exportWebMap()
{
    const auto makeSaveDialog = [this]() {
        // FIXME: code duplication
        auto save = std::make_unique<QFileDialog>(this,
                                                  "Choose map file name ...",
                                                  QDir::current().absolutePath());
        save->setFileMode(QFileDialog::Directory);
        save->setOption(QFileDialog::ShowDirsOnly, true);
        save->setAcceptMode(QFileDialog::AcceptSave);
        save->setDirectory(QDir(getConfig().autoLoad.lastMapDirectory));
        return save;
    };

    const QStringList fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        statusBar()->showMessage(tr("No filename provided"), 2000);
        return false;
    }

    return saveFile(fileNames[0], SaveModeEnum::BASEMAP, SaveFormatEnum::WEB);
}

bool MainWindow::slot_exportMmpMap()
{
    const auto makeSaveDialog = [this]() {
        // FIXME: code duplication
        auto save = std::make_unique<QFileDialog>(this,
                                                  "Choose map file name ...",
                                                  QDir::current().absolutePath());
        save->setFileMode(QFileDialog::AnyFile);
        save->setDirectory(QDir(getConfig().autoLoad.lastMapDirectory));
        save->setNameFilter("MMP Maps (*.xml)");
        save->setDefaultSuffix("xml");
        save->setAcceptMode(QFileDialog::AcceptSave);
        save->selectFile(QFileInfo(m_mapData->getFileName())
                             .fileName()
                             .replace(QRegularExpression(R"(\.mm2$)"), "-mmp.xml"));

        return save;
    };

    const auto fileNames = getSaveFileNames(makeSaveDialog());
    if (fileNames.isEmpty()) {
        statusBar()->showMessage(tr("No filename provided"), 2000);
        return false;
    }
    return saveFile(fileNames[0], SaveModeEnum::FULL, SaveFormatEnum::MMP);
}

void MainWindow::slot_about()
{
    AboutDialog about(this);
    about.exec();
}

bool MainWindow::maybeSave()
{
    if (!m_mapData->dataChanged())
        return true;

    const int ret = QMessageBox::warning(this,
                                         tr("mmapper"),
                                         tr("The document has been modified.\n"
                                            "Do you want to save your changes?"),
                                         QMessageBox::Yes | QMessageBox::Default,
                                         QMessageBox::No,
                                         QMessageBox::Cancel | QMessageBox::Escape);

    if (ret == QMessageBox::Yes) {
        return slot_save();
    }

    // REVISIT: is it a bug if this returns true? (Shouldn't this always be false?)
    return ret != QMessageBox::Cancel;
}

MainWindow::ProgressDialogLifetime MainWindow::createNewProgressDialog(const QString &text)
{
    m_progressDlg = std::make_unique<QProgressDialog>(this);
    {
        QPushButton *cb = new QPushButton("Abort ...", m_progressDlg.get());
        cb->setEnabled(false);
        m_progressDlg->setCancelButton(cb);
    }
    m_progressDlg->setLabelText(text);
    m_progressDlg->setCancelButtonText("Abort");
    m_progressDlg->setMinimum(0);
    m_progressDlg->setMaximum(100);
    m_progressDlg->setValue(0);
    m_progressDlg->show();
    return ProgressDialogLifetime{*this};
}

void MainWindow::endProgressDialog()
{
    m_progressDlg.reset();
}

void MainWindow::loadFile(const QString &fileName)
{
    if (fileName.isEmpty()) {
        statusBar()->showMessage(tr("No filename provided"), 2000);
        return;
    }

    CanvasDisabler canvasDisabler{deref(getCanvas())};

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        showWarning(tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
        return;
    }

    // Immediately discard the old map.
    forceNewFile();

    auto progressDlg = createNewProgressDialog("Loading map...");

    const auto storage = [this, &fileName, &file]() -> std::unique_ptr<AbstractMapStorage> {
        const QString fileNameLower = fileName.toLower();
        if (fileNameLower.endsWith(".xml")) {
            // Pandora map
            return std::make_unique<PandoraMapStorage>(*m_mapData, fileName, &file, this);
        } else if (fileNameLower.endsWith(".mm2xml")) {
            // MMapper2 XML map
            return std::make_unique<XmlMapStorage>(*m_mapData, fileName, &file, this);
        } else {
            // MMapper2 binary map
            return std::make_unique<MapStorage>(*m_mapData, fileName, &file, this);
        }
    }();

    // REVISIT: refactor the connections to a common function?
    connect(storage.get(),
            &AbstractMapStorage::sig_onDataLoaded,
            getCanvas(),
            &MapCanvas::slot_dataLoaded);
    connect(storage.get(),
            &AbstractMapStorage::sig_onDataLoaded,
            m_groupWidget,
            &GroupWidget::slot_mapLoaded);
    connect(storage.get(), &AbstractMapStorage::sig_onDataLoaded, this, [this]() {
        setWindowModified(false);
        saveAct->setEnabled(false);
    });
    connect(&storage->getProgressCounter(),
            &ProgressCounter::sig_onPercentageChanged,
            this,
            &MainWindow::slot_percentageChanged);
    connect(storage.get(), &AbstractMapStorage::sig_log, this, &MainWindow::slot_log);

    const bool loaded = [this, &storage]() -> bool {
        ActionDisabler actionDisabler{*this};
        CanvasHider canvasHider{*this};
        return storage->canLoad() && storage->loadData();
    }();

    if (!loaded) {
        showWarning(tr("Failed to load file %1.").arg(fileName));
        return;
    }

    mapChanged();
    setCurrentFile(m_mapData->getFileName());
    statusBar()->showMessage(tr("File loaded"), 2000);
}

void MainWindow::slot_percentageChanged(const quint32 p)
{
    if (m_progressDlg == nullptr)
        return;

    m_progressDlg->setValue(static_cast<int>(p));
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}

void MainWindow::showWarning(const QString &s)
{
    // REVISIT: shouldn't the warning have "this" as parent?
    QMessageBox::warning(nullptr, tr("Application"), s);
}

bool MainWindow::saveFile(const QString &fileName,
                          const SaveModeEnum mode,
                          const SaveFormatEnum format)
{
    CanvasDisabler canvasDisabler{deref(getCanvas())};

    FileSaver saver;
    // REVISIT: You can still test a directory for writing...
    if (format != SaveFormatEnum::WEB) { // Web uses a whole directory
        try {
            saver.open(fileName);
        } catch (const std::exception &e) {
            showWarning(tr("Cannot write file %1:\n%2.").arg(fileName).arg(e.what()));
            return false;
        }
    }

    const auto storage = [this, format, &fileName, &saver]() -> std::unique_ptr<AbstractMapStorage> {
        switch (format) {
        case SaveFormatEnum::MM2:
            return std::make_unique<MapStorage>(*m_mapData, fileName, &saver.file(), this);
        case SaveFormatEnum::MM2XML:
            return std::make_unique<XmlMapStorage>(*m_mapData, fileName, &saver.file(), this);
        case SaveFormatEnum::MMP:
            return std::make_unique<MmpMapStorage>(*m_mapData, fileName, &saver.file(), this);
        case SaveFormatEnum::WEB:
            return std::make_unique<JsonMapStorage>(*m_mapData, fileName, this);
        }
        assert(false);
        return {};
    }();

    if (!storage || !storage->canSave()) {
        showWarning(tr("Selected format cannot save."));
        return false;
    }

    auto progressDlg = createNewProgressDialog("Saving map...");

    // REVISIT: This is done enough times that it should probably be a function by itself.
    connect(&storage->getProgressCounter(),
            &ProgressCounter::sig_onPercentageChanged,
            this,
            &MainWindow::slot_percentageChanged);
    connect(storage.get(), &AbstractMapStorage::sig_log, this, &MainWindow::slot_log);

    const bool saveOk = [this, mode, &storage]() -> bool {
        ActionDisabler actionDisabler{*this};
        // REVISIT: Does this need hide/show?
        return storage->saveData(mode == SaveModeEnum::BASEMAP);
    }();
    progressDlg.reset();

    try {
        saver.close();
    } catch (const std::exception &e) {
        showWarning(tr("Cannot write file %1:\n%2.").arg(fileName).arg(e.what()));
        return false;
    }

    if (!saveOk) {
        showWarning(tr("Error while saving (see log)."));
        // REVISIT: Shouldn't this return false?
    } else {
        if (mode == SaveModeEnum::FULL && format == SaveFormatEnum::MM2) {
            m_mapData->setFileName(fileName, !QFileInfo(fileName).isWritable());
            setCurrentFile(fileName);
        }
        statusBar()->showMessage(tr("File saved"), 2000);
    }

    setWindowModified(false);
    saveAct->setEnabled(false);
    return true;
}

void MainWindow::slot_onFindRoom()
{
    m_findRoomsDlg->show();
}

void MainWindow::slot_onLaunchClient()
{
    m_dockDialogClient->show();
    m_clientWidget->setFocus();
}

void MainWindow::slot_groupNetworkStatus(const bool status)
{
    if (status) {
        m_dockDialogGroup->show();
        groupNetwork.networkStopAct->setShortcut(tr("Ctrl+G"));
        groupNetwork.networkStartAct->setChecked(true);
        groupNetwork.networkStartAct->setShortcut(tr(""));
    } else {
        groupNetwork.networkStartAct->setShortcut(tr("Ctrl+G"));
        groupNetwork.networkStopAct->setChecked(true);
        groupNetwork.networkStopAct->setShortcut(tr(""));
    }
}

void MainWindow::slot_onModeGroupOff()
{
    groupModeMenu->setIcon(QIcon(":/icons/groupoff.png"));
    groupNetwork.groupNetworkGroup->setEnabled(false);
    groupNetwork.networkStartAct->setText("Start");
    groupNetwork.networkStopAct->setText("Stop");
    emit sig_setGroupMode(GroupManagerStateEnum::Off);
}

void MainWindow::slot_onModeGroupClient()
{
    groupModeMenu->setIcon(QIcon(":/icons/groupclient.png"));
    groupNetwork.groupNetworkGroup->setEnabled(true);
    groupNetwork.networkStartAct->setText("&Connect to a friend's map");
    groupNetwork.networkStopAct->setText("&Disconnect");
    emit sig_setGroupMode(GroupManagerStateEnum::Client);
}

void MainWindow::slot_onModeGroupServer()
{
    groupModeMenu->setIcon(QIcon(":/icons/groupserver.png"));
    groupNetwork.groupNetworkGroup->setEnabled(true);
    groupNetwork.networkStartAct->setText("&Host your map with friends");
    groupNetwork.networkStopAct->setText("&Disconnect");
    emit sig_setGroupMode(GroupManagerStateEnum::Server);
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    QFileInfo file(fileName);
    const auto shownName = fileName.isEmpty() ? "Untitled" : file.fileName();
    const auto fileSuffix = ((m_mapData && m_mapData->isFileReadOnly()) || !file.isWritable())
                                ? " [read-only]"
                                : "";
    const auto appSuffix = isMMapperBeta() ? " Beta" : "";

    // [*] is Qt black magic for only showing the '*' if the document has been modified.
    // From https://doc.qt.io/qt-5/qwidget.html:
    // > The window title must contain a "[*]" placeholder, which indicates where the '*' should appear.
    // > Normally, it should appear right after the file name (e.g., "document1.txt[*] - Text Editor").
    // > If the window isn't modified, the placeholder is simply removed.
    setWindowTitle(QString("%1[*]%2 - MMapper%3").arg(shownName).arg(fileSuffix).arg(appSuffix));
}

void MainWindow::slot_onLayerUp()
{
    getCanvas()->slot_layerUp();
}

void MainWindow::slot_onLayerDown()
{
    getCanvas()->slot_layerDown();
}

void MainWindow::slot_onLayerReset()
{
    getCanvas()->slot_layerReset();
}

void MainWindow::slot_onModeConnectionSelect()
{
    setCanvasMouseMode(CanvasMouseModeEnum::SELECT_CONNECTIONS);
}

void MainWindow::slot_onModeRoomRaypick()
{
    setCanvasMouseMode(CanvasMouseModeEnum::RAYPICK_ROOMS);
}

void MainWindow::slot_onModeRoomSelect()
{
    setCanvasMouseMode(CanvasMouseModeEnum::SELECT_ROOMS);
}

void MainWindow::slot_onModeMoveSelect()
{
    setCanvasMouseMode(CanvasMouseModeEnum::MOVE);
}

void MainWindow::slot_onModeCreateRoomSelect()
{
    setCanvasMouseMode(CanvasMouseModeEnum::CREATE_ROOMS);
}

void MainWindow::slot_onModeCreateConnectionSelect()
{
    setCanvasMouseMode(CanvasMouseModeEnum::CREATE_CONNECTIONS);
}

void MainWindow::slot_onModeCreateOnewayConnectionSelect()
{
    setCanvasMouseMode(CanvasMouseModeEnum::CREATE_ONEWAY_CONNECTIONS);
}

void MainWindow::slot_onModeInfoMarkSelect()
{
    setCanvasMouseMode(CanvasMouseModeEnum::SELECT_INFOMARKS);
}

void MainWindow::slot_onModeCreateInfoMarkSelect()
{
    setCanvasMouseMode(CanvasMouseModeEnum::CREATE_INFOMARKS);
}

void MainWindow::slot_onEditInfoMarkSelection()
{
    if (m_infoMarkSelection == nullptr)
        return;

    InfoMarksEditDlg dlg(this);
    dlg.setInfoMarkSelection(m_infoMarkSelection, m_mapData, getCanvas());
    dlg.exec();
    dlg.show();
}

void MainWindow::slot_onCreateRoom()
{
    getCanvas()->slot_createRoom();
    mapChanged();
}

void MainWindow::slot_onEditRoomSelection()
{
    if (m_roomSelection == nullptr)
        return;

    RoomEditAttrDlg roomEditDialog(this);
    roomEditDialog.setRoomSelection(m_roomSelection, m_mapData, getCanvas());
    roomEditDialog.exec();
    roomEditDialog.show();
}

void MainWindow::slot_onDeleteInfoMarkSelection()
{
    if (m_infoMarkSelection == nullptr) {
        return;
    }

    {
        const auto tmp = std::exchange(m_infoMarkSelection, nullptr);
        m_mapData->removeMarkers(*tmp);
    }

    MapCanvas *const canvas = getCanvas();
    canvas->slot_clearInfoMarkSelection();
    canvas->infomarksChanged();
}

void MainWindow::slot_onDeleteRoomSelection()
{
    if (m_roomSelection == nullptr)
        return;

    execSelectionGroupMapAction(std::make_unique<Remove>());
    getCanvas()->slot_clearRoomSelection();
    mapChanged();
}

void MainWindow::slot_onDeleteConnectionSelection()
{
    if (m_connectionSelection == nullptr)
        return; // previously called mapChanged for no good reason

    const auto &first = m_connectionSelection->getFirst();
    const auto &second = m_connectionSelection->getSecond();
    const Room *const r1 = first.room;
    const Room *const r2 = second.room;
    if (r1 == nullptr || r2 == nullptr)
        return; // previously called mapChanged for no good reason

    const ExitDirEnum dir1 = first.direction;
    const ExitDirEnum dir2 = second.direction;
    const RoomId &id1 = r1->getId();
    const RoomId &id2 = r2->getId();

    const auto tmpSel = RoomSelection::createSelection(*m_mapData);
    tmpSel->getRoom(id1);
    tmpSel->getRoom(id2);
    getCanvas()->slot_clearConnectionSelection();
    m_mapData->execute(std::make_unique<RemoveTwoWayExit>(id1, id2, dir1, dir2), tmpSel);

    mapChanged();
}

void MainWindow::slot_onMoveUpRoomSelection()
{
    if (m_roomSelection == nullptr)
        return;

    execSelectionGroupMapAction(std::make_unique<MoveRelative>(Coordinate(0, 0, 1)));
    slot_onLayerUp();
    mapChanged();
}

void MainWindow::slot_onMoveDownRoomSelection()
{
    if (m_roomSelection == nullptr)
        return;

    execSelectionGroupMapAction(std::make_unique<MoveRelative>(Coordinate(0, 0, -1)));
    slot_onLayerDown();
    mapChanged();
}

void MainWindow::slot_onMergeUpRoomSelection()
{
    if (m_roomSelection == nullptr)
        return;

    execSelectionGroupMapAction(std::make_unique<MergeRelative>(Coordinate(0, 0, 1)));
    mapChanged();
    slot_onLayerUp();
    slot_onModeRoomSelect();
}

void MainWindow::slot_onMergeDownRoomSelection()
{
    if (m_roomSelection == nullptr)
        return;

    execSelectionGroupMapAction(std::make_unique<MergeRelative>(Coordinate(0, 0, -1)));
    mapChanged();
    slot_onLayerDown();
    slot_onModeRoomSelect();
}

void MainWindow::slot_onConnectToNeighboursRoomSelection()
{
    if (m_roomSelection == nullptr)
        return;

    execSelectionGroupMapAction(std::make_unique<ConnectToNeighbours>());
    mapChanged();
}

void MainWindow::slot_onCheckForUpdate()
{
    assert(!NO_UPDATER);
    m_updateDialog->show();
    m_updateDialog->open();
}

void MainWindow::slot_voteForMUME()
{
    QDesktopServices::openUrl(QUrl(
        "https://www.mudconnect.com/cgi-bin/search.cgi?mode=mud_listing&mud=MUME+-+Multi+Users+In+Middle+Earth"));
}

void MainWindow::slot_openMumeWebsite()
{
    QDesktopServices::openUrl(QUrl("https://mume.org/"));
}

void MainWindow::slot_openMumeForum()
{
    QDesktopServices::openUrl(QUrl("https://mume.org/forum/"));
}

void MainWindow::slot_openMumeWiki()
{
    QDesktopServices::openUrl(QUrl("https://mume.org/wiki/"));
}

void MainWindow::slot_openSettingUpMmapper()
{
    QDesktopServices::openUrl(QUrl("https://github.com/MUME/MMapper/wiki/Troubleshooting"));
}

void MainWindow::slot_openNewbieHelp()
{
    QDesktopServices::openUrl(QUrl("https://mume.org/newbie.php"));
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        m_mapWindow->keyPressEvent(event);
        return;
    }
    QWidget::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        m_mapWindow->keyReleaseEvent(event);
        return;
    }
    QWidget::keyReleaseEvent(event);
}

MapCanvas *MainWindow::getCanvas() const
{
    return m_mapWindow->getCanvas();
}

void MainWindow::mapChanged() const
{
    if (MapCanvas *const canvas = getCanvas())
        canvas->mapChanged();
}

void MainWindow::setCanvasMouseMode(const CanvasMouseModeEnum mode)
{
    if (MapCanvas *const canvas = getCanvas())
        canvas->slot_setCanvasMouseMode(mode);
}

void MainWindow::execSelectionGroupMapAction(std::unique_ptr<AbstractAction> input_action)
{
    deref(m_roomSelection);
    deref(m_mapData).execute(std::make_unique<GroupMapAction>(std::move(input_action),
                                                              m_roomSelection),
                             m_roomSelection);
}
