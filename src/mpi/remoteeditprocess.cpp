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

#include <stdexcept>
#include <utility>
#include <QDateTime>
#include <QMessageLogContext>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

#include "../configuration/configuration.h"
#include "../global/io.h"

RemoteEditProcess::RemoteEditProcess(const bool editSession,
                                     const QString &title,
                                     const QString &body,
                                     QObject *const parent)
    : QObject(parent)
    , m_title(title)
    , m_body(body)
    , m_editSession(editSession)
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
    if (!m_file.open()) {
        qCritical() << "View session was unable to create a temporary file";
        throw std::runtime_error("failed to start");
    }

    const QString &fileName = m_file.fileName();
    qDebug() << "View session file template" << fileName;
    m_file.write(m_body.toLatin1());
    m_file.flush();
    io::fsyncNoexcept(m_file);
    m_file.close();
    m_previousTime = QFileInfo{m_file}.lastModified();

    // Set the TITLE environmental variable
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains("TITLE")) {
        env.remove("TITLE");
    }
    env.insert("TITLE", m_title);
    m_process.setProcessEnvironment(env);

    // Start the process!
    QStringList args = splitCommandLine(Config().mumeClientProtocol.externalRemoteEditorCommand);
    args << fileName;
    const QString &program = args.takeFirst();
    qDebug() << program << args;
    m_process.start(program, args);

    qDebug() << "View session started";
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

enum class State { Idle, Arg, QuotedArg };

QStringList RemoteEditProcess::splitCommandLine(const QString &cmdLine)
{
    // https://stackoverflow.com/questions/25068750/extract-parameters-from-string-included-quoted-regions-in-qt
    QStringList list;
    QString arg;
    bool escape = false;
    State state = State::Idle;
    for (QChar const c : cmdLine) {
        if (!escape && c == '\\') {
            escape = true;
            continue;
        }
        switch (state) {
        case State::Idle:
            if (!escape && c == '"') {
                state = State::QuotedArg;
            } else if (escape || !c.isSpace()) {
                arg += c;
                state = State::Arg;
            }
            break;
        case State::Arg:
            if (!escape && c == '"') {
                state = State::QuotedArg;
            } else if (escape || !c.isSpace()) {
                arg += c;
            } else {
                list << arg;
                arg.clear();
                state = State::Idle;
            }
            break;
        case State::QuotedArg:
            if (!escape && c == '"') {
                state = arg.isEmpty() ? State::Idle : State::Arg;
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
