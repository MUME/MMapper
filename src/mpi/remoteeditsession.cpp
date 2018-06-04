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
#include "remoteedit.h"
#include "remoteeditprocess.h"
#include "remoteeditwidget.h"

#include <cassert>
#include <utility>
#include <QDebug>

RemoteEditSession::RemoteEditSession(uint sessionId, int key, RemoteEdit *remoteEdit)
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
    uint sessionId, int key, QString title, QString body, RemoteEdit *parent)
    : RemoteEditSession(sessionId, key, parent)
    , m_widget(new RemoteEditWidget(isEditSession(), title, body))
{
    connect(m_widget, &RemoteEditWidget::save, this, &RemoteEditSession::onSave);
    connect(m_widget, &RemoteEditWidget::cancel, this, &RemoteEditSession::onCancel);
}

RemoteEditInternalSession::~RemoteEditInternalSession()
{
    qDebug() << "Destructed RemoteEditInternalSession" << getId() << getKey();
    m_widget->deleteLater();
}

RemoteEditExternalSession::RemoteEditExternalSession(
    uint sessionId, int key, QString title, QString body, RemoteEdit *parent)
    : RemoteEditSession(sessionId, key, parent)
    , m_process(new RemoteEditProcess(isEditSession(), title, body, this))
{
    connect(m_process, &RemoteEditProcess::save, this, &RemoteEditExternalSession::onSave);
    connect(m_process, &RemoteEditProcess::cancel, this, &RemoteEditExternalSession::onCancel);
}

RemoteEditExternalSession::~RemoteEditExternalSession()
{
    qDebug() << "Destructed RemoteEditExternalSession" << getId() << getKey();
    m_process->deleteLater();
}
