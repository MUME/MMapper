// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "remoteeditsession.h"

#include "../global/utils.h"
#include "remoteedit.h"
#include "remoteeditwidget.h"

#ifndef Q_OS_WASM
#include "remoteeditprocess.h"
#endif

#include <cassert>

#include <QMessageLogContext>
#include <QObject>
#include <QPointer>
#include <QString>

RemoteEditSession::RemoteEditSession(const RemoteInternalId internalId,
                                     const RemoteSessionId sessionId,
                                     QString title,
                                     RemoteEdit *const remoteEdit)
    : QObject(remoteEdit)
    , m_manager(remoteEdit)
    , m_title(std::move(title))
    , m_internalId(internalId)
    , m_sessionId(sessionId)
{
    assert(m_manager != nullptr);
}

void RemoteEditSession::save()
{
    m_manager->save(this);
}

void RemoteEditSession::cancel()
{
    m_manager->cancel(this);
}

void RemoteEditSession::raise()
{
    if (!isEditSession())
        return;

    // For recovered tasks or sessions whose editor window was closed,
    // opening it means showing the current draft content in a read-only widget.
    QFile file(getFullDraftPath());
    if (file.open(QFile::ReadOnly)) {
        QString content = QString::fromLatin1(file.readAll());
        file.close();

        auto *widget = new RemoteEditWidget(false, m_title, content, nullptr);
        widget->setAttribute(Qt::WA_DeleteOnClose);
        widget->show();
        widget->raise();
        widget->activateWindow();
    }
}

QString RemoteEditSession::getFullDraftPath() const
{
    if (m_draftFileName.isEmpty()) {
        return QString();
    }
    return QDir(RemoteEdit::getDraftDirectory()).absoluteFilePath(m_draftFileName);
}

RemoteEditInternalSession::RemoteEditInternalSession(const RemoteInternalId internalId,
                                                     const RemoteSessionId sessionId,
                                                     const QString &title,
                                                     const QString &body,
                                                     RemoteEdit *const parent)
    : RemoteEditSession(internalId, sessionId, title, parent)
    , m_widget(
          new RemoteEditWidget(isEditSession(),
                               title,
                               body,
                               checked_dynamic_downcast<QWidget *>(parent->parent()) // MainWindow
                               ))
{
    const auto widget = m_widget.data();
    connect(widget, &RemoteEditWidget::sig_save, this, &RemoteEditSession::slot_onSave);
    connect(widget, &RemoteEditWidget::sig_cancel, this, &RemoteEditSession::slot_onCancel);
    connect(widget,
            &RemoteEditWidget::sig_textModified,
            this,
            &RemoteEditInternalSession::slot_onTextModified);

    if (isEditSession()) {
        m_debounceTimer = new QTimer(this);
        m_debounceTimer->setSingleShot(true);
        connect(m_debounceTimer,
                &QTimer::timeout,
                this,
                &RemoteEditInternalSession::slot_performAutoSave);

        m_throttleTimer = new QTimer(this);
        m_throttleTimer->setSingleShot(true);
        connect(m_throttleTimer,
                &QTimer::timeout,
                this,
                &RemoteEditInternalSession::slot_performAutoSave);

        m_lastWriteTimer.start();
    }
}

RemoteEditInternalSession::~RemoteEditInternalSession()
{
    qDebug() << "Destructed RemoteEditInternalSession" << getInternalId().asUint32()
             << getSessionId().asInt32();
    if (auto *const p = m_widget.get()) {
        p->close();
    }
}

void RemoteEditInternalSession::slot_onTextModified(const QString &content)
{
    m_content = content;

    // FR-3.2: 2000ms debounce
    m_debounceTimer->start(2000);

    // FR-3.3: 15000ms max throttle
    if (!m_throttleTimer->isActive()) {
        m_throttleTimer->start(15000);
    }
}

void RemoteEditInternalSession::raise()
{
    if (m_widget) {
        m_widget->show();
        m_widget->raise();
        m_widget->activateWindow();
    } else {
        RemoteEditSession::raise();
    }
}

void RemoteEditInternalSession::slot_performAutoSave()
{
    if (m_draftFileName.isEmpty()) {
        return;
    }

    if (RemoteEdit::saveDraftAtomic(m_draftFileName, m_content)) {
        qDebug() << "Auto-save successful for" << m_draftFileName;
        m_lastWriteTimer.restart();
        m_debounceTimer->stop();
        m_throttleTimer->stop();
    } else {
        qWarning() << "Auto-save failed for" << m_draftFileName;
    }
}

#ifndef Q_OS_WASM
RemoteEditExternalSession::RemoteEditExternalSession(const RemoteInternalId internalId,
                                                     const RemoteSessionId sessionId,
                                                     const QString &title,
                                                     const QString &body,
                                                     RemoteEdit *const parent)
    : RemoteEditSession(internalId, sessionId, title, parent)
{
    m_process = new RemoteEditProcess(isEditSession(), title, body, getFullDraftPath(), this);
    const auto proc = m_process.data();
    connect(proc, &RemoteEditProcess::sig_save, this, &RemoteEditExternalSession::slot_onSave);
    connect(proc, &RemoteEditProcess::sig_cancel, this, &RemoteEditExternalSession::slot_onCancel);
}

RemoteEditExternalSession::~RemoteEditExternalSession()
{
    qDebug() << "Destructed RemoteEditExternalSession" << getInternalId().asUint32()
             << getSessionId().asInt32();
    if (auto *const p = m_process.get()) {
        p->deleteLater();
    }
}
#endif
