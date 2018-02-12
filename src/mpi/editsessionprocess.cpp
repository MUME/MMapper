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

#include "editsessionprocess.h"

#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QTemporaryFile>
#include <QFileInfo>

EditSessionProcess::EditSessionProcess(int key, const QString &title, const QString &body,
                                       QObject *parent)
    : ViewSessionProcess(key, title, body, parent)
{
    // Store the file information
    QFileInfo fileInfo(m_file);
    m_previousTime = fileInfo.lastModified();
}

EditSessionProcess::~EditSessionProcess()
{
    qDebug() << "Edit session" << m_key << "was destroyed";
}

void EditSessionProcess::onFinished(int exitCode, QProcess::ExitStatus status)
{
    qDebug() << "Edit session" << m_key << "process finished with code" << exitCode;
    if (status == QProcess::NormalExit) {
        // See if the file was modified since we created it
        QFileInfo fileInfo(m_file);
        QDateTime currentTime = fileInfo.lastModified();
        if (m_previousTime != currentTime) {
            // Read the file and submit it to MUME
            qDebug() << "Edit session" << m_key << "had changes, reading";
            if (m_file.open())
                m_body = m_file.readAll();
            else
                qWarning() << "Edit session" << m_key << "unable to read file!";
            return finishEdit();

        } else qDebug() << "Edit session" << m_key << "canceled (no changes)";

    } else qWarning() << "File process did not end normally";

    // Cancel
    cancelEdit();
}

void EditSessionProcess::onError(QProcess::ProcessError /*error*/)
{
    qWarning() << "Edit session" << m_key << "encountered an error:" << errorString();
    qWarning() << "Output:" << readAll();
    cancelEdit();
}


void EditSessionProcess::cancelEdit()
{
    emit cancel(m_key);
    deleteLater();
}


void EditSessionProcess::finishEdit()
{
    emit save(m_body, m_key);
    deleteLater();
}
