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

#include <QMessageBox>
#include <QMessageLogContext>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QString>

RemoteEditSession::RemoteEditSession(const RemoteInternalId internalId,
                                     const RemoteSessionId sessionId,
                                     QString title,
                                     QString draftKey,
                                     const bool draftView,
                                     RemoteEdit *const remoteEdit)
    : QObject(remoteEdit)
    , m_manager(remoteEdit)
    , m_title(std::move(title))
    , m_internalId(internalId)
    , m_sessionId(sessionId)
    , m_draftView(draftView)
    , m_draftKey(std::move(draftKey))
{
    assert(m_manager != nullptr);
}

void RemoteEditSession::save()
{
    m_manager->save(this);
}

void RemoteEditSession::cancel()
{
    m_manager->cancelEdit(this);
}

void RemoteEditSession::keepAsDraft()
{
    m_manager->keepDraft(this);
}

void RemoteEditSession::discard()
{
    m_manager->discardDraft(this);
}

QString RemoteEditSession::getFullDraftPath() const
{
    return m_manager->getDraftStore().filePath(m_draftKey);
}

RemoteEditInternalSession::RemoteEditInternalSession(const RemoteInternalId internalId,
                                                     const RemoteSessionId sessionId,
                                                     const QString &title,
                                                     const QString &body,
                                                     const MumeEditOptions &options,
                                                     const QString &draftKey,
                                                     const bool draftView,
                                                     RemoteEdit *const parent)
    : RemoteEditSession(internalId, sessionId, title, draftKey, draftView, parent)
    , m_widget(new RemoteEditWidget(isEditSession(), draftView, title, body, nullptr, options))
{
    const auto widget = m_widget.data();
    connect(widget, &RemoteEditWidget::sig_save, this, &RemoteEditSession::slot_onSave);
    connect(widget, &RemoteEditWidget::sig_cancel, this, &RemoteEditSession::slot_onCancel);
    connect(widget, &RemoteEditWidget::sig_discard, this, &RemoteEditSession::slot_onDiscard);
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
    }
}

RemoteEditInternalSession::~RemoteEditInternalSession()
{
    qDebug() << "Destructed RemoteEditInternalSession" << getInternalId().asUint32()
             << getSessionId().asInt32();
    if (auto *const p = m_widget.get()) {
        p->closeSilently();
    }
}

RemoteEditWidget *RemoteEditInternalSession::virt_getWidget() const
{
    return m_widget.data();
}

void RemoteEditInternalSession::virt_focus()
{
    if (auto *const p = m_widget.get()) {
        p->focus();
    }
}

void RemoteEditInternalSession::virt_offerDraft(const RemoteEditDraftInfo &draft)
{
    auto *const p = m_widget.get();
    if (p == nullptr) {
        return;
    }
    const QString key = draft.key;
    p->offerRecoveredDraft(
        draft.lastModified,
        [this, key]() {
            if (auto *const w = m_widget.get()) {
                w->replaceText(m_manager->readDraft(key));
            }
            m_manager->deleteDraft(key);
        },
        [this, key]() { m_manager->deleteDraft(key); });
}

void RemoteEditInternalSession::virt_onDisconnected()
{
    if (auto *const p = m_widget.get()) {
        p->showDisconnected();
    }
}

void RemoteEditInternalSession::virt_flushDraft()
{
    if ((m_debounceTimer != nullptr && m_debounceTimer->isActive())
        || (m_throttleTimer != nullptr && m_throttleTimer->isActive())) {
        slot_performAutoSave();
    }
}

void RemoteEditInternalSession::slot_onTextModified(const QString &content)
{
    m_content = content;

    // 2000ms debounce
    m_debounceTimer->start(2000);

    // 15000ms max throttle
    if (!m_throttleTimer->isActive()) {
        m_throttleTimer->start(15000);
    }
}

