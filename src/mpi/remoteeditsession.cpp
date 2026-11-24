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

#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QString>

RemoteEditSession::RemoteEditSession(const RemoteInternalId internalId,
                                     const RemoteSessionId sessionId,
                                     RemoteEdit *const remoteEdit)
    : QObject(remoteEdit)
    , m_manager(remoteEdit)
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

RemoteEditInternalSession::RemoteEditInternalSession(const RemoteInternalId internalId,
                                                     const RemoteSessionId sessionId,
                                                     const QString &title,
                                                     const QString &body,
                                                     const MumeEditOptions &options,
                                                     RemoteEdit *const parent)
    : RemoteEditSession(internalId, sessionId, parent)
    , m_widget(
          new RemoteEditWidget(isEditSession(),
                               title,
                               body,
                               checked_dynamic_downcast<QWidget *>(parent->parent()), // MainWindow
                               options))
{
    const auto widget = m_widget.data();
    connect(widget, &RemoteEditWidget::sig_save, this, &RemoteEditSession::slot_onSave);
    connect(widget, &RemoteEditWidget::sig_cancel, this, &RemoteEditSession::slot_onCancel);
}

RemoteEditInternalSession::~RemoteEditInternalSession()
{
    qDebug() << "Destructed RemoteEditInternalSession" << getInternalId().asUint32()
             << getSessionId().asInt32();
    if (auto *const p = m_widget.get()) {
        p->close();
    }
}

#ifndef Q_OS_WASM
RemoteEditExternalSession::RemoteEditExternalSession(const RemoteInternalId internalId,
                                                     const RemoteSessionId sessionId,
                                                     const QString &title,
                                                     const QString &body,
                                                     const MumeEditOptions &options,
                                                     RemoteEdit *const parent)
    : RemoteEditSession(internalId, sessionId, parent)
    , m_process(new RemoteEditProcess(isEditSession(), title, body, this))
    , m_options(options)
{
    const auto proc = m_process.data();
    connect(proc,
            &RemoteEditProcess::sig_save,
            this,
            &RemoteEditExternalSession::slot_onExternalSave);
    connect(proc, &RemoteEditProcess::sig_cancel, this, &RemoteEditExternalSession::slot_onCancel);
}

void RemoteEditExternalSession::slot_onExternalSave(const QString &content)
{
    const QString issues = m_options.describeIssues(content);
    if (issues.isEmpty()) {
        slot_onSave(content);
        return;
    }

    // The external editor has already exited, so there is nothing to go back to:
    // the choices are to submit anyway, or to cancel and keep the text on the clipboard.
    const QString msg = issues + tr("\nCancelling copies your text to the clipboard.");

    auto *const dlg = new QMessageBox(QMessageBox::Warning,
                                      tr("MMapper Editor"),
                                      msg,
                                      QMessageBox::NoButton,
                                      qobject_cast<QWidget *>(m_manager->parent()));
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    auto *const submitAnywayBtn = dlg->addButton(tr("Submit Anyway"), QMessageBox::AcceptRole);
    dlg->addButton(QMessageBox::Cancel);
    connect(dlg, &QMessageBox::finished, this, [this, dlg, submitAnywayBtn, content](int) {
        if (dlg->clickedButton() == submitAnywayBtn) {
            slot_onSave(content);
        } else {
            QGuiApplication::clipboard()->setText(content);
            slot_onCancel();
        }
    });
    dlg->open();
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
