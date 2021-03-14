#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>
#include <optional>
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

#include "../configuration/configuration.h"
#include "../display/CanvasMouseModeEnum.h"
#include "../mapdata/roomselection.h"
#include "../pandoragroup/mmapper2group.h"

class AutoLogger;
class AbstractAction;
class ClientWidget;
class ConfigDialog;
class ConnectionListener;
class ConnectionSelection;
class FindRoomsDlg;
class GroupWidget;
class InfoMarkSelection;
class MapCanvas;
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
class QShowEvent;
class QTextBrowser;
class QToolBar;
class QWidget;
class UpdateDialog;
class RoomSelection;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
    ~MainWindow() final;

    enum class NODISCARD SaveModeEnum { FULL, BASEMAP };
    enum class NODISCARD SaveFormatEnum { MM2, WEB, MMP };
    bool saveFile(const QString &fileName, SaveModeEnum mode, SaveFormatEnum format);
    void loadFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);

signals:
    void sig_setGroupMode(GroupManagerStateEnum);
    void sig_startGroupNetwork();
    void sig_stopGroupNetwork();

public slots:
    void slot_newFile();
    void slot_open();
    void slot_reload();
    void slot_merge();
    bool slot_save();
    bool slot_saveAs();
    bool slot_exportBaseMap();
    bool slot_exportWebMap();
    bool slot_exportMmpMap();
    void slot_about();

    void slot_percentageChanged(quint32);

    void slot_log(const QString &, const QString &);

    void slot_onModeConnectionSelect();
    void slot_onModeRoomRaypick();
    void slot_onModeRoomSelect();
    void slot_onModeMoveSelect();
    void slot_onModeInfoMarkSelect();
    void slot_onModeCreateInfoMarkSelect();
    void slot_onModeCreateRoomSelect();
    void slot_onModeCreateConnectionSelect();
    void slot_onModeCreateOnewayConnectionSelect();
    void slot_onLayerUp();
    void slot_onLayerDown();
    void slot_onLayerReset();
    void slot_onCreateRoom();
    void slot_onEditRoomSelection();
    void slot_onEditInfoMarkSelection();
    void slot_onDeleteInfoMarkSelection();
    void slot_onDeleteRoomSelection();
    void slot_onDeleteConnectionSelection();
    void slot_onMoveUpRoomSelection();
    void slot_onMoveDownRoomSelection();
    void slot_onMergeUpRoomSelection();
    void slot_onMergeDownRoomSelection();
    void slot_onConnectToNeighboursRoomSelection();
    void slot_onFindRoom();
    void slot_onLaunchClient();
    void slot_onPreferences();
    void slot_onPlayMode();
    void slot_onMapMode();
    void slot_onOfflineMode();
    void slot_setMode(MapModeEnum mode);
    void slot_alwaysOnTop();

    void slot_newRoomSelection(const SigRoomSelection &);
    void slot_newConnectionSelection(ConnectionSelection *);
    void slot_newInfoMarkSelection(InfoMarkSelection *);
    void slot_showContextMenu(const QPoint &);

    void slot_onModeGroupOff();
    void slot_onModeGroupClient();
    void slot_onModeGroupServer();
    void slot_groupNetworkStatus(bool toggle);

    void slot_onCheckForUpdate();
    void slot_voteForMUME();
    void slot_openMumeWebsite();
    void slot_openMumeForum();
    void slot_openMumeWiki();
    void slot_openSettingUpMmapper();
    void slot_openNewbieHelp();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void startServices();
    void forceNewFile();
    void showWarning(const QString &s);