void RemoteEditInternalSession::slot_performAutoSave()
{
    if (m_draftKey.isEmpty()) {
        return;
    }

    if (m_manager->getDraftStore().save(m_draftKey, m_content)) {
        qDebug() << "Auto-save successful for" << m_draftKey;
        m_debounceTimer->stop();
        m_throttleTimer->stop();
    } else {
        qWarning() << "Auto-save failed for" << m_draftKey;
    }
}

#ifndef Q_OS_WASM
RemoteEditExternalSession::RemoteEditExternalSession(const RemoteInternalId internalId,
                                                     const RemoteSessionId sessionId,
                                                     const QString &title,
                                                     const QString &body,
                                                     const MumeEditOptions &options,
                                                     const QString &draftKey,
                                                     RemoteEdit *const parent)
    : RemoteEditSession(internalId, sessionId, title, draftKey, /*draftView=*/false, parent)
    , m_options(options)
{
    launchProcess(body);
}

void RemoteEditExternalSession::launchProcess(const QString &body)
{
    m_process = new RemoteEditProcess(isEditSession(), getTitle(), body, getFullDraftPath(), this);
    const auto proc = m_process.data();
    connect(proc,
            &RemoteEditProcess::sig_save,
            this,
            &RemoteEditExternalSession::slot_onExternalSave);
    connect(proc,
            &RemoteEditProcess::sig_cancel,
            this,
            &RemoteEditExternalSession::slot_onExternalCancel);
}

void RemoteEditExternalSession::slot_onExternalCancel()
{
    if (m_relaunched) {
        keepAsDraft();
    } else {
        cancel();
    }
}

void RemoteEditExternalSession::slot_onExternalSave(const QString &content)
{
    const QString issues = m_options.describeIssues(content);
    if (issues.isEmpty()) {
        slot_onSave(content);
        return;
    }

    // The external editor has already exited, so offer to start it again on the
    // same file; otherwise the text stays in the draft rather than being lost.
    auto *const dlg = new QMessageBox(QMessageBox::Warning,
                                      tr("MMapper Editor"),
                                      issues + tr("\nWhat would you like to do?"),
                                      QMessageBox::NoButton,
                                      qobject_cast<QWidget *>(m_manager->parent()));
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    auto *const editAgainBtn = dlg->addButton(tr("Edit Again"), QMessageBox::AcceptRole);
    auto *const submitAnywayBtn = dlg->addButton(tr("Submit Anyway"), QMessageBox::AcceptRole);
    auto *const keepBtn = dlg->addButton(tr("Keep as Draft"), QMessageBox::RejectRole);
    dlg->setDefaultButton(editAgainBtn);
    dlg->setEscapeButton(keepBtn);
    connect(dlg,
            &QMessageBox::finished,
            this,
            [this, dlg, editAgainBtn, submitAnywayBtn, content](int) {
                const auto *const clicked = dlg->clickedButton();
                if (clicked == submitAnywayBtn) {
                    slot_onSave(content);
                } else if (clicked == editAgainBtn) {
                    try {
                        if (m_process) {
                            m_process->deleteLater();
                        }
                        m_relaunched = true;
                        launchProcess(content);
                    } catch (const std::exception &e) {
                        qWarning() << "Unable to relaunch the external editor:" << e.what();
                        keepAsDraft();
                    }
                } else {
                    keepAsDraft();
                }
            });
    dlg->open();
}

RemoteEditExternalSession::~RemoteEditExternalSession()
{
    qDebug() << "Destructed RemoteEditExternalSession" << getInternalId().asUint32()
             << getSessionId().asInt32();
    // m_process is a QObject child of `this`, so QObject's own destructor
    // deletes it synchronously right after this body runs -- which in turn
    // synchronously terminates the child process (RemoteEditProcess::~RemoteEditProcess()
    // calls terminateSynchronously()). Deliberately not deleteLater(): that
    // defers to the event loop, which never runs again if this destructor
    // fires after QApplication::exec() has already returned (see
    // RemoteEdit::shutdown()).
}
#endif
