// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mainwindow.h"

#include "../adventure/adventuretracker.h"
#include "../adventure/adventurewidget.h"
#include "../adventure/xpstatuswidget.h"
#include "../client/ClientWidget.h"
#include "../clock/mumeclock.h"
#include "../clock/mumeclockwidget.h"
#include "../display/InfomarkSelection.h"
#include "../display/MapCanvasData.h"
#include "../display/mapcanvas.h"
#include "../display/mapwindow.h"
#include "../global/AsyncTasks.h"
#include "../global/SendToUser.h"
#include "../global/SignalBlocker.h"
#include "../global/Version.h"
#include "../global/window_utils.h"
#include "../group/groupwidget.h"
#include "../logger/autologger.h"
#include "../pathmachine/mmapper2pathmachine.h"
#include "../preferences/configdialog.h"
#include "../proxy/connectionlistener.h"
#include "../roompanel/RoomManager.h"
#include "../roompanel/RoomWidget.h"
#include "../viewers/TopLevelWindows.h"
#include "DescriptionWidget.h"
#include "MapZoomSlider.h"
#include "UpdateDialog.h"
#include "aboutdialog.h"
#include "findroomsdlg.h"
#include "infomarkseditdlg.h"
#include "metatypes.h"
#include "roomeditattrdlg.h"
#include "utils.h"

#include <memory>
#include <mutex>

#include <QActionGroup>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFontDatabase>
#include <QIcon>
#include <QProgressDialog>
#include <QSize>
#include <QString>
#include <QTextBrowser>
#include <QUrl>
#include <QtWidgets>

NODISCARD static const char *get_type_name(const AsyncTypeEnum mode)
{
    switch (mode) {
    case AsyncTypeEnum::Load:
        return "load";
    case AsyncTypeEnum::Merge:
        return "merge";
    case AsyncTypeEnum::Save:
        return "save";
    default:
        assert(false);
        return "(error)";
    }
}

NODISCARD static const char *basic_plural(size_t n)
{
    return (n == 1) ? "" : "s";
}

QString MainWindow::chooseLoadOrMergeFileName()
{
    const QString &savedLastMapDir = setConfig().autoLoad.lastMapDirectory;
    return QFileDialog::getOpenFileName(this,
                                        "Choose map file ...",
                                        savedLastMapDir,
                                        "MMapper2 maps (*.mm2)"
                                        ";;MMapper2 XML or Pandora maps (*.xml)"
                                        ";;Alternate suffix for MMapper2 XML maps (*.mm2xml)");
}

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
            if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac) {
                defaultClientFont.setPointSize(12);
            } else {
                defaultClientFont.setPointSize(10);
            }
            defaultClientFont.setStyleStrategy(QFont::PreferAntialias);
            setConfig().integratedClient.font = defaultClientFont.toString();
        }
    }
}

MainWindow::~MainWindow()
{
    forceNewFile();
    mmqt::rdisconnect(this);
    async_tasks::cleanup();
    delete m_listener;
    destroyTopLevelWindows();
}

