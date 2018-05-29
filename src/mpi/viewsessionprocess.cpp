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

#include "viewsessionprocess.h"
#include "configuration/configuration.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QTemporaryFile>
#include <utility>

ViewSessionProcess::ViewSessionProcess(int key, QString title, QString body,
                                       QObject *parent)
    : QProcess(parent), m_key(key), m_title(std::move(title)), m_body(std::move(body))
{
    setReadChannelMode(QProcess::MergedChannels);

    // Signals/Slots
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
            SLOT(onFinished(int, QProcess::ExitStatus)));
    connect(this, SIGNAL(error(QProcess::ProcessError)),
            SLOT(onError(QProcess::ProcessError)));

    QString keyTemp;
    if (m_key == -1) {
        keyTemp = "view";
    } else {
        keyTemp = QString("key%1").arg(m_key);
    }

    // Set the file template
    QString fileTemplate = QString("%1MMapper.%2.pid%3.XXXXXX")
                           .arg(QDir::tempPath() + QDir::separator()) // %1
                           .arg(keyTemp)                              // %2
                           .arg(QCoreApplication::applicationPid());  // %3
    m_file.setFileTemplate(fileTemplate);

    // Try opening up the temporary file
    if (m_file.open()) {
        const QString &fileName = m_file.fileName();
        qDebug() << "View session file template" << fileName;
        m_file.write(m_body.toLatin1());
        m_file.flush();
        m_file.close();

        if (!QFile::exists(m_file.fileName())) {
            qWarning() << "File does not exist!" << m_file.fileName();
        }

        // Set the TITLE environmental variable
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        if (env.contains("TITLE")) {
            env.remove("TITLE");
        }
        env.insert("TITLE", m_title);
        setProcessEnvironment(env);

        // Start the process!
        QStringList args = splitCommandLine(Config().m_externalRemoteEditorCommand);
        args << fileName;
        const QString &program = args.takeFirst();
        qDebug() << program << args;
        start(program, args);

        qDebug() << "View session" << m_key << m_title << "started";

    } else {
        qCritical() << "View session was unable to create a temporary file";
        // Calling a not virtual method from the ctor is undefined behavior
        // // onError(QProcess::FailedToStart);
        // consider throwing instead?
        // // throw std::runtime_error("failed to start");
    }
}

ViewSessionProcess::~ViewSessionProcess()
    = default;

void ViewSessionProcess::onFinished(int exitCode, QProcess::ExitStatus status)
{
    qDebug() << "View session" << m_key << "process finished with code" << exitCode;
    if (status != QProcess::NormalExit) {
        qWarning() << "Process did not end normally" << exitCode;
    }
    deleteLater();
}

void ViewSessionProcess::onError(QProcess::ProcessError /*error*/)
{
    qWarning() << "View session" << m_key << "encountered an error:" << errorString();
    qWarning() << "Output:" << readAll();
    deleteLater();
}

QStringList ViewSessionProcess::splitCommandLine(const QString &cmdLine)
{
    // https://stackoverflow.com/questions/25068750/extract-parameters-from-string-included-quoted-regions-in-qt
    QStringList list;
    QString arg;
    bool escape = false;
    enum { Idle, Arg, QuotedArg } state = Idle;
    for (QChar const c : cmdLine) {
        if (!escape && c == '\\') {
            escape = true;
            continue;
        }
        switch (state) {
        case Idle:
            if (!escape && c == '"') {
                state = QuotedArg;
            } else if (escape || !c.isSpace()) {
                arg += c;
                state = Arg;
            }
            break;
        case Arg:
            if (!escape && c == '"') {
                state = QuotedArg;
            } else if (escape || !c.isSpace()) {
                arg += c;
            } else {
                list << arg;
                arg.clear();
                state = Idle;
            }
            break;
        case QuotedArg:
            if (!escape && c == '"') {
                state = arg.isEmpty() ? Idle : Arg;
            } else {
                arg += c;
            }
            break;
        }
        escape = false;
    }
    if (!arg.isEmpty()) {
        list << arg;
    }
    return list;
}
