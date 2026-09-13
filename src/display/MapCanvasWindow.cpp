// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "MapCanvasWindow.h"

#include "../global/ConfigConsts-Computed.h"
#include "../global/utils.h"
#include "../global/window_utils.h"
#include "InfomarkSelection.h"
#include "connectionselection.h"

#include <QApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>

using NonOwningPointer = MapCanvasWindow *;
NODISCARD static NonOwningPointer &primaryMapCanvas()
{
    static NonOwningPointer primary = nullptr;
    return primary;
}

MapCanvasWindow::MapCanvasWindow(MapData &mapData,
                                 GameObserver &observer,
                                 PrespammedPath &prespammedPath,
                                 Mmapper2Group &groupManager,
                                 QWindow *const parent)
    : QOpenGLWindow{NoPartialUpdate, parent}
    , m_core{mapData, observer, prespammedPath, groupManager, static_cast<MapCanvasHost &>(*this)}
{
    // Forward the core's public signal surface as our own, so that callers
    // (MainWindow, MapWindow, ...) can connect to `MapCanvasWindow::sig_*`
    // without knowing MapCanvas exists.
    connect(&m_core, &MapCanvas::sig_onCenter, this, &MapCanvasWindow::sig_onCenter);
    connect(&m_core, &MapCanvas::sig_mapMove, this, &MapCanvasWindow::sig_mapMove);
    connect(&m_core, &MapCanvas::sig_setScrollBars, this, &MapCanvasWindow::sig_setScrollBars);
    connect(&m_core, &MapCanvas::sig_continuousScroll, this, &MapCanvasWindow::sig_continuousScroll);
    connect(&m_core, &MapCanvas::sig_log, this, &MapCanvasWindow::sig_log);
    connect(&m_core, &MapCanvas::sig_selectionChanged, this, &MapCanvasWindow::sig_selectionChanged);
    connect(&m_core, &MapCanvas::sig_newRoomSelection, this, &MapCanvasWindow::sig_newRoomSelection);
    connect(&m_core,
            &MapCanvas::sig_newConnectionSelection,
            this,
            &MapCanvasWindow::sig_newConnectionSelection);
    connect(&m_core,
            &MapCanvas::sig_newInfomarkSelection,
            this,
            &MapCanvasWindow::sig_newInfomarkSelection);
    connect(&m_core, &MapCanvas::sig_setCurrentRoom, this, &MapCanvasWindow::sig_setCurrentRoom);
    connect(&m_core, &MapCanvas::sig_zoomChanged, this, &MapCanvasWindow::sig_zoomChanged);
    connect(&m_core, &MapCanvas::sig_showTooltip, this, &MapCanvasWindow::sig_showTooltip);
    connect(&m_core,
            &MapCanvas::sig_customContextMenuRequested,
            this,
            &MapCanvasWindow::sig_customContextMenuRequested);
    connect(&m_core,
            &MapCanvas::sig_dismissContextMenu,
            this,
            &MapCanvasWindow::sig_dismissContextMenu);

    // The core stays QtWidgets-free; these two connections are where the
    // widget-specific UX (message boxes, hiding the window, aborting) lives.
    connect(&m_core,
            &MapCanvas::sig_glInitFailed,
            this,
            &MapCanvasWindow::handleGlInitFailed,
            Qt::DirectConnection);
    connect(&m_core,
            &MapCanvas::sig_glFatalError,
            this,
            &MapCanvasWindow::handleGlFatalError,
            Qt::DirectConnection);

    NonOwningPointer &pmc = primaryMapCanvas();
    if (pmc == nullptr) {
        pmc = this;
    }
}

MapCanvasWindow::~MapCanvasWindow()
{
    NonOwningPointer &pmc = primaryMapCanvas();
    if (pmc == this) {
        pmc = nullptr;
    }
}

MapCanvasWindow *MapCanvasWindow::getPrimary()
{
    return primaryMapCanvas();
}

void MapCanvasWindow::handleGlInitFailed(const QString &reason)
{
    qWarning() << "OpenGL initialization failed:" << reason;
    hide();
    doneCurrent();
    mmqt::showCritical(QApplication::activeWindow(),
                       "Unable to initialize OpenGL",
                       "Upgrade your video card drivers");
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows) {
        // Link to Microsoft OpenGL Compatibility Pack
        QDesktopServices::openUrl(
            QUrl(QStringLiteral("ms-windows-store://pdp/?productid=9nqpsl29bfff")));
    }
}

void MapCanvasWindow::handleGlFatalError(const QString &message)
{
    QMessageBox box;
    box.setWindowTitle("Fatal OpenGL error");
    box.setText(message);
    box.exec();

    std::abort();
}
