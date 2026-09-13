// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

// The compact layout for small (phone-sized) windows: everything MainWindow
// does differently while compact lives here, so that the policy
// (CompactLayout.h) and its realization can be read as one unit. The layout
// is chosen once at startup from the screen (see showEvent()) and is
// otherwise a manual Window > "Compact Layout" toggle; it does not follow
// window resizes, since flipping the whole window layout under the user is
// worse than a briefly cramped one.
// Elsewhere in MainWindow only three places consult m_compact: the menu bar
// and status bar toggles (slot_setShowMenuBar(), slot_setShowStatusBar())
// and the hidden menu bar's hover peek (eventFilter()).

#include "../client/ClientWidget.h"
#include "../client/displaywidget.h"
#include "../configuration/configuration.h"
#include "../display/mapwindow.h"
#include "../global/ConfigConsts-Computed.h"
#include "CompactLayout.h"
#include "mainwindow.h"

#include <array>

#include <QScreen>
#include <QScroller>
#include <QtWidgets>

QToolButton *MainWindow::createMenuButton()
{
    // "\u2630": the whole menu (see m_appMenu).
    QMenu *const menu = m_appMenu;
    auto *const menuButton = new QToolButton(statusBar());
    menuButton->setText(QStringLiteral("\u2630"));
    menuButton->setToolTip(tr("Menu"));
    menuButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    menuButton->setAutoRaise(true);
    QFont font = menuButton->font();
    font.setPointSizeF(font.pointSizeF() * CompactLayout::TAB_FONT_SCALE);
    menuButton->setFont(font);
    // Not setMenu()/InstantPopup: QToolButton shows its menu with
    // QMenu::exec(), a nested event loop, which is unavailable on wasm.
    connect(menuButton, &QToolButton::clicked, menu, [menuButton, menu]() {
        menu->popup(menuButton->mapToGlobal(QPoint(0, 0)));
    });
    return menuButton;
}

