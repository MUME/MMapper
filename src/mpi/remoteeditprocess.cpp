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

#include "remoteeditprocess.h"
#include "configuration/configuration.h"

#include <utility>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QTemporaryFile>

#ifdef __APPLE__
#include <fcntl.h>
#elif not defined(__WIN32__)
#include <unistd.h>
#endif

RemoteEditProcess::RemoteEditProcess(bool editSession,
                                     const QString &title,
                                     const QString &body,
                                     QObject *parent)
    : QObject(parent)
    , m_editSession(editSession)
    , m_title(std::move(title))
    , m_body(std::move(body))
{
    m_process.setReadChannelMode(QProcess::MergedChannels);

    // Signals/Slots
    connect(&m_process,
            SIGNAL(finished(int, QProcess::ExitStatus)),
            SLOT(onFinished(int, QProcess::ExitStatus)));
    connect(&m_process,
            SIGNAL(error(QProcess::ProcessError)),
            SLOT(onError(QProcess::ProcessError)));

    // Set the file template
    QString fileTemplate = QString("%1MMapper.%2.pid%3.XXXXXX")
                               .arg(QDir::tempPath() + QDir::separator()) // %1
                               .arg(m_editSession ? "edit" : "view")      // %2
                               .arg(QCoreApplication::applicationPid());  // %3
    m_file.setFileTemplate(fileTemplate);
    // Try opening up the temporary file
    if (m_file.open()) {
        const QString &fileName = m_file.fileName();
        qDebug() << "View session file template" << fileName;
        m_file.write(m_body.toLatin1());
        m_file.flush();
#ifdef __APPLE__
        fcntl(m_file.handle(), F_FULLFSYNC);
#elif not defined(__WIN32__)
        fsync(m_file.handle());
#endif
        m_file.close();
        QFileInfo fileInfo(m_file);
        m_previousTime = fileInfo.lastModified();

        // Set the TITLE environmental variable
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        if (env.contains("TITLE")) {
            env.remove("TITLE");
        }
        env.insert("TITLE", m_title);
        m_process.setProcessEnvironment(env);

        // Start the process!
        QStringList args = splitCommandLine(Config().m_externalRemoteEditorCommand);
        args << fileName;
        const QString &program = args.takeFirst();
        qDebug() << program << args;
        m_process.start(program, args);

        qDebug() << "View session started";
    } else {
        qCritical() << "View session was unable to create a temporary file";
        throw std::runtime_error("failed to start");
    }
}

RemoteEditProcess::~RemoteEditProcess()
{
    qInfo() << "Destroyed RemoteEditProcess";
}

void RemoteEditProcess::onFinished(int exitCode, QProcess::ExitStatus status)
{
    qDebug() << "Edit session process finished with code" << exitCode;
    if (status == QProcess::NormalExit) {
        if (m_editSession) {
            if (m_file.open()) {
                // See if the file was modified since we created it
                QFileInfo fileInfo(m_file);
                QDateTime currentTime = fileInfo.lastModified();
                if (m_previousTime != currentTime) {
                    // Read the file and submit it to MUME
                    QString content = QString::fromLatin1(m_file.readAll());
                    qDebug() << "Edit session had changes" << content;
                    emit save(content);
                } else {
                    qDebug() << "Edit session canceled (no changes)";
                }
                m_file.close();
            } else {
                qWarning() << "Edit session unable to read file!";
            }
        }

    } else {
        qWarning() << "File process did not end normally";
        qWarning() << "Output:" << m_process.readAll();
    }

    emit cancel();
}

void RemoteEditProcess::onError(QProcess::ProcessError /*error*/)
{
    qWarning() << "View session encountered an error:" << m_process.errorString();
    qWarning() << "Output:" << m_process.readAll();
    emit cancel();
}

QStringList RemoteEditProcess::splitCommandLine(const QString &cmdLine)
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
