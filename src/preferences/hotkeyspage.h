#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../global/macros.h"

#include <QWidget>

class QKeySequenceEdit;
class QPushButton;
class QScrollArea;

class NODISCARD_QOBJECT HotkeysPage final : public QWidget
{
    Q_OBJECT

public:
    explicit HotkeysPage(QWidget *parent);
    ~HotkeysPage() final = default;

private:
    void setupUI();
    void connectSignals();
    void loadSettings();
    void saveSettings();
    void resetToDefaults();

    QWidget *createHotkeyRow(const QString &label, QKeySequenceEdit **editor, QPushButton **clearBtn);
    void clearShortcut(QKeySequenceEdit *editor);

    // File operations
    QKeySequenceEdit *m_fileOpen = nullptr;
    QKeySequenceEdit *m_fileSave = nullptr;
    QKeySequenceEdit *m_fileReload = nullptr;
    QKeySequenceEdit *m_fileQuit = nullptr;

    // Edit operations
    QKeySequenceEdit *m_editUndo = nullptr;
    QKeySequenceEdit *m_editRedo = nullptr;
    QKeySequenceEdit *m_editPreferences = nullptr;
    QKeySequenceEdit *m_editPreferencesAlt = nullptr;
    QKeySequenceEdit *m_editFindRooms = nullptr;
    QKeySequenceEdit *m_editRoom = nullptr;

    // View operations
    QKeySequenceEdit *m_viewZoomIn = nullptr;
    QKeySequenceEdit *m_viewZoomOut = nullptr;
    QKeySequenceEdit *m_viewZoomReset = nullptr;
    QKeySequenceEdit *m_viewLayerUp = nullptr;
    QKeySequenceEdit *m_viewLayerDown = nullptr;
    QKeySequenceEdit *m_viewLayerReset = nullptr;

    // View toggles
    QKeySequenceEdit *m_viewRadialTransparency = nullptr;
    QKeySequenceEdit *m_viewStatusBar = nullptr;
    QKeySequenceEdit *m_viewScrollBars = nullptr;
    QKeySequenceEdit *m_viewMenuBar = nullptr;
    QKeySequenceEdit *m_viewAlwaysOnTop = nullptr;

    // Side panels
    QKeySequenceEdit *m_panelLog = nullptr;
    QKeySequenceEdit *m_panelClient = nullptr;
    QKeySequenceEdit *m_panelGroup = nullptr;
    QKeySequenceEdit *m_panelRoom = nullptr;
    QKeySequenceEdit *m_panelAdventure = nullptr;
    QKeySequenceEdit *m_panelComms = nullptr;
    QKeySequenceEdit *m_panelDescription = nullptr;

    // Mouse modes
    QKeySequenceEdit *m_modeMoveMap = nullptr;
    QKeySequenceEdit *m_modeRaypick = nullptr;
    QKeySequenceEdit *m_modeSelectRooms = nullptr;
    QKeySequenceEdit *m_modeSelectMarkers = nullptr;
    QKeySequenceEdit *m_modeSelectConnection = nullptr;
    QKeySequenceEdit *m_modeCreateMarker = nullptr;
    QKeySequenceEdit *m_modeCreateRoom = nullptr;
    QKeySequenceEdit *m_modeCreateConnection = nullptr;
    QKeySequenceEdit *m_modeCreateOnewayConnection = nullptr;

    // Room operations
    QKeySequenceEdit *m_roomCreate = nullptr;
    QKeySequenceEdit *m_roomMoveUp = nullptr;
    QKeySequenceEdit *m_roomMoveDown = nullptr;
    QKeySequenceEdit *m_roomMergeUp = nullptr;
    QKeySequenceEdit *m_roomMergeDown = nullptr;
    QKeySequenceEdit *m_roomDelete = nullptr;
    QKeySequenceEdit *m_roomConnectNeighbors = nullptr;
    QKeySequenceEdit *m_roomMoveToSelected = nullptr;
    QKeySequenceEdit *m_roomUpdateSelected = nullptr;

    QPushButton *m_resetButton = nullptr;

signals:
    void sig_hotkeysChanged();

public slots:
    void slot_loadConfig();
};