QWidget *MainWindow::createCompactActionBar()
{
    auto *const bar = new QWidget(statusBar());
    auto *const layout = new QHBoxLayout(bar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // The map operations touch has no wheel or keyboard for (zoom is a pinch).
    for (QAction *const action : {layerUpAct, layerDownAct, centerOnPlayerAct}) {
        auto *const button = new QToolButton(bar);
        button->setDefaultAction(action);
        button->setIconSize(QSize(24, 24));
        button->setAutoRaise(true);
        layout->addWidget(button);
    }
    return bar;
}

void MainWindow::setCompactLayout(const bool compact)
{
    if (!m_layoutRestored) {
        return; // see showEvent()
    }
    if (compact == m_compact) {
        return;
    }
    m_compact = compact;
    compactLayoutAct->setChecked(compact);
    applyCompactMenuBar(compact);

    if (compact) {
        m_expandedState = saveState();
        if (!m_compactState.isEmpty() && restoreState(m_compactState)) {
            applyCompactChrome(true);
            fitCompactWindowToScreen();
            return;
        }

        // No usable compact state yet: build the default one.
        buildDefaultCompactLayout();
        applyCompactChrome(true);
        fitCompactWindowToScreen();
    } else {
        m_compactState = saveState();
        if (!restoreState(m_expandedState)) {
            qWarning() << "Unable to restore the expanded window layout";
        }
        applyCompactChrome(false);
    }
}

void MainWindow::buildDefaultCompactLayout()
{
    for (QToolBar *const toolBar : findChildren<QToolBar *>()) {
        toolBar->hide();
    }

    // One tabbed group so a single panel shows at a time, below the
    // map; hidden docks stay hidden and appear as tabs when shown.
    const std::array<QDockWidget *, 8> docks{m_dockDialogClient,
                                             m_dockDialogGroup,
                                             m_dockDialogRoom,
                                             m_dockDialogDescription,
                                             m_dockDialogLog,
                                             m_dockDialogAdventure,
                                             m_dockDialogTimers,
                                             m_dockDialogAsync};
    QDockWidget *first = nullptr;
    for (QDockWidget *const dock : docks) {
        dock->setFloating(false);
        addDockWidget(Qt::BottomDockWidgetArea, dock);
        if (first == nullptr) {
            first = dock;
        } else {
            tabifyDockWidget(first, dock);
        }
    }

    // The client is what a phone user is here for: put its tab in front
    // and give the group its share of the height, leaving the map above.
    m_dockDialogClient->raise();
    resizeDocks({m_dockDialogClient},
                {CompactLayout::clientHeight(compactLayoutProbeSize())},
                Qt::Vertical);
}

void MainWindow::slot_resetWindowLayout()
{
    // Both layouts go back to their defaults: the one showing right now, and
    // the stored one for the other mode. The docks' visibility is part of
    // the state, so hidden panels come back too.
    if (m_compact) {
        buildDefaultCompactLayout();
        applyCompactChrome(true);
        m_expandedState = m_defaultExpandedState;
    } else {
        if (!restoreState(m_defaultExpandedState)) {
            qWarning() << "Unable to restore the default window layout";
        }
        m_compactState.clear();
        fitClientDockToTerminal();
    }
}

void MainWindow::fitClientDockToTerminal()
{
    // The default expanded layout gives the client its configured terminal
    // size (80x24 unless changed) when the window has room for that and
    // the map; QMainWindow's own default split does not consult it.
    const QSize terminal = deref(m_dockDialogClient).widget()->sizeHint();
    const QSize mapMin = deref(m_mapWindow).minimumSize();
    if (width() - terminal.width() >= mapMin.width()) {
        resizeDocks({m_dockDialogClient}, {terminal.width()}, Qt::Horizontal);
    }
    if (height() - terminal.height() >= mapMin.height()) {
        resizeDocks({m_dockDialogClient}, {terminal.height()}, Qt::Vertical);
    }
}

QSize MainWindow::compactLayoutProbeSize() const
{
    const QScreen *const scr = screen();
    return CompactLayout::probeSize(size(), scr != nullptr ? scr->availableSize() : QSize());
}

void MainWindow::fitCompactWindowToScreen()
{
    // Once the compact layout's smaller minimum size has taken effect (the
    // layout settles on the next event loop pass), pull a window that the
    // expanded layout had pushed past the screen's edge back onto it.
    QMetaObject::invokeMethod(
        this,
        [this]() {
            const QScreen *const scr = screen();
            if (scr == nullptr) {
                return;
            }
            const QRect avail = scr->availableGeometry();
            if (width() > avail.width() || height() > avail.height()) {
                setGeometry(avail);
            }
        },
        Qt::QueuedConnection);
}

void MainWindow::applyPanelScrollGesture(const bool compact)
{
    // Kinetic scrolling for every panel (client output, log, tables). Mouse
    // and wheel input are unaffected when expanded. While compact the
    // gesture is taken from the (synthesized) left button instead, since a
    // browser can deliver a finger as pointer/mouse events rather than
    // touch events; QScroller holds the press back until it is clear the
    // finger is not scrolling, so a held press still selects text.
    const auto gesture = compact ? QScroller::LeftMouseButtonGesture : QScroller::TouchGesture;
    for (QAbstractScrollArea *const area : findChildren<QAbstractScrollArea *>()) {
        if (qobject_cast<DisplayWidget *>(area) != nullptr) {
            continue; // scrolls touch itself; see DisplayWidget::mousePressEvent()
        }
        QScroller::grabGesture(area->viewport(), gesture);
    }
}

void MainWindow::applyCompactChrome(const bool compact)
{
    deref(m_clientWidget).setCompactLayout(compact);
    applyPanelScrollGesture(compact);
    deref(m_mapWindow).setScrollBarsSuppressed(compact);
    // The action bar lives in the status bar, which therefore has to be
    // shown while compact regardless of the setting. The path machine's
    // state label and the size grip (touch cannot use it) make room for
    // it on a phone-width bar.
    m_compactActionBar->setVisible(compact);
    m_pathMachineStatus->setVisible(!compact);
    statusBar()->setSizeGripEnabled(!compact);
    statusBar()->setVisible(compact || getConfig().general.showStatusBar);

    // Dock title bars: one tab strip already names the visible panel, and
    // the float/close buttons are not touch targets, so the title bars are
    // replaced by empty widgets while compact (which also pins the docks).
    for (QDockWidget *const dock : findChildren<QDockWidget *>()) {
        QWidget *const old = dock->titleBarWidget();
        dock->setTitleBarWidget(compact ? new QWidget(dock) : nullptr);
        delete old;
    }

    // The tab strip QMainWindow creates for tabified docks: stretch the
    // tabs across the width and use a larger font so they are finger-sized.
    for (QTabBar *const tabBar : findChildren<QTabBar *>()) {
        if (qobject_cast<QMainWindow *>(tabBar->parentWidget()) == nullptr) {
            continue; // a widget's own tab bar, not the dock strip
        }
        tabBar->setExpanding(compact);
        QFont font = this->font();
        if (compact) {
            font.setPointSizeF(font.pointSizeF() * CompactLayout::TAB_FONT_SCALE);
        }
        tabBar->setFont(font);
    }
}

void MainWindow::applyCompactMenuBar(const bool compact)
{
    // While compact the menu bar's row is given back to the content; the
    // menus are reached through the status bar's "\u2630" button instead
    // (see m_appMenu). The Mac's native menu bar lives outside the
    // window, so it costs no space and is left alone (see
    // slot_setShowMenuBar()).
    if constexpr (CURRENT_PLATFORM != PlatformEnum::Mac) {
        menuBar()->setVisible(!compact && getConfig().general.showMenuBar);
        m_menuButton->setVisible(compact);
    }
}
