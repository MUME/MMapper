#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"

#include <QWidget>

class QLabel;
class QStackedLayout;
class QTabWidget;
class QVBoxLayout;
class RemoteEdit;
class RemoteEditWidget;

/// Hosts every open internal editor/viewer as a tab, and above the tabs
/// lists what has no page of its own: edits open in an external editor and
/// unsent drafts on disk. Tabs track RemoteEdit::getSessions(): a session's
/// page is added when the session appears and vanishes when the session
/// deletes it.
class NODISCARD_QOBJECT RemoteEditPanel final : public QWidget
{
    Q_OBJECT

private:
    RemoteEdit &m_remoteEdit;
    QWidget *m_list = nullptr;
    QVBoxLayout *m_listLayout = nullptr;
    QStackedLayout *m_stack = nullptr;
    QLabel *m_placeholder = nullptr;
    QTabWidget *m_tabs = nullptr;

public:
    explicit RemoteEditPanel(RemoteEdit &remoteEdit, QWidget *parent);
    ~RemoteEditPanel() final;

    NODISCARD QSize sizeHint() const override;

signals:
    /// A page was added or asked to be focused; the hosting dock should show itself.
    void sig_showRequested();

private:
    void syncTabs();
    void rebuildList();
    void addPage(RemoteEditWidget *widget);
    void updateTabLabel(RemoteEditWidget *widget);
    void updatePlaceholder();
};
