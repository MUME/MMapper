// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "hotkeyspage.h"

#include "../configuration/configuration.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QKeySequence>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

HotkeysPage::HotkeysPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    connectSignals();
    loadSettings();
}

QWidget *HotkeysPage::createHotkeyRow(const QString &label,
                                       QKeySequenceEdit **editor,
                                       QPushButton **clearBtn)
{
    auto *widget = new QWidget(this);
    auto *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *labelWidget = new QLabel(label, widget);
    labelWidget->setMinimumWidth(250);
    layout->addWidget(labelWidget);

    *editor = new QKeySequenceEdit(widget);
    (*editor)->setMaximumSequenceLength(1);
    layout->addWidget(*editor);

    *clearBtn = new QPushButton("Clear", widget);
    (*clearBtn)->setMaximumWidth(60);
    layout->addWidget(*clearBtn);

    return widget;
}

void HotkeysPage::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);

    // File Operations Group
    auto *fileGroup = new QGroupBox(tr("File Operations"));
    auto *fileLayout = new QVBoxLayout(fileGroup);
    QPushButton *clearBtn = nullptr;
    fileLayout->addWidget(createHotkeyRow("Open", &m_fileOpen, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_fileOpen); });
    fileLayout->addWidget(createHotkeyRow("Save", &m_fileSave, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_fileSave); });
    fileLayout->addWidget(createHotkeyRow("Reload", &m_fileReload, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_fileReload); });
    fileLayout->addWidget(createHotkeyRow("Quit", &m_fileQuit, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_fileQuit); });
    mainLayout->addWidget(fileGroup);

    // Edit Operations Group
    auto *editGroup = new QGroupBox(tr("Edit Operations"));
    auto *editLayout = new QVBoxLayout(editGroup);
    editLayout->addWidget(createHotkeyRow("Undo", &m_editUndo, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_editUndo); });
    editLayout->addWidget(createHotkeyRow("Redo", &m_editRedo, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_editRedo); });
    editLayout->addWidget(createHotkeyRow("Preferences", &m_editPreferences, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_editPreferences); });
    editLayout->addWidget(createHotkeyRow("Preferences (Alt)", &m_editPreferencesAlt, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_editPreferencesAlt); });
    editLayout->addWidget(createHotkeyRow("Find Rooms", &m_editFindRooms, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_editFindRooms); });
    editLayout->addWidget(createHotkeyRow("Edit Room", &m_editRoom, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_editRoom); });
    mainLayout->addWidget(editGroup);

    // View Operations Group
    auto *viewGroup = new QGroupBox(tr("View Operations"));
    auto *viewLayout = new QVBoxLayout(viewGroup);
    viewLayout->addWidget(createHotkeyRow("Zoom In", &m_viewZoomIn, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_viewZoomIn); });
    viewLayout->addWidget(createHotkeyRow("Zoom Out", &m_viewZoomOut, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_viewZoomOut); });
    viewLayout->addWidget(createHotkeyRow("Zoom Reset", &m_viewZoomReset, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_viewZoomReset); });
    viewLayout->addWidget(createHotkeyRow("Layer Up", &m_viewLayerUp, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_viewLayerUp); });
    viewLayout->addWidget(createHotkeyRow("Layer Down", &m_viewLayerDown, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_viewLayerDown); });
    viewLayout->addWidget(createHotkeyRow("Layer Reset", &m_viewLayerReset, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_viewLayerReset); });
    mainLayout->addWidget(viewGroup);

    // View Toggles Group
    auto *togglesGroup = new QGroupBox(tr("View Toggles"));
    auto *togglesLayout = new QVBoxLayout(togglesGroup);
    togglesLayout->addWidget(createHotkeyRow("Radial Transparency", &m_viewRadialTransparency, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_viewRadialTransparency); });
    togglesLayout->addWidget(createHotkeyRow("Status Bar", &m_viewStatusBar, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_viewStatusBar); });
    togglesLayout->addWidget(createHotkeyRow("Scroll Bars", &m_viewScrollBars, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_viewScrollBars); });
    togglesLayout->addWidget(createHotkeyRow("Menu Bar", &m_viewMenuBar, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_viewMenuBar); });
    togglesLayout->addWidget(createHotkeyRow("Always on Top", &m_viewAlwaysOnTop, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_viewAlwaysOnTop); });
    mainLayout->addWidget(togglesGroup);

    // Side Panels Group
    auto *panelsGroup = new QGroupBox(tr("Side Panels"));
    auto *panelsLayout = new QVBoxLayout(panelsGroup);
    panelsLayout->addWidget(createHotkeyRow("Log Panel", &m_panelLog, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_panelLog); });
    panelsLayout->addWidget(createHotkeyRow("Client Panel", &m_panelClient, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_panelClient); });
    panelsLayout->addWidget(createHotkeyRow("Group Panel", &m_panelGroup, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_panelGroup); });
    panelsLayout->addWidget(createHotkeyRow("Room Panel", &m_panelRoom, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_panelRoom); });
    panelsLayout->addWidget(createHotkeyRow("Adventure Panel", &m_panelAdventure, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_panelAdventure); });
    panelsLayout->addWidget(createHotkeyRow("Communications Panel", &m_panelComms, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_panelComms); });
    panelsLayout->addWidget(createHotkeyRow("Description Panel", &m_panelDescription, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_panelDescription); });
    mainLayout->addWidget(panelsGroup);

    // Mouse Modes Group
    auto *modesGroup = new QGroupBox(tr("Mouse Modes"));
    auto *modesLayout = new QVBoxLayout(modesGroup);
    modesLayout->addWidget(createHotkeyRow("Move Map", &m_modeMoveMap, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_modeMoveMap); });
    modesLayout->addWidget(createHotkeyRow("Ray-pick Rooms", &m_modeRaypick, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_modeRaypick); });
    modesLayout->addWidget(createHotkeyRow("Select Rooms", &m_modeSelectRooms, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_modeSelectRooms); });
    modesLayout->addWidget(createHotkeyRow("Select Markers", &m_modeSelectMarkers, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_modeSelectMarkers); });
    modesLayout->addWidget(createHotkeyRow("Select Connection", &m_modeSelectConnection, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_modeSelectConnection); });
    modesLayout->addWidget(createHotkeyRow("Create Marker", &m_modeCreateMarker, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_modeCreateMarker); });
    modesLayout->addWidget(createHotkeyRow("Create Room", &m_modeCreateRoom, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_modeCreateRoom); });
    modesLayout->addWidget(createHotkeyRow("Create Connection", &m_modeCreateConnection, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_modeCreateConnection); });
    modesLayout->addWidget(createHotkeyRow("Create One-way Connection", &m_modeCreateOnewayConnection, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_modeCreateOnewayConnection); });
    mainLayout->addWidget(modesGroup);

    // Room Operations Group
    auto *roomGroup = new QGroupBox(tr("Room Operations"));
    auto *roomLayout = new QVBoxLayout(roomGroup);
    roomLayout->addWidget(createHotkeyRow("Create New Room", &m_roomCreate, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_roomCreate); });
    roomLayout->addWidget(createHotkeyRow("Move Up Selected Rooms", &m_roomMoveUp, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_roomMoveUp); });
    roomLayout->addWidget(createHotkeyRow("Move Down Selected Rooms", &m_roomMoveDown, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_roomMoveDown); });
    roomLayout->addWidget(createHotkeyRow("Merge Up Selected Rooms", &m_roomMergeUp, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_roomMergeUp); });
    roomLayout->addWidget(createHotkeyRow("Merge Down Selected Rooms", &m_roomMergeDown, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_roomMergeDown); });
    roomLayout->addWidget(createHotkeyRow("Delete Selected Rooms", &m_roomDelete, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_roomDelete); });
    roomLayout->addWidget(createHotkeyRow("Connect Rooms to Neighbors", &m_roomConnectNeighbors, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_roomConnectNeighbors); });
    roomLayout->addWidget(createHotkeyRow("Move to Selected Room", &m_roomMoveToSelected, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_roomMoveToSelected); });
    roomLayout->addWidget(createHotkeyRow("Update Selected Room", &m_roomUpdateSelected, &clearBtn));
    connect(clearBtn, &QPushButton::clicked, [this]() { clearShortcut(m_roomUpdateSelected); });
    mainLayout->addWidget(roomGroup);

    mainLayout->addStretch();

    // Reset to Defaults button at bottom
    auto *buttonLayout = new QHBoxLayout();
    m_resetButton = new QPushButton(tr("Reset to Defaults"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_resetButton);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void HotkeysPage::connectSignals()
{
    // Connect all QKeySequenceEdit editingFinished signals to save and emit signal
    auto connectEditor = [this](QKeySequenceEdit *editor) {
        connect(editor, &QKeySequenceEdit::editingFinished, this, [this]() {
            saveSettings();
            emit sig_hotkeysChanged();
        });
    };

    // File operations
    connectEditor(m_fileOpen);
    connectEditor(m_fileSave);
    connectEditor(m_fileReload);
    connectEditor(m_fileQuit);

    // Edit operations
    connectEditor(m_editUndo);
    connectEditor(m_editRedo);
    connectEditor(m_editPreferences);
    connectEditor(m_editPreferencesAlt);
    connectEditor(m_editFindRooms);
    connectEditor(m_editRoom);

    // View operations
    connectEditor(m_viewZoomIn);
    connectEditor(m_viewZoomOut);
    connectEditor(m_viewZoomReset);
    connectEditor(m_viewLayerUp);
    connectEditor(m_viewLayerDown);
    connectEditor(m_viewLayerReset);

    // View toggles
    connectEditor(m_viewRadialTransparency);
    connectEditor(m_viewStatusBar);
    connectEditor(m_viewScrollBars);
    connectEditor(m_viewMenuBar);
    connectEditor(m_viewAlwaysOnTop);

    // Side panels
    connectEditor(m_panelLog);
    connectEditor(m_panelClient);
    connectEditor(m_panelGroup);
    connectEditor(m_panelRoom);
    connectEditor(m_panelAdventure);
    connectEditor(m_panelComms);
    connectEditor(m_panelDescription);

    // Mouse modes
    connectEditor(m_modeMoveMap);
    connectEditor(m_modeRaypick);
    connectEditor(m_modeSelectRooms);
    connectEditor(m_modeSelectMarkers);
    connectEditor(m_modeSelectConnection);
    connectEditor(m_modeCreateMarker);
    connectEditor(m_modeCreateRoom);
    connectEditor(m_modeCreateConnection);
    connectEditor(m_modeCreateOnewayConnection);

    // Room operations
    connectEditor(m_roomCreate);
    connectEditor(m_roomMoveUp);
    connectEditor(m_roomMoveDown);
    connectEditor(m_roomMergeUp);
    connectEditor(m_roomMergeDown);
    connectEditor(m_roomDelete);
    connectEditor(m_roomConnectNeighbors);
    connectEditor(m_roomMoveToSelected);
    connectEditor(m_roomUpdateSelected);

    // Reset button
    connect(m_resetButton, &QPushButton::clicked, this, &HotkeysPage::resetToDefaults);
}

void HotkeysPage::loadSettings()
{
    const auto &hotkeys = getConfig().hotkeys;

    m_fileOpen->setKeySequence(QKeySequence(hotkeys.fileOpen.get()));
    m_fileSave->setKeySequence(QKeySequence(hotkeys.fileSave.get()));
    m_fileReload->setKeySequence(QKeySequence(hotkeys.fileReload.get()));
    m_fileQuit->setKeySequence(QKeySequence(hotkeys.fileQuit.get()));

    m_editUndo->setKeySequence(QKeySequence(hotkeys.editUndo.get()));
    m_editRedo->setKeySequence(QKeySequence(hotkeys.editRedo.get()));
    m_editPreferences->setKeySequence(QKeySequence(hotkeys.editPreferences.get()));
    m_editPreferencesAlt->setKeySequence(QKeySequence(hotkeys.editPreferencesAlt.get()));
    m_editFindRooms->setKeySequence(QKeySequence(hotkeys.editFindRooms.get()));
    m_editRoom->setKeySequence(QKeySequence(hotkeys.editRoom.get()));

    m_viewZoomIn->setKeySequence(QKeySequence(hotkeys.viewZoomIn.get()));
    m_viewZoomOut->setKeySequence(QKeySequence(hotkeys.viewZoomOut.get()));
    m_viewZoomReset->setKeySequence(QKeySequence(hotkeys.viewZoomReset.get()));
    m_viewLayerUp->setKeySequence(QKeySequence(hotkeys.viewLayerUp.get()));
    m_viewLayerDown->setKeySequence(QKeySequence(hotkeys.viewLayerDown.get()));
    m_viewLayerReset->setKeySequence(QKeySequence(hotkeys.viewLayerReset.get()));

    m_viewRadialTransparency->setKeySequence(QKeySequence(hotkeys.viewRadialTransparency.get()));
    m_viewStatusBar->setKeySequence(QKeySequence(hotkeys.viewStatusBar.get()));
    m_viewScrollBars->setKeySequence(QKeySequence(hotkeys.viewScrollBars.get()));
    m_viewMenuBar->setKeySequence(QKeySequence(hotkeys.viewMenuBar.get()));
    m_viewAlwaysOnTop->setKeySequence(QKeySequence(hotkeys.viewAlwaysOnTop.get()));

    m_panelLog->setKeySequence(QKeySequence(hotkeys.panelLog.get()));
    m_panelClient->setKeySequence(QKeySequence(hotkeys.panelClient.get()));
    m_panelGroup->setKeySequence(QKeySequence(hotkeys.panelGroup.get()));
    m_panelRoom->setKeySequence(QKeySequence(hotkeys.panelRoom.get()));
    m_panelAdventure->setKeySequence(QKeySequence(hotkeys.panelAdventure.get()));
    m_panelComms->setKeySequence(QKeySequence(hotkeys.panelComms.get()));
    m_panelDescription->setKeySequence(QKeySequence(hotkeys.panelDescription.get()));

    m_modeMoveMap->setKeySequence(QKeySequence(hotkeys.modeMoveMap.get()));
    m_modeRaypick->setKeySequence(QKeySequence(hotkeys.modeRaypick.get()));
    m_modeSelectRooms->setKeySequence(QKeySequence(hotkeys.modeSelectRooms.get()));
    m_modeSelectMarkers->setKeySequence(QKeySequence(hotkeys.modeSelectMarkers.get()));
    m_modeSelectConnection->setKeySequence(QKeySequence(hotkeys.modeSelectConnection.get()));
    m_modeCreateMarker->setKeySequence(QKeySequence(hotkeys.modeCreateMarker.get()));
    m_modeCreateRoom->setKeySequence(QKeySequence(hotkeys.modeCreateRoom.get()));
    m_modeCreateConnection->setKeySequence(QKeySequence(hotkeys.modeCreateConnection.get()));
    m_modeCreateOnewayConnection->setKeySequence(QKeySequence(hotkeys.modeCreateOnewayConnection.get()));

    m_roomCreate->setKeySequence(QKeySequence(hotkeys.roomCreate.get()));
    m_roomMoveUp->setKeySequence(QKeySequence(hotkeys.roomMoveUp.get()));
    m_roomMoveDown->setKeySequence(QKeySequence(hotkeys.roomMoveDown.get()));
    m_roomMergeUp->setKeySequence(QKeySequence(hotkeys.roomMergeUp.get()));
    m_roomMergeDown->setKeySequence(QKeySequence(hotkeys.roomMergeDown.get()));
    m_roomDelete->setKeySequence(QKeySequence(hotkeys.roomDelete.get()));
    m_roomConnectNeighbors->setKeySequence(QKeySequence(hotkeys.roomConnectNeighbors.get()));
    m_roomMoveToSelected->setKeySequence(QKeySequence(hotkeys.roomMoveToSelected.get()));
    m_roomUpdateSelected->setKeySequence(QKeySequence(hotkeys.roomUpdateSelected.get()));
}

void HotkeysPage::saveSettings()
{
    auto &hotkeys = setConfig().hotkeys;

    hotkeys.fileOpen.set(m_fileOpen->keySequence().toString());
    hotkeys.fileSave.set(m_fileSave->keySequence().toString());
    hotkeys.fileReload.set(m_fileReload->keySequence().toString());
    hotkeys.fileQuit.set(m_fileQuit->keySequence().toString());

    hotkeys.editUndo.set(m_editUndo->keySequence().toString());
    hotkeys.editRedo.set(m_editRedo->keySequence().toString());
    hotkeys.editPreferences.set(m_editPreferences->keySequence().toString());
    hotkeys.editPreferencesAlt.set(m_editPreferencesAlt->keySequence().toString());
    hotkeys.editFindRooms.set(m_editFindRooms->keySequence().toString());
    hotkeys.editRoom.set(m_editRoom->keySequence().toString());

    hotkeys.viewZoomIn.set(m_viewZoomIn->keySequence().toString());
    hotkeys.viewZoomOut.set(m_viewZoomOut->keySequence().toString());
    hotkeys.viewZoomReset.set(m_viewZoomReset->keySequence().toString());
    hotkeys.viewLayerUp.set(m_viewLayerUp->keySequence().toString());
    hotkeys.viewLayerDown.set(m_viewLayerDown->keySequence().toString());
    hotkeys.viewLayerReset.set(m_viewLayerReset->keySequence().toString());

    hotkeys.viewRadialTransparency.set(m_viewRadialTransparency->keySequence().toString());
    hotkeys.viewStatusBar.set(m_viewStatusBar->keySequence().toString());
    hotkeys.viewScrollBars.set(m_viewScrollBars->keySequence().toString());
    hotkeys.viewMenuBar.set(m_viewMenuBar->keySequence().toString());
    hotkeys.viewAlwaysOnTop.set(m_viewAlwaysOnTop->keySequence().toString());

    hotkeys.panelLog.set(m_panelLog->keySequence().toString());
    hotkeys.panelClient.set(m_panelClient->keySequence().toString());
    hotkeys.panelGroup.set(m_panelGroup->keySequence().toString());
    hotkeys.panelRoom.set(m_panelRoom->keySequence().toString());
    hotkeys.panelAdventure.set(m_panelAdventure->keySequence().toString());
    hotkeys.panelComms.set(m_panelComms->keySequence().toString());
    hotkeys.panelDescription.set(m_panelDescription->keySequence().toString());

    hotkeys.modeMoveMap.set(m_modeMoveMap->keySequence().toString());
    hotkeys.modeRaypick.set(m_modeRaypick->keySequence().toString());
    hotkeys.modeSelectRooms.set(m_modeSelectRooms->keySequence().toString());
    hotkeys.modeSelectMarkers.set(m_modeSelectMarkers->keySequence().toString());
    hotkeys.modeSelectConnection.set(m_modeSelectConnection->keySequence().toString());
    hotkeys.modeCreateMarker.set(m_modeCreateMarker->keySequence().toString());
    hotkeys.modeCreateRoom.set(m_modeCreateRoom->keySequence().toString());
    hotkeys.modeCreateConnection.set(m_modeCreateConnection->keySequence().toString());
    hotkeys.modeCreateOnewayConnection.set(m_modeCreateOnewayConnection->keySequence().toString());

    hotkeys.roomCreate.set(m_roomCreate->keySequence().toString());
    hotkeys.roomMoveUp.set(m_roomMoveUp->keySequence().toString());
    hotkeys.roomMoveDown.set(m_roomMoveDown->keySequence().toString());
    hotkeys.roomMergeUp.set(m_roomMergeUp->keySequence().toString());
    hotkeys.roomMergeDown.set(m_roomMergeDown->keySequence().toString());
    hotkeys.roomDelete.set(m_roomDelete->keySequence().toString());
    hotkeys.roomConnectNeighbors.set(m_roomConnectNeighbors->keySequence().toString());
    hotkeys.roomMoveToSelected.set(m_roomMoveToSelected->keySequence().toString());
    hotkeys.roomUpdateSelected.set(m_roomUpdateSelected->keySequence().toString());
}

void HotkeysPage::clearShortcut(QKeySequenceEdit *editor)
{
    editor->clear();
    saveSettings();
    emit sig_hotkeysChanged();
}

void HotkeysPage::resetToDefaults()
{
    // Reset to default values defined in configuration.h
    auto &hotkeys = setConfig().hotkeys;

    // File operations
    hotkeys.fileOpen.set("Ctrl+O");
    hotkeys.fileSave.set("Ctrl+S");
    hotkeys.fileReload.set("Ctrl+R");
    hotkeys.fileQuit.set("Ctrl+Q");

    // Edit operations
    hotkeys.editUndo.set("Ctrl+Z");
    hotkeys.editRedo.set("Ctrl+Y");
    hotkeys.editPreferences.set("Ctrl+P");
    hotkeys.editPreferencesAlt.set("Esc");
    hotkeys.editFindRooms.set("Ctrl+F");
    hotkeys.editRoom.set("Ctrl+E");

    // View operations (most blank, except Zoom Reset)
    hotkeys.viewZoomIn.set("");
    hotkeys.viewZoomOut.set("");
    hotkeys.viewZoomReset.set("Ctrl+0");
    hotkeys.viewLayerUp.set("");
    hotkeys.viewLayerDown.set("");
    hotkeys.viewLayerReset.set("");

    // View toggles (all blank by default)
    hotkeys.viewRadialTransparency.set("");
    hotkeys.viewStatusBar.set("");
    hotkeys.viewScrollBars.set("");
    hotkeys.viewMenuBar.set("");
    hotkeys.viewAlwaysOnTop.set("");

    // Side panels (only Log has default)
    hotkeys.panelLog.set("Ctrl+L");
    hotkeys.panelClient.set("");
    hotkeys.panelGroup.set("");
    hotkeys.panelRoom.set("");
    hotkeys.panelAdventure.set("");
    hotkeys.panelComms.set("");
    hotkeys.panelDescription.set("");

    // Mouse modes (all blank by default)
    hotkeys.modeMoveMap.set("");
    hotkeys.modeRaypick.set("");
    hotkeys.modeSelectRooms.set("");
    hotkeys.modeSelectMarkers.set("");
    hotkeys.modeSelectConnection.set("");
    hotkeys.modeCreateMarker.set("");
    hotkeys.modeCreateRoom.set("");
    hotkeys.modeCreateConnection.set("");
    hotkeys.modeCreateOnewayConnection.set("");

    // Room operations (only Delete has default)
    hotkeys.roomCreate.set("");
    hotkeys.roomMoveUp.set("");
    hotkeys.roomMoveDown.set("");
    hotkeys.roomMergeUp.set("");
    hotkeys.roomMergeDown.set("");
    hotkeys.roomDelete.set("Del");
    hotkeys.roomConnectNeighbors.set("");
    hotkeys.roomMoveToSelected.set("");
    hotkeys.roomUpdateSelected.set("");

    loadSettings();
    emit sig_hotkeysChanged();
}

void HotkeysPage::slot_loadConfig()
{
    loadSettings();
}
