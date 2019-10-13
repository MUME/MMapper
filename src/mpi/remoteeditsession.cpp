// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "remoteeditsession.h"

#include <cassert>
#include <QMessageLogContext>
#include <QObject>
#include <QScopedPointer>
#include <QString>

#include "../global/utils.h"
#include "remoteedit.h"
#include "remoteeditprocess.h"
#include "remoteeditwidget.h"

RemoteEditSession::RemoteEditSession(uint sessionId, int key, RemoteEdit *const remoteEdit)
    : QObject(remoteEdit)
    , m_sessionId(sessionId)
    , m_key(key)
    , m_manager(remoteEdit)
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

RemoteEditInternalSession::RemoteEditInternalSession(
    uint sessionId, int key, const QString &title, const QString &body, RemoteEdit *parent)
    : RemoteEditSession(sessionId, key, parent)
    , m_widget(
          new RemoteEditWidget(isEditSession(),
                               title,
                               body,
                               checked_dynamic_downcast<QWidget *>(parent->parent()) // MainWindow
                               ))
{
    const auto widget = m_widget.data();
    connect(widget, &RemoteEditWidget::save, this, &RemoteEditSession::onSave);
    connect(widget, &RemoteEditWidget::cancel, this, &RemoteEditSession::onCancel);
}

RemoteEditInternalSession::~RemoteEditInternalSession()
{
    qDebug() << "Destructed RemoteEditInternalSession" << getId() << getKey();
    if (auto notLeaked = m_widget.take())
        notLeaked->deleteLater();
}

RemoteEditExternalSession::RemoteEditExternalSession(
    uint sessionId, int key, const QString &title, const QString &body, RemoteEdit *parent)
    : RemoteEditSession(sessionId, key, parent)
    , m_process(new RemoteEditProcess(isEditSession(), title, body, this))
{
    const auto proc = m_process.data();
    connect(proc, &RemoteEditProcess::save, this, &RemoteEditExternalSession::onSave);
    connect(proc, &RemoteEditProcess::cancel, this, &RemoteEditExternalSession::onCancel);
}

RemoteEditExternalSession::~RemoteEditExternalSession()
{
    qDebug() << "Destructed RemoteEditExternalSession" << getId() << getKey();
    if (auto notLeaked = m_process.take())
        notLeaked->deleteLater();
}
