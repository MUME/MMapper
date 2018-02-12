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

#include "remoteedit.h"
#include "remoteeditwidget.h"
#include "viewsessionprocess.h"
#include "editsessionprocess.h"
#include "configuration/configuration.h"

#include <QDebug>

const QRegExp RemoteEdit::s_lineFeedNewlineRx("(?!\\r)\\n");

RemoteEdit::RemoteEdit(QObject *parent)
    : QObject(parent)
{}

RemoteEdit::~RemoteEdit()
{
}

void RemoteEdit::remoteView(QString title, QString body)
{
#ifdef Q_OS_WIN
    body.replace(s_lineFeedNewlineRx, "\r\n");
#endif
    if (Config().m_internalRemoteEditor) {
        new RemoteEditWidget(-1, title, body);
    } else {
        new ViewSessionProcess(-1, title, body, this);
    }
}

void RemoteEdit::remoteEdit(const int key, QString title, QString body)
{
#ifdef Q_OS_WIN
    body.replace(s_lineFeedNewlineRx, "\r\n");
#endif
    if (Config().m_internalRemoteEditor) {
        RemoteEditWidget *widget = new RemoteEditWidget(key, title, body);
        connect(widget, SIGNAL(save(const QString &, const int)), SLOT(save(const QString &,
                                                                            const int)));
        connect(widget, SIGNAL(cancel(const int)), SLOT(cancel(const int)));

    } else {
        EditSessionProcess *process = new EditSessionProcess(key, title, body, this);
        connect(process, SIGNAL(save(const QString &, const int)),
                SLOT(save(const QString &, const int)));
        connect(process, SIGNAL(cancel(const int)), SLOT(cancel(const int)));
    }
}

void RemoteEdit::cancel(const int key)
{
    const QString &keystr = QString("C%1\n").arg(key);
    const QByteArray &buffer = QString("%1E%2\n%3")
                               .arg("~$#E")
                               .arg(keystr.length())
                               .arg(keystr)
                               .toLatin1();

    qDebug() << "Cancel" << buffer;
    emit sendToSocket(buffer);
}

void RemoteEdit::save(const QString &body, const int key)
{
    QString content = body;
#ifdef Q_OS_WIN
    content.replace("\r\n", "\n");
#endif
    // The body contents have to be followed by a LF if they are not empty
    if (!content.isEmpty() && !content.endsWith('\n'))
        content.append('\n');

    const QString &keystr = QString("E%1\n").arg(key);
    const QByteArray &buffer = QString("%1E%2\n%3%4")
                               .arg("~$#E")
                               .arg(body.length() + keystr.length())
                               .arg(keystr)
                               .arg(content)
                               .toLatin1();

    qInfo() << "Save" << buffer;
    emit sendToSocket(buffer);
}
