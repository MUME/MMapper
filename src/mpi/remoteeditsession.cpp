/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "remoteeditsession.h"

#include <cassert>
#include <QMessageLogContext>
#include <QObject>
#include <QScopedPointer>
#include <QString>

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
    , m_widget(new RemoteEditWidget(isEditSession(), title, body))
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