MainWindow::MainWindow()
    : QMainWindow(nullptr, Qt::WindowFlags{})
    , m_asyncTask(this)
{
    initTopLevelWindows();
    async_tasks::init();
    setObjectName("MainWindow");
    setWindowIcon(QIcon(":/icons/m.png"));
    addApplicationFont();
    registerMetatypes();

    m_mapData = new MapData(this);
    MapData &mapData = deref(m_mapData);

    m_mapData->setObjectName("MapData");
    setCurrentFile("");

    m_prespammedPath = new PrespammedPath(this);

    m_groupManager = new Mmapper2Group(this);
    m_groupManager->setObjectName("GroupManager");

    m_mapWindow = new MapWindow(mapData, deref(m_prespammedPath), deref(m_groupManager), this);
    setCentralWidget(m_mapWindow);

    m_pathMachine = new Mmapper2PathMachine(mapData, this);
    m_pathMachine->setObjectName("Mmapper2PathMachine");

    m_gameObserver = std::make_unique<GameObserver>();
    m_adventureTracker = new AdventureTracker(deref(m_gameObserver), this);

    // View -> Side Panels -> Client Panel
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

    // View -> Side Panels -> Log Panel
    m_dockDialogLog = new QDockWidget(tr("Log Panel"), this);
    m_dockDialogLog->setObjectName("DockWidgetLog");
    m_dockDialogLog->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    m_dockDialogLog->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable
                                 | QDockWidget::DockWidgetClosable);
    m_dockDialogLog->toggleViewAction()->setShortcut(tr("Ctrl+L"));
    addDockWidget(Qt::BottomDockWidgetArea, m_dockDialogLog);

    logWindow = new QTextBrowser(m_dockDialogLog);
    logWindow->setReadOnly(true);
    logWindow->setObjectName("LogWindow");
    m_dockDialogLog->setWidget(logWindow);
    m_dockDialogLog->hide();

    // View -> Side Panels -> Group Panel and Tools -> Group Manager
    m_groupWidget = new GroupWidget(m_groupManager, m_mapData, this);
    m_dockDialogGroup = new QDockWidget(tr("Group Panel"), this);
    m_dockDialogGroup->setObjectName("DockWidgetGroup");
    m_dockDialogGroup->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    m_dockDialogGroup->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable
                                   | QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::TopDockWidgetArea, m_dockDialogGroup);
    m_dockDialogGroup->setWidget(m_groupWidget);
    connect(m_groupWidget, &GroupWidget::sig_center, m_mapWindow, &MapWindow::slot_centerOnWorldPos);
    auto *canvas = getCanvas();
    connect(m_groupWidget, &GroupWidget::sig_characterUpdated, canvas, [canvas](SharedGroupChar) {
        canvas->slot_requestUpdate();
    });

    // View -> Side Panels -> Room Panel (Mobs)
    m_roomManager = new RoomManager(this);
    m_roomManager->setObjectName("RoomManager");
    deref(m_gameObserver).sig2_sentToUserGmcp.connect(m_lifetime, [this](const GmcpMessage &gmcp) {
        deref(m_roomManager).slot_parseGmcpInput(gmcp);
    });
    m_roomWidget = new RoomWidget(deref(m_roomManager), this);
    m_dockDialogRoom = new QDockWidget(tr("Room Panel"), this);
    m_dockDialogRoom->setObjectName("DockWidgetRoom");
    m_dockDialogRoom->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    m_dockDialogRoom->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable
                                  | QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::BottomDockWidgetArea, m_dockDialogRoom);
    m_dockDialogRoom->setWidget(m_roomWidget);
    m_dockDialogRoom->hide();

    // Find Room Dialog
    m_findRoomsDlg = new FindRoomsDlg(*m_mapData, this);
    m_findRoomsDlg->setObjectName("FindRoomsDlg");

    // View -> Side Panels -> Adventure Panel (Trophy XP, Achievements, Hints, etc)
    m_dockDialogAdventure = new QDockWidget(tr("Adventure Panel"), this);
    m_dockDialogAdventure->setObjectName("DockWidgetGameConsole");
    m_dockDialogAdventure->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_dockDialogAdventure->setFeatures(QDockWidget::DockWidgetClosable
                                       | QDockWidget::DockWidgetFloatable
                                       | QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::BottomDockWidgetArea, m_dockDialogAdventure);
    m_adventureWidget = new AdventureWidget(deref(m_adventureTracker), this);
    m_dockDialogAdventure->setWidget(m_adventureWidget);
    m_dockDialogAdventure->hide();

    // View -> Side Panels -> Description / Area Panel
    m_descriptionWidget = new DescriptionWidget(this);
    m_dockDialogDescription = new QDockWidget(tr("Description Panel"), this);
    m_dockDialogDescription->setObjectName("DockWidgetDescription");
    m_dockDialogDescription->setAllowedAreas(Qt::AllDockWidgetAreas);
    m_dockDialogDescription->setFeatures(QDockWidget::DockWidgetMovable
                                         | QDockWidget::DockWidgetFloatable
                                         | QDockWidget::DockWidgetClosable);
    addDockWidget(Qt::RightDockWidgetArea, m_dockDialogDescription);
    m_dockDialogDescription->setWidget(m_descriptionWidget);

    m_mumeClock = new MumeClock(getConfig().mumeClock.startEpoch, deref(m_gameObserver), this);
    if constexpr (!NO_UPDATER) {
        m_updateDialog = new UpdateDialog(this);
    }

    createActions();
    setupToolBars();
    setupMenuBar();
    setupStatusBar();

    setCorner(Qt::TopLeftCorner, Qt::TopDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::TopDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    m_logger = new AutoLogger(this);

    // TODO move this connect() wiring into AutoLogger::ctor ?
    GameObserver &observer = deref(m_gameObserver);
    observer.sig2_connected.connect(m_lifetime, [this]() {
        //
        deref(m_logger).slot_onConnected();
    });
    observer.sig2_toggledEchoMode.connect(m_lifetime, [this](bool echo) {
        deref(m_logger).slot_shouldLog(echo);
    });
    observer.sig2_sentToMudString.connect(m_lifetime, [this](const QString &msg) {
        deref(m_logger).slot_writeToLog(msg);
    });
    observer.sig2_sentToUserString.connect(m_lifetime, [this](const QString &msg) {
        deref(m_logger).slot_writeToLog(msg);
    });

    m_listener = new ConnectionListener(deref(m_mapData),
                                        deref(m_pathMachine),
                                        deref(m_prespammedPath),
                                        deref(m_groupManager),
                                        deref(m_mumeClock),
                                        deref(getCanvas()),
                                        deref(m_gameObserver),
                                        this);

    // update connections
    wireConnections();

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

    showStatusBarAct->setChecked(getConfig().general.showStatusBar);
    slot_setShowStatusBar();

    showScrollBarsAct->setChecked(getConfig().general.showScrollBars);
    slot_setShowScrollBars();

    if constexpr (CURRENT_PLATFORM != PlatformEnum::Mac) {
        showMenuBarAct->setChecked(getConfig().general.showMenuBar);
        slot_setShowMenuBar();
    }

    connect(m_mapData,
            &MapData::sig_checkMapConsistency,
            this,
            &MainWindow::slot_checkMapConsistency);
    connect(m_mapData, &MapData::sig_generateBaseMap, this, &MainWindow::slot_generateBaseMap);

    readSettings();
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
                                     .arg(QString::fromUtf8(e.what()));
        QMessageBox::critical(this, tr("mmapper"), errorMsg);
    }

    if constexpr (!NO_UPDATER) {
        auto should_check_for_update = []() -> bool { return getConfig().general.checkForUpdate; };
        // Raise the update dialog if an update is found
        if (should_check_for_update()) {
            m_updateDialog->open();
        }
    }
}

void MainWindow::readSettings()
{
    struct NODISCARD MySettings final
    {
#define X_DECL(name) decltype(Configuration::GeneralSettings::name) name{};
#define X_COPY(name) (my_settings.name) = (actual_settings.name);
#define XFOREACH_MY_SETTINGS(X) \
    X(firstRun) \
    X(windowGeometry) \
    X(windowState)

        // member variable declarations
        XFOREACH_MY_SETTINGS(X_DECL)

        NODISCARD static MySettings get()
        {
            // REVISIT: Does this actually need to take the lock?
            auto &&config = getConfig();
            const auto &actual_settings = config.general;
            MySettings my_settings{};
            XFOREACH_MY_SETTINGS(X_COPY)
            return my_settings;
        }

#undef X_COPY
#undef X_DECL
#undef XFOREACH_MY_SETTINGS
    };

    MySettings settings = MySettings::get();

    if (settings.firstRun) {
        adjustSize();
        setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                        Qt::AlignCenter,
                                        size(),
                                        qApp->primaryScreen()->availableGeometry()));

    } else {
        if (!restoreState(settings.windowState)) {
            qWarning() << "Unable to restore toolbars and dockwidgets state";
        }
        if (!restoreGeometry(settings.windowGeometry)) {
            qWarning() << "Unable to restore window geometry";
        }
        raise();

        // Check if the window was moved to a screen with a different DPI
        getCanvas()->screenChanged();
    }
}

void MainWindow::writeSettings()
{
    auto &savedConfig = setConfig().general;
    savedConfig.windowGeometry = saveGeometry();
    savedConfig.windowState = saveState();
}

