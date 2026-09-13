// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "RemoteEditPanel.h"

#include "remoteedit.h"
#include "remoteeditsession.h"
#include "remoteeditwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedLayout>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

RemoteEditPanel::RemoteEditPanel(RemoteEdit &remoteEdit, QWidget *const parent)
    : QWidget(parent)
    , m_remoteEdit(remoteEdit)
{
    auto *const outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_list = new QWidget(this);
    m_listLayout = new QVBoxLayout(m_list);
    m_listLayout->setContentsMargins(8, 4, 8, 4);
    m_listLayout->setSpacing(2);
    outer->addWidget(m_list, 0);

    auto *const stackHost = new QWidget(this);
    outer->addWidget(stackHost, 1);
    m_stack = new QStackedLayout(stackHost);
    m_stack->setContentsMargins(0, 0, 0, 0);

    m_placeholder = new QLabel(tr("No remote edits are open.\n"
                                  "MUME will open pages here when you edit or view text."),
                               stackHost);
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setWordWrap(true);
    m_placeholder->setEnabled(false);
    m_stack->addWidget(m_placeholder);

    m_tabs = new QTabWidget(stackHost);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    m_tabs->setElideMode(Qt::ElideRight);
    m_tabs->tabBar()->setExpanding(false);
    m_stack->addWidget(m_tabs);

    connect(m_tabs, &QTabWidget::tabCloseRequested, this, [this](const int index) {
        if (auto *const page = dynamic_cast<RemoteEditWidget *>(m_tabs->widget(index))) {
            page->requestClose();
        }
    });
    // QTabWidget drops a tab on its own when the page widget is destroyed;
    // this keeps the placeholder in sync with that.
    connect(m_tabs, &QTabWidget::currentChanged, this, [this]() { updatePlaceholder(); });

    connect(&m_remoteEdit, &RemoteEdit::sig_sessionsChanged, this, &RemoteEditPanel::syncTabs);
    connect(&m_remoteEdit, &RemoteEdit::sig_draftsChanged, this, &RemoteEditPanel::rebuildList);

    syncTabs();
}

void RemoteEditPanel::rebuildList()
{
    while (QLayoutItem *const item = m_listLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const auto addRow = [this](const QString &text, auto &&...buttons) {
        auto *const row = new QWidget(m_list);
        auto *const layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        auto *const label = new QLabel(text, row);
        label->setTextFormat(Qt::PlainText);
        layout->addWidget(label, 1);
        (
            [&](const auto &button) {
                auto *const b = new QPushButton(button.first, row);
                b->setAutoDefault(false);
                connect(b, &QPushButton::clicked, this, button.second);
                layout->addWidget(b, 0);
            }(buttons),
            ...);
        m_listLayout->addWidget(row);
    };

    for (const auto &[id, session] : m_remoteEdit.getSessions()) {
        if (session->getWidget() != nullptr) {
            continue;
        }
        const RemoteInternalId internalId = id;
        addRow(tr("%1 -- open in %2 editor%3")
                   .arg(session->getTitle(),
                        QString::fromLatin1(session->getEditorTypeName()).toLower(),
                        session->isConnected() ? QString() : tr(" (disconnected)")),
               std::make_pair(tr("Cancel"), [this, internalId]() {
                   const auto &sessions = m_remoteEdit.getSessions();
                   if (const auto it = sessions.find(internalId); it != sessions.end()) {
                       m_remoteEdit.cancelEdit(it->second.get());
                   }
               }));
    }

    for (const auto &draft : m_remoteEdit.pendingDrafts()) {
        addRow(tr("Unsent draft: %1 (%2)").arg(draft.title, draft.lastModified.toString()),
               std::make_pair(tr("View"), [this, draft]() { m_remoteEdit.viewDraft(draft); }),
               std::make_pair(tr("Discard"), [this, draft]() { m_remoteEdit.discardDraft(draft); }));
    }

    m_list->setVisible(m_listLayout->count() > 0);
}

RemoteEditPanel::~RemoteEditPanel() = default;

QSize RemoteEditPanel::sizeHint() const
{
    return QSize{640, 480};
}

void RemoteEditPanel::syncTabs()
{
    for (const auto &[id, session] : m_remoteEdit.getSessions()) {
        if (auto *const widget = session->getWidget(); widget != nullptr
                                                        && m_tabs->indexOf(widget) < 0) {
            addPage(widget);
        }
    }
    rebuildList();
    updatePlaceholder();
}

void RemoteEditPanel::addPage(RemoteEditWidget *const widget)
{
    const int index = m_tabs->addTab(widget, widget->getTitle());
    m_tabs->setTabToolTip(index,
                          widget->isDraftRecovery() ? tr("Recovered draft")
                          : widget->isEditSession() ? tr("Editing")
                                                    : tr("Viewing"));
    updateTabLabel(widget);

    connect(widget, &RemoteEditWidget::sig_focusRequested, this, [this, widget]() {
        m_tabs->setCurrentWidget(widget);
        emit sig_showRequested();
        widget->setFocus(Qt::OtherFocusReason);
    });
    connect(widget, &RemoteEditWidget::sig_textModified, this, [this, widget]() {
        updateTabLabel(widget);
    });

    m_tabs->setCurrentWidget(widget);
    emit sig_showRequested();
    widget->setFocus(Qt::OtherFocusReason);
}

void RemoteEditPanel::updateTabLabel(RemoteEditWidget *const widget)
{
    const int index = m_tabs->indexOf(widget);
    if (index < 0) {
        return;
    }
    QString label = widget->getTitle();
    if (widget->isDraftRecovery()) {
        label = tr("[draft] %1").arg(label);
    } else if (!widget->isEditSession()) {
        label = tr("[view] %1").arg(label);
    } else if (widget->isModified()) {
        label += "*";
    }
    m_tabs->setTabText(index, label);
}

void RemoteEditPanel::updatePlaceholder()
{
    m_stack->setCurrentWidget(m_tabs->count() == 0 ? static_cast<QWidget *>(m_placeholder)
                                                   : static_cast<QWidget *>(m_tabs));
}