private:
    MapWindow *m_mapWindow = nullptr;
    QTextBrowser *logWindow = nullptr;

    QDockWidget *m_dockDialogLog = nullptr;
    QDockWidget *m_dockDialogGroup = nullptr;
    QDockWidget *m_dockDialogClient = nullptr;

    AutoLogger *m_logger = nullptr;
    ConnectionListener *m_listener = nullptr;
    Mmapper2PathMachine *m_pathMachine = nullptr;
    MapData *m_mapData = nullptr;
    PrespammedPath *m_prespammedPath = nullptr;
    MumeClock *m_mumeClock = nullptr;

    // Pandora Ported
    FindRoomsDlg *m_findRoomsDlg = nullptr;
    Mmapper2Group *m_groupManager = nullptr;
    GroupWidget *m_groupWidget = nullptr;

    ClientWidget *m_clientWidget = nullptr;
    UpdateDialog *m_updateDialog = nullptr;

    SharedRoomSelection m_roomSelection;
    std::shared_ptr<ConnectionSelection> m_connectionSelection;
    std::shared_ptr<InfoMarkSelection> m_infoMarkSelection;

    std::unique_ptr<QProgressDialog> m_progressDlg;

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

    struct NODISCARD MouseModeActions final
    {
        QActionGroup *mouseModeActGroup = nullptr;
        QAction *modeConnectionSelectAct = nullptr;
        QAction *modeRoomRaypickAct = nullptr;
        QAction *modeRoomSelectAct = nullptr;
        QAction *modeMoveSelectAct = nullptr;
        QAction *modeInfoMarkSelectAct = nullptr;
        QAction *modeCreateInfoMarkAct = nullptr;
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

    struct NODISCARD InfoMarkActions final
    {
        QActionGroup *infoMarkGroup = nullptr;
        QAction *deleteInfoMarkAct = nullptr;
        QAction *editInfoMarkAct = nullptr;
    } infoMarkActions{};

    struct NODISCARD GroupModeActions final
    {
        QActionGroup *groupModeGroup = nullptr;
        QAction *groupOffAct = nullptr;
        QAction *groupClientAct = nullptr;
        QAction *groupServerAct = nullptr;
    } groupMode{};

    struct NODISCARD GroupNetworkActions final
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
    QAction *saveLogAct = nullptr;

    QAction *forceRoomAct = nullptr;
    QAction *releaseAllPathsAct = nullptr;
    QAction *rebuildMeshesAct = nullptr;

    std::unique_ptr<ConfigDialog> m_configDialog;

    void wireConnections();

    void createActions();
    void setupMenuBar();
    void setupToolBars();
    void setupStatusBar();

    void readSettings();
    void writeSettings();

    NODISCARD bool maybeSave();
    NODISCARD std::unique_ptr<QFileDialog> createDefaultSaveDialog();

    struct NODISCARD ActionDisabler final
    {
    private:
        MainWindow &self;

    public:
        explicit ActionDisabler(MainWindow &self)
            : self(self)
        {
            self.disableActions(true);
        }
        ~ActionDisabler() { self.disableActions(false); }

    public:
        DELETE_CTORS_AND_ASSIGN_OPS(ActionDisabler);
    };
    void disableActions(bool value);

    struct NODISCARD CanvasHider final
    {
    private:
        MainWindow &self;

    public:
        explicit CanvasHider(MainWindow &self)
            : self(self)
        {
            self.hideCanvas(true);
        }
        ~CanvasHider() { self.hideCanvas(false); }

    public:
        DELETE_CTORS_AND_ASSIGN_OPS(CanvasHider);
    };
    void hideCanvas(bool hide);

    struct NODISCARD ProgressDialogLifetime final
    {
    private:
        MainWindow &self;

    public:
        explicit ProgressDialogLifetime(MainWindow &self)
            : self(self)
        {}
        ~ProgressDialogLifetime() { reset(); }

    public:
        DELETE_CTORS_AND_ASSIGN_OPS(ProgressDialogLifetime);

    public:
        void reset() { self.endProgressDialog(); }
    };
    NODISCARD ProgressDialogLifetime createNewProgressDialog(const QString &text);
    void endProgressDialog();
    NODISCARD MapCanvas *getCanvas() const;
    void mapChanged() const;
    void setCanvasMouseMode(CanvasMouseModeEnum mode);
    void execSelectionGroupMapAction(std::unique_ptr<AbstractAction> action);
};