void MainWindow::wireConnections()
{
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

    connect(m_pathMachine,
            &Mmapper2PathMachine::sig_playerMoved,
            m_descriptionWidget,
            [this](const RoomId &id) {
                if (const auto room = m_mapData->getRoomHandle(id)) {
                    m_descriptionWidget->updateRoom(room);
                }
            });

    connect(m_mapData, &MapData::sig_onPositionChange, this, [this]() {
        m_pathMachine->onPositionChange(m_mapData->getCurrentRoomId());
        m_descriptionWidget->updateRoom(m_mapData->getCurrentRoom());
    });

    connect(m_mapData,
            &MapData::sig_onForcedPositionChange,
            canvas,
            &MapCanvas::slot_onForcedPositionChange);

    // moved to mapwindow
    connect(m_mapData, &MapData::sig_mapSizeChanged, m_mapWindow, &MapWindow::slot_setScrollBars);

    connect(m_prespammedPath, &PrespammedPath::sig_update, canvas, &MapCanvas::slot_requestUpdate);

    connect(m_mapData, &MapData::sig_log, this, &MainWindow::slot_log);
    connect(canvas, &MapCanvas::sig_log, this, &MainWindow::slot_log);

    connect(m_mapData, &MapData::sig_onDataChanged, this, [this]() { this->updateMapModified(); });

    connect(zoomInAct, &QAction::triggered, canvas, &MapCanvas::slot_zoomIn);
    connect(zoomOutAct, &QAction::triggered, canvas, &MapCanvas::slot_zoomOut);
    connect(zoomResetAct, &QAction::triggered, canvas, &MapCanvas::slot_zoomReset);

    connect(canvas, &MapCanvas::sig_newRoomSelection, this, &MainWindow::slot_newRoomSelection);
    connect(canvas, &MapCanvas::sig_selectionChanged, this, [this]() {
        if (m_roomSelection != nullptr && m_roomSelection->size()) {
            auto anyRoomAtOffset = [this](const Coordinate &offset) -> bool {
                const auto &sel = deref(m_roomSelection);
                for (const RoomId id : sel) {
                    const Coordinate &here = m_mapData->getRoomHandle(id).getPosition();
                    const Coordinate target = here + offset;
                    if (m_mapData->findRoomHandle(target)) {
                        return true;
                    }
                }
                return false;
            };
            moveUpRoomSelectionAct->setEnabled(!anyRoomAtOffset(Coordinate(0, 0, 1)));
            moveDownRoomSelectionAct->setEnabled(!anyRoomAtOffset(Coordinate(0, 0, -1)));
            mergeUpRoomSelectionAct->setEnabled(anyRoomAtOffset(Coordinate(0, 0, 1)));
            mergeDownRoomSelectionAct->setEnabled(anyRoomAtOffset(Coordinate(0, 0, -1)));
        }
    });
    connect(canvas,
            &MapCanvas::sig_newConnectionSelection,
            this,
            &MainWindow::slot_newConnectionSelection);
    connect(canvas,
            &MapCanvas::sig_newInfomarkSelection,
            this,
            &MainWindow::slot_newInfomarkSelection);
    connect(canvas, &QWidget::customContextMenuRequested, this, &MainWindow::slot_showContextMenu);

    // Group
    connect(m_groupManager, &Mmapper2Group::sig_log, this, &MainWindow::slot_log);
    connect(m_groupManager,
            &Mmapper2Group::sig_updateMapCanvas,
            canvas,
            &MapCanvas::slot_requestUpdate);

    connect(m_mapData, &MapFrontend::sig_clearingMap, m_groupWidget, &GroupWidget::slot_mapUnloaded);

    connect(m_mumeClock, &MumeClock::sig_log, this, &MainWindow::slot_log);

    connect(m_listener, &ConnectionListener::sig_log, this, &MainWindow::slot_log);
    connect(m_dockDialogClient,
            &QDockWidget::visibilityChanged,
            m_clientWidget,
            &ClientWidget::slot_onVisibilityChanged);
    connect(m_listener, &ConnectionListener::sig_clientSuccessfullyConnected, this, [this]() {
        if (!m_clientWidget->isUsingClient()) {
            m_dockDialogClient->hide();
        }
    });
    connect(m_clientWidget, &ClientWidget::sig_relayMessage, this, [this](const QString &message) {
        showStatusShort(message);
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

void MainWindow::slot_log(const QString &mod, const QString &message)
{
    logWindow->append(QString("[%1] %2").arg(mod, message));
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

    m_undoAction = new QAction(QIcon::fromTheme("edit-undo"), tr("&Undo"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setStatusTip(tr("Undo the last action"));
    connect(m_undoAction, &QAction::triggered, m_mapData, &MapData::slot_undo);
    connect(m_mapData, &MapData::sig_undoAvailable, m_undoAction, &QAction::setEnabled);
    m_undoAction->setEnabled(false);

    m_redoAction = new QAction(QIcon::fromTheme("edit-redo"), tr("&Redo"), this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setStatusTip(tr("Redo the last undone action"));
    connect(m_redoAction, &QAction::triggered, m_mapData, &MapData::slot_redo);
    connect(m_mapData, &MapData::sig_redoAvailable, m_redoAction, &QAction::setEnabled);
    m_redoAction->setEnabled(false);

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

    actionReportIssue = new QAction(QIcon::fromTheme("help-browser"),
                                    tr("Report an &Issue..."),
                                    this);
    actionReportIssue->setStatusTip(tr("Open the MMapper issue tracker in your browser"));
    connect(actionReportIssue, &QAction::triggered, this, &MainWindow::onReportIssueTriggered);

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

    alwaysOnTopAct = new QAction(tr("Always On Top"), this);
    alwaysOnTopAct->setCheckable(true);
    connect(alwaysOnTopAct, &QAction::triggered, this, &MainWindow::slot_alwaysOnTop);

    showStatusBarAct = new QAction(tr("Always Show Status Bar"), this);
    showStatusBarAct->setCheckable(true);
    connect(showStatusBarAct, &QAction::triggered, this, &MainWindow::slot_setShowStatusBar);

    showScrollBarsAct = new QAction(tr("Always Show Scrollbars"), this);
    showScrollBarsAct->setCheckable(true);
    connect(showScrollBarsAct, &QAction::triggered, this, &MainWindow::slot_setShowScrollBars);

    if constexpr (CURRENT_PLATFORM != PlatformEnum::Mac) {
        showMenuBarAct = new QAction(tr("Always Show Menubar"), this);
        showMenuBarAct->setCheckable(true);
        connect(showMenuBarAct, &QAction::triggered, this, &MainWindow::slot_setShowMenuBar);
    }

    layerUpAct = new QAction(QIcon::fromTheme("go-up", QIcon(":/icons/layerup.png")),
                             tr("Layer Up"),
                             this);
    layerUpAct->setShortcut(tr([]() -> const char * {
        // Technically tr() could convert Ctrl to Meta, right?
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac) {
            return "Meta+Tab";
        }
        return "Ctrl+Tab";
    }()));
    layerUpAct->setStatusTip(tr("Layer Up"));
    connect(layerUpAct, &QAction::triggered, this, &MainWindow::slot_onLayerUp);
    layerDownAct = new QAction(QIcon::fromTheme("go-down", QIcon(":/icons/layerdown.png")),
                               tr("Layer Down"),
                               this);

    layerDownAct->setShortcut(tr([]() -> const char * {
        // Technically tr() could convert Ctrl to Meta, right?
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac) {
            return "Meta+Shift+Tab";
        }
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
    mouseMode.modeInfomarkSelectAct = new QAction(QIcon(":/icons/infomarkselection.png"),
                                                  tr("Select Markers"),
                                                  this);
    mouseMode.modeInfomarkSelectAct->setStatusTip(tr("Select Info Markers"));
    mouseMode.modeInfomarkSelectAct->setCheckable(true);
    connect(mouseMode.modeInfomarkSelectAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onModeInfomarkSelect);
    mouseMode.modeCreateInfomarkAct = new QAction(QIcon(":/icons/infomarkcreate.png"),
                                                  tr("Create New Markers"),
                                                  this);
    mouseMode.modeCreateInfomarkAct->setStatusTip(tr("Create New Info Markers"));
    mouseMode.modeCreateInfomarkAct->setCheckable(true);
    connect(mouseMode.modeCreateInfomarkAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onModeCreateInfomarkSelect);
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
    mouseMode.mouseModeActGroup->addAction(mouseMode.modeInfomarkSelectAct);
    mouseMode.mouseModeActGroup->addAction(mouseMode.modeCreateInfomarkAct);
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

    gotoRoomAct = new QAction(QIcon(":/icons/goto.png"), tr("Move to selected room"), this);
    gotoRoomAct->setStatusTip(tr("Move to selected room"));
    gotoRoomAct->setCheckable(false);
    gotoRoomAct->setEnabled(false);
    connect(gotoRoomAct, &QAction::triggered, this, [this]() {
        if (m_roomSelection == nullptr) {
            return;
        }

        auto &mapData = deref(m_mapData);
        auto &sel = deref(m_roomSelection);
        sel.removeMissing(mapData);

        if (m_roomSelection->size() == 1) {
            const RoomId id = m_roomSelection->getFirstRoomId();
            m_mapData->setRoom(id);
        }
    });

    forceRoomAct = new QAction(QIcon(":/icons/force.png"),
                               tr("Update selected room with last movement"),
                               this);
    forceRoomAct->setStatusTip(tr("Update selected room with last movement"));
    forceRoomAct->setCheckable(false);
    forceRoomAct->setEnabled(false);
    connect(forceRoomAct, &QAction::triggered, this, &MainWindow::slot_forceMapperToRoom);

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

    infomarkActions.editInfomarkAct = new QAction(QIcon(":/icons/infomarkedit.png"),
                                                  tr("Edit Selected Markers"),
                                                  this);
    infomarkActions.editInfomarkAct->setStatusTip(tr("Edit Selected Info Markers"));
    infomarkActions.editInfomarkAct->setCheckable(true);
    connect(infomarkActions.editInfomarkAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onEditInfomarkSelection);
    infomarkActions.deleteInfomarkAct = new QAction(QIcon(":/icons/infomarkdelete.png"),
                                                    tr("Delete Selected Markers"),
                                                    this);
    infomarkActions.deleteInfomarkAct->setStatusTip(tr("Delete Selected Info Markers"));
    infomarkActions.deleteInfomarkAct->setCheckable(true);
    connect(infomarkActions.deleteInfomarkAct,
            &QAction::triggered,
            this,
            &MainWindow::slot_onDeleteInfomarkSelection);

    infomarkActions.infomarkGroup = new QActionGroup(this);
    infomarkActions.infomarkGroup->setExclusive(false);
    infomarkActions.infomarkGroup->addAction(infomarkActions.deleteInfomarkAct);
    infomarkActions.infomarkGroup->addAction(infomarkActions.editInfomarkAct);
    infomarkActions.infomarkGroup->setEnabled(false);

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

    rebuildMeshesAct = new QAction(QIcon(":/icons/graphicscfg.png"), tr("&Rebuild World"), this);
    rebuildMeshesAct->setStatusTip(tr("Reconstruct the world mesh to fix graphical rendering bugs"));
    rebuildMeshesAct->setCheckable(false);
    connect(rebuildMeshesAct, &QAction::triggered, getCanvas(), &MapCanvas::slot_rebuildMeshes);
}

static void setConfigMapMode(const MapModeEnum mode)
{
    setConfig().general.mapMode = mode;
}

void MainWindow::slot_onPlayMode()
{
    // map mode can only create rooms, but play mode can make changes
    setConfigMapMode(MapModeEnum::PLAY);
    modeMenu->setIcon(mapperMode.playModeAct->icon());
    // needed so that the menu updates to reflect state set by commands
    mapperMode.playModeAct->setChecked(true);
}

void MainWindow::slot_onMapMode()
{
    slot_log("MainWindow",
             "Map mode selected - new rooms are created when entering unmapped areas.");
    setConfigMapMode(MapModeEnum::MAP);
    modeMenu->setIcon(mapperMode.mapModeAct->icon());
    // needed so that the menu updates to reflect state set by commands
    mapperMode.mapModeAct->setChecked(true);
}

void MainWindow::slot_onOfflineMode()
{
    slot_log("MainWindow", "Offline emulation mode selected - learn new areas safely.");
    setConfigMapMode(MapModeEnum::OFFLINE);
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
    // REVISIT: Which of these should be allowed during async actions?
    // Note: Some of the ones that would launch async actions (e.g. loading/saving)
    // would probably be blocked because we only allow one async action at a time,
    // but they'll probably need to be individually tested to see what actually happens.
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
}

void MainWindow::hideCanvas(const bool hide)
{
    // REVISIT: It seems that updates don't work if the canvas is hidden,
    // so we may want to save mapChanged() and other similar requests
    // and send them after we show the canvas.
    if (MapCanvas *const canvas = getCanvas()) {
        if (hide) {
            canvas->hide();
        } else {
            canvas->show();
        }
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
    editMenu->addAction(m_undoAction);
    editMenu->addAction(m_redoAction);
    editMenu->addSeparator();

    QMenu *infoMarkMenu = editMenu->addMenu(QIcon(":/icons/infomarkselection.png"), tr("M&arkers"));
    infoMarkMenu->setStatusTip("Info markers");
    infoMarkMenu->addAction(mouseMode.modeInfomarkSelectAct);
    infoMarkMenu->addSeparator();
    infoMarkMenu->addAction(mouseMode.modeCreateInfomarkAct);
    infoMarkMenu->addAction(infomarkActions.editInfomarkAct);
    infoMarkMenu->addAction(infomarkActions.deleteInfomarkAct);

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
    toolbars->addAction(settingsToolBar->toggleViewAction());
    QMenu *sidepanels = viewMenu->addMenu(tr("&Side Panels"));
    sidepanels->addAction(m_dockDialogLog->toggleViewAction());
    sidepanels->addAction(m_dockDialogClient->toggleViewAction());
    sidepanels->addAction(m_dockDialogGroup->toggleViewAction());
    sidepanels->addAction(m_dockDialogRoom->toggleViewAction());
    sidepanels->addAction(m_dockDialogAdventure->toggleViewAction());
    sidepanels->addAction(m_dockDialogDescription->toggleViewAction());
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
    if constexpr (CURRENT_PLATFORM != PlatformEnum::Mac) {
        viewMenu->addAction(showMenuBarAct);
    }
    viewMenu->addAction(alwaysOnTopAct);

    settingsMenu = menuBar()->addMenu(tr("&Tools"));
    QMenu *clientMenu = settingsMenu->addMenu(QIcon(":/icons/terminal.png"),
                                              tr("&Integrated Mud Client"));
    clientMenu->addAction(clientAct);
    clientMenu->addAction(saveLogAct);
    QMenu *pathMachineMenu = settingsMenu->addMenu(QIcon(":/icons/goto.png"), tr("&Path Machine"));
    pathMachineMenu->addAction(mouseMode.modeRoomSelectAct);
    pathMachineMenu->addSeparator();
    pathMachineMenu->addAction(gotoRoomAct);
    pathMachineMenu->addAction(forceRoomAct);
    pathMachineMenu->addAction(releaseAllPathsAct);

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(settingUpMmapperAct);
    helpMenu->addAction(actionReportIssue);
    if constexpr (!NO_UPDATER) {
        helpMenu->addAction(mmapperCheckForUpdateAct);
    }
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
        // ^^^ Let's enforce that with a variant then?
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
                contextMenu.addAction(gotoRoomAct);
                contextMenu.addAction(forceRoomAct);
            }
        }
        if (m_infoMarkSelection != nullptr && !m_infoMarkSelection->empty()) {
            if (m_roomSelection != nullptr) {
                contextMenu.addSeparator();
            }
            contextMenu.addAction(infomarkActions.editInfomarkAct);
            contextMenu.addAction(infomarkActions.deleteInfomarkAct);
        }
    }
    contextMenu.addSeparator();
    QMenu *mouseMenu = contextMenu.addMenu(QIcon::fromTheme("input-mouse"), "Mouse Mode");
    mouseMenu->addAction(mouseMode.modeMoveSelectAct);
    mouseMenu->addAction(mouseMode.modeRoomRaypickAct);
    mouseMenu->addAction(mouseMode.modeRoomSelectAct);
    mouseMenu->addAction(mouseMode.modeInfomarkSelectAct);
    mouseMenu->addAction(mouseMode.modeConnectionSelectAct);
    mouseMenu->addAction(mouseMode.modeCreateInfomarkAct);
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
    statusBar()->setVisible(showStatusBar);
    setConfig().general.showStatusBar = showStatusBar;
    show();
}

void MainWindow::slot_setShowScrollBars()
{
    const bool showScrollBars = this->showScrollBarsAct->isChecked();
    setConfig().general.showScrollBars = showScrollBars;
    m_mapWindow->updateScrollBars();
    show();
}

void MainWindow::slot_setShowMenuBar()
{
    const bool showMenuBar = this->showMenuBarAct->isChecked();
    setConfig().general.showMenuBar = showMenuBar;
    show();

    m_dockDialogAdventure->setMouseTracking(!showMenuBar);
    m_dockDialogClient->setMouseTracking(!showMenuBar);
    m_dockDialogGroup->setMouseTracking(!showMenuBar);
    m_dockDialogLog->setMouseTracking(!showMenuBar);
    m_dockDialogRoom->setMouseTracking(!showMenuBar);
    getCanvas()->setMouseTracking(!showMenuBar);

    if (showMenuBar) {
        menuBar()->show();
        m_dockDialogAdventure->removeEventFilter(this);
        m_dockDialogClient->removeEventFilter(this);
        m_dockDialogGroup->removeEventFilter(this);
        m_dockDialogLog->removeEventFilter(this);
        m_dockDialogRoom->removeEventFilter(this);
        getCanvas()->removeEventFilter(this);
    } else {
        m_dockDialogAdventure->installEventFilter(this);
        m_dockDialogClient->installEventFilter(this);
        m_dockDialogGroup->installEventFilter(this);
        m_dockDialogLog->installEventFilter(this);
        m_dockDialogRoom->installEventFilter(this);
        getCanvas()->installEventFilter(this);
    }
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
    mouseModeToolBar->addAction(mouseMode.modeInfomarkSelectAct);
    mouseModeToolBar->addAction(mouseMode.modeCreateInfomarkAct);
    mouseModeToolBar->hide();

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
    pathMachineToolBar->addAction(gotoRoomAct);
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
    showStatusForever(tr("Say friend and enter..."));
    statusBar()->insertPermanentWidget(0, new MumeClockWidget(m_mumeClock, this));

    XPStatusWidget *xpStatus = new XPStatusWidget(*m_adventureTracker, statusBar(), this);
    xpStatus->setToolTip("Click to toggle the Adventure Panel.");

    connect(xpStatus, &QPushButton::clicked, [this]() {
        m_dockDialogAdventure->setVisible(!m_dockDialogAdventure->isVisible());
    });
    statusBar()->insertPermanentWidget(0, xpStatus);

    QLabel *pathmachineStatus = new QLabel(statusBar());
    connect(m_pathMachine, &Mmapper2PathMachine::sig_state, pathmachineStatus, &QLabel::setText);
    statusBar()->insertPermanentWidget(0, pathmachineStatus);
}

void MainWindow::slot_onPreferences()
{
    if (m_configDialog == nullptr) {
        m_configDialog = std::make_unique<ConfigDialog>(this);
    }

    connect(m_configDialog.get(),
            &ConfigDialog::sig_graphicsSettingsChanged,
            m_mapWindow,
            &MapWindow::slot_graphicsSettingsChanged);
    connect(m_configDialog.get(),
            &ConfigDialog::sig_groupSettingsChanged,
            m_groupManager,
            &Mmapper2Group::slot_groupSettingsChanged);
    m_configDialog->show();
}

void MainWindow::slot_newRoomSelection(const SigRoomSelection &rs)
{
    const bool isValidSelection = rs.isValid();
    const size_t selSize = !isValidSelection ? 0 : rs.deref().size();

    m_roomSelection = !isValidSelection ? nullptr : rs.getShared();
    selectedRoomActGroup->setEnabled(isValidSelection);
    gotoRoomAct->setEnabled(selSize == 1);
    forceRoomAct->setEnabled(selSize == 1 && m_pathMachine->hasLastEvent());

    if (isValidSelection) {
        const auto msg = QString("Selection: %1 room%2").arg(selSize).arg(basic_plural(selSize));
        showStatusLong(msg);
    }
}

void MainWindow::slot_newConnectionSelection(ConnectionSelection *const cs)
{
    m_connectionSelection = (cs != nullptr) ? cs->shared_from_this() : nullptr;
    selectedConnectionActGroup->setEnabled(m_connectionSelection != nullptr);
}

void MainWindow::slot_newInfomarkSelection(InfomarkSelection *const is)
{
    m_infoMarkSelection = (is != nullptr) ? is->shared_from_this() : nullptr;
    infomarkActions.infomarkGroup->setEnabled(m_infoMarkSelection != nullptr);

    if (m_infoMarkSelection != nullptr) {
        showStatusLong(QString("Selection: %1 mark%2")
                           .arg(m_infoMarkSelection->size())
                           .arg((m_infoMarkSelection->size() != 1) ? "s" : ""));
        if (m_infoMarkSelection->empty()) {
            // Create a new infomark if its an empty selection
            slot_onEditInfomarkSelection();
        }
    }
}

bool MainWindow::eventFilter(QObject *const obj, QEvent *const event)
{
    if (QApplication::activeWindow() == this && event->type() == QEvent::MouseMove) {
        if (const auto *const mouseEvent = dynamic_cast<QMouseEvent *>(event)) {
            QRect rect = geometry();
            rect.setHeight(menuBar()->sizeHint().height());
            if (rect.contains(mouseEvent->globalPosition().toPoint())) {
                menuBar()->show();
            } else {
                menuBar()->hide();
            }
        }
    }
    return QObject::eventFilter(obj, event);
}

void MainWindow::closeEvent(QCloseEvent *const event)
{
    // REVISIT: wait and see if we're actually exiting first?
    writeSettings();

    if (!maybeSave()) {
        event->ignore();
        return;
    }

    if (m_asyncTask) {
        qInfo() << "Attempting to async task for faster shutdown";
        m_progressDlg->reject();
    }
    event->accept();
}

void MainWindow::showEvent(QShowEvent *const event)
{
    // Check screen DPI each time MMapper's window is shown
    getCanvas()->screenChanged();

    static std::once_flag flag;
    std::call_once(flag, [this]() {
        // Start services on startup
        startServices();

        connect(window()->windowHandle(), &QWindow::screenChanged, this, [this]() {
            MapWindow &window = deref(m_mapWindow);
            CanvasDisabler canvasDisabler{window};
            MapCanvas &canvas = deref(getCanvas());
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
    {
        auto &mapData = deref(m_mapData);
        mapData.clear();
        mapData.setFileName("", false);
        mapData.forcePosition(Coordinate{});
    }

    setCurrentFile("");
    getCanvas()->slot_dataLoaded();
    m_groupWidget->slot_mapLoaded();
    m_descriptionWidget->updateRoom(RoomHandle{});

    /*
    updateMapModified();
    mapChanged();
    */
}

void MainWindow::slot_open()
{
    if (!maybeSave()) {
        return;
    }

    const QString fileName = chooseLoadOrMergeFileName();
    if (fileName.isEmpty()) {
        showStatusShort(tr("No filename provided"));
        return;
    }
    QFileInfo file(fileName);
    auto &savedLastMapDir = setConfig().autoLoad.lastMapDirectory;
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

void MainWindow::slot_about()
{
    AboutDialog about(this);
    about.exec();
}

MainWindow::ProgressDialogLifetime MainWindow::createNewProgressDialog(const QString &text,
                                                                       const bool allow_cancel)
{
    m_progressDlg = std::make_unique<QProgressDialog>(this);
    QProgressDialog *const progress = m_progressDlg.get();
    if (allow_cancel) {
        progress->setCancelButtonText("Cancel");
    } else {
        QPushButton *const cb = new QPushButton("Cancel", progress);
        cb->setEnabled(false);
        progress->setCancelButton(cb);
    }

    progress->setWindowTitle(text);
    progress->setMinimum(0);
    progress->setMaximum(100);
    progress->setValue(0);
    progress->setMinimumWidth(this->width() / 4);
    progress->showNormal();
    progress->raise();
    progress->activateWindow();
    progress->focusWidget();

    connect(progress, &QProgressDialog::canceled, this, [this, allow_cancel]() {
        if (allow_cancel) {
            qInfo() << "QProgressDialog::canceled()";
            this->m_asyncTask.request_cancel();
        }
    });
    connect(progress, &QProgressDialog::rejected, this, [this, allow_cancel]() {
        if (allow_cancel) {
            qInfo() << "QProgressDialog::rejected()";
            this->m_asyncTask.request_cancel();
        }
    });

    return ProgressDialogLifetime{*this};
}

void MainWindow::endProgressDialog()
{
    m_progressDlg.reset();
}

void MainWindow::setMapModified(bool modified)
{
    setWindowModified(modified);
    saveAct->setEnabled(modified);
    if (modified) {
        deref(m_mapWindow).hideSplashImage();
    }
}

void MainWindow::updateMapModified()
{
    setMapModified(m_mapData->isModified());
    getCanvas()->update();
}

void MainWindow::percentageChanged(const uint32_t p)
{
    if (m_progressDlg == nullptr) {
        return;
    }

    m_progressDlg->setValue(static_cast<int>(p));
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}

void MainWindow::showWarning(const QString &s)
{
    // REVISIT: shouldn't the warning have "this" as parent?
    // REVISIT: shouldn't this also say MMapper?
    QMessageBox::warning(nullptr, tr("Application"), s);
}

void MainWindow::showAsyncFailure(const QString &fileName,
                                  const AsyncTypeEnum mode,
                                  const bool wasCanceled)
{
    const char *const modeName = get_type_name(mode);
    const char *const msg = wasCanceled ? "User canceled the %1 of file %2"
                                        : "Failed to %1 file %2";
    showWarning(tr(msg).arg(modeName, fileName));
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

void MainWindow::setCurrentFile(const QString &fileName)
{
    QFileInfo file(fileName);
    const auto shownName = fileName.isEmpty() ? "Untitled" : file.fileName();
    const auto fileSuffix = ((m_mapData && m_mapData->isFileReadOnly())
                             || (!fileName.isEmpty() && !file.isWritable()))
                                ? " [read-only]"
                                : "";
    const auto appSuffix = isMMapperBeta() ? " Beta" : "";

    // [*] is Qt black magic for only showing the '*' if the document has been modified.
    // From https://doc.qt.io/qt-5/qwidget.html:
    // > The window title must contain a "[*]" placeholder, which indicates where the '*' should appear.
    // > Normally, it should appear right after the file name (e.g., "document1.txt[*] - Text Editor").
    // > If the window isn't modified, the placeholder is simply removed.

    mmqt::setWindowTitle2(*this,
                          QString("MMapper%1").arg(appSuffix),
                          QString("%1[*]%2").arg(shownName).arg(fileSuffix));
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

void MainWindow::slot_onModeInfomarkSelect()
{
    setCanvasMouseMode(CanvasMouseModeEnum::SELECT_INFOMARKS);
}

void MainWindow::slot_onModeCreateInfomarkSelect()
{
    setCanvasMouseMode(CanvasMouseModeEnum::CREATE_INFOMARKS);
}

void MainWindow::slot_onEditInfomarkSelection()
{
    if (m_infoMarkSelection == nullptr) {
        return;
    }

    InfomarksEditDlg dlg(this);
    dlg.setInfomarkSelection(m_infoMarkSelection, m_mapData, getCanvas());
    dlg.exec();
    dlg.show();
}

void MainWindow::slot_onCreateRoom()
{
    deref(getCanvas()).slot_createRoom();
}

void MainWindow::slot_onEditRoomSelection()
{
    if (m_roomSelection == nullptr) {
        return;
    }

    if (m_roomEditAttrDlg != nullptr) {
        m_roomEditAttrDlg->setFocus();
        return;
    }

    m_roomEditAttrDlg = std::make_unique<RoomEditAttrDlg>(this);
    {
        auto &roomEditDialog = deref(m_roomEditAttrDlg);
        roomEditDialog.setRoomSelection(m_roomSelection, m_mapData, getCanvas());
        roomEditDialog.show();
        connect(&roomEditDialog, &QDialog::finished, this, [this](MAYBE_UNUSED int result) {
            m_roomEditAttrDlg.reset();
        });
    }
}

void MainWindow::slot_onDeleteInfomarkSelection()
{
    if (m_infoMarkSelection == nullptr) {
        return;
    }

    {
        const auto tmp = std::exchange(m_infoMarkSelection, nullptr);
        ChangeList changes;
        for (const InfomarkId id : tmp->getMarkerList()) {
            changes.add(Change{infomark_change_types::RemoveInfomark{id}});
        }
        if (!changes.empty()) {
            m_mapData->applyChanges(changes);
        }
    }

    MapCanvas *const canvas = getCanvas();
    canvas->slot_clearInfomarkSelection();
}

void MainWindow::slot_onDeleteRoomSelection()
{
    if (m_roomSelection == nullptr) {
        return;
    }

    applyGroupAction([](const RawRoom &room) -> Change {
        return Change{room_change_types::RemoveRoom{room.getId()}};
    });
    getCanvas()->slot_clearRoomSelection();
}

void MainWindow::slot_onDeleteConnectionSelection()
{
    if (m_connectionSelection == nullptr) {
        return; // previously called mapChanged for no good reason
    }

    const auto &first = m_connectionSelection->getFirst();
    const auto &second = m_connectionSelection->getSecond();
    const auto &r1 = first.room;
    const auto &r2 = second.room;
    if (!r1 || !r2) {
        return; // previously called mapChanged for no good reason
    }

    const ExitDirEnum dir1 = first.direction;
    MAYBE_UNUSED const ExitDirEnum dir2 = second.direction;
    const RoomId &id1 = r1.getId();
    const RoomId &id2 = r2.getId();

    getCanvas()->slot_clearConnectionSelection();

    // REVISIT: dir2?
    m_mapData->applySingleChange(Change{exit_change_types::ModifyExitConnection{
        ChangeTypeEnum::Remove, id1, dir1, id2, WaysEnum::TwoWay}});
}

bool MainWindow::slot_moveRoomSelection(const Coordinate &offset)
{
    if (m_roomSelection == nullptr) {
        return false;
    }

    applyGroupAction([&offset](const RawRoom &room) -> Change {
        return Change{room_change_types::MoveRelative{room.getId(), offset}};
    });
    return true;
}

void MainWindow::slot_onMoveUpRoomSelection()
{
    if (!slot_moveRoomSelection(Coordinate(0, 0, 1))) {
        return;
    }

    slot_onLayerUp();
}

void MainWindow::slot_onMoveDownRoomSelection()
{
    if (!slot_moveRoomSelection(Coordinate(0, 0, -1))) {
        return;
    }

    slot_onLayerDown();
}

bool MainWindow::slot_mergeRoomSelection(const Coordinate &offset)
{
    if (m_roomSelection == nullptr) {
        return false;
    }

    applyGroupAction([&offset](const RawRoom &room) -> Change {
        return Change{room_change_types::MergeRelative{room.getId(), offset}};
    });
    return true;
}

void MainWindow::slot_onMergeUpRoomSelection()
{
    if (!slot_mergeRoomSelection(Coordinate(0, 0, 1))) {
        return;
    }

    slot_onLayerUp();
    slot_onModeRoomSelect();
}

void MainWindow::slot_onMergeDownRoomSelection()
{
    if (!slot_mergeRoomSelection(Coordinate(0, 0, -1))) {
        return;
    }

    slot_onLayerDown();
    slot_onModeRoomSelect();
}

void MainWindow::slot_onConnectToNeighboursRoomSelection()
{
    if (m_roomSelection == nullptr) {
        return;
    }

    auto &mapData = deref(m_mapData);
    auto &sel = deref(m_roomSelection);
    sel.removeMissing(mapData);

    const Map &map = mapData.getCurrentMap();
    ChangeList changes;
    for (const RoomId id : sel) {
        const auto &room = map.getRoomHandle(id);
        connectToNeighbors(changes, room, ConnectToNeighborsArgs{});
    }

    if (changes.getChanges().empty()) {
        return;
    }

    mapData.applyChanges(changes);
}

void MainWindow::slot_forceMapperToRoom()
{
    if (m_roomSelection == nullptr) {
        return;
    }

    auto &mapData = deref(m_mapData);
    auto &sel = deref(m_roomSelection);
    sel.removeMissing(mapData);

    if (m_roomSelection->size() == 1) {
        const RoomId id = m_roomSelection->getFirstRoomId();
        m_mapData->setRoom(id);
        m_pathMachine->forceUpdate(id);
    }
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif
void MainWindow::slot_onCheckForUpdate()
{
    assert(!NO_UPDATER);
    m_updateDialog->show();
    m_updateDialog->open();
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif

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

void MainWindow::onReportIssueTriggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/MUME/MMapper/issues"));
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
    if (MapCanvas *const canvas = getCanvas()) {
        canvas->slot_mapChanged();
    }
}

void MainWindow::setCanvasMouseMode(const CanvasMouseModeEnum mode)
{
    if (MapCanvas *const canvas = getCanvas()) {
        canvas->slot_setCanvasMouseMode(mode);
    }
}

void MainWindow::applyGroupAction(const std::function<Change(const RawRoom &)> &getChange)
{
    m_mapData->applyChangesToList(deref(m_roomSelection), getChange);
}

void MainWindow::showStatusInternal(const QString &text, int duration)
{
    statusBar()->showMessage(text, duration);
}

void MainWindow::onSuccessfulLoad(const MapLoadData &mapLoadData)
{
    auto &mapData = deref(m_mapData);
    auto &mapCanvas = deref(getCanvas());
    auto &groupWidget = deref(m_groupWidget);
    auto &pathMachine = deref(m_pathMachine);

    mapData.setMapData(mapLoadData);
    mapData.checkSize();

    // TODO: convert from slot_xxx prefix if these are no longer used as slots anywhere.
    mapCanvas.slot_dataLoaded();
    groupWidget.slot_mapLoaded();
    pathMachine.onMapLoaded();
    if (const auto room = mapData.getCurrentRoom()) {
        auto &widget = deref(m_descriptionWidget);
        widget.updateRoom(room);
    }

    // Should this be part of mapChanged?
    updateMapModified();
    mapChanged();

    setCurrentFile(mapData.getFileName());
    showStatusShort(tr("File loaded"));

    global::sendToUser("Map loaded.\n");
}

void MainWindow::onSuccessfulMerge(const Map &map)
{
    auto &mapData = deref(m_mapData);
    auto &mapCanvas = deref(getCanvas());
    auto &groupWidget = deref(m_groupWidget);

    mapData.setCurrentMap(map);
    mapData.checkSize();

    // FIXME: mapData.setMapData() or mapData.checkSize() kicks off an async remesh,
    // and then slot_dataLoaded() immediately sets it to be ignored and starts another
    // one in parallel? (see: ignorePendingRemesh)

    mapCanvas.slot_dataLoaded();
    groupWidget.slot_mapLoaded();
    updateMapModified();
    mapChanged();
    showStatusShort(tr("File merged"));
}

void MainWindow::onSuccessfulSave(const SaveModeEnum mode,
                                  const SaveFormatEnum format,
                                  const QString &fileName)
{
    auto &mapData = deref(m_mapData);

    if (mode == SaveModeEnum::FULL && format == SaveFormatEnum::MM2) {
        mapData.setFileName(fileName, !QFileInfo(fileName).isWritable());
        setCurrentFile(fileName);
        mapData.currentHasBeenSaved();
    }

    showStatusShort(tr("File saved"));
    updateMapModified();

    QFileInfo file{fileName};

    // Update last directory
    auto &config = setConfig().autoLoad;
    config.lastMapDirectory = file.absoluteDir().absolutePath();

    const QString &absoluteFilePath = file.absoluteFilePath();
    if (!config.autoLoadMap || config.fileName != absoluteFilePath) {
        // Check if this should be the new autoload map
        QMessageBox dlg(QMessageBox::Question,
                        "Autoload Map?",
                        "Autoload this map when MMapper starts?",
                        QMessageBox::StandardButtons{QMessageBox::Yes | QMessageBox::No},
                        this);
        if (dlg.exec() == QMessageBox::Yes) {
            config.autoLoadMap = true;
            config.fileName = absoluteFilePath;
        }
    }
}
