// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

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
    m_process.setProcessChannelMode(QProcess::MergedChannels);

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

    m_fileName = m_file.fileName();
    qDebug() << "View session file template" << m_fileName;
    m_file.write(m_body.toLatin1()); // note: MUME expects all remote edit data to be Latin-1.
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
    QStringList args = splitCommandLine(getConfig().mumeClientProtocol.externalRemoteEditorCommand);
    args << m_fileName;
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
    if (status != QProcess::NormalExit) {
        qWarning() << "File process did not end normally";
        qWarning() << "Output:" << m_process.readAll();
        emit cancel();
        return;
    }

    if (!m_editSession) {
        emit cancel();
        return;
    }

    QFile read(m_fileName);
    if (!read.open(QFile::ReadOnly)) {
        qWarning() << "Edit session unable to read file!";
        emit cancel();
        return;
    }

    // See if the file was modified since we created it
    QFileInfo fileInfo(read);
    QDateTime currentTime = fileInfo.lastModified();
    if (m_previousTime == currentTime) {
        qDebug() << "Edit session canceled (no changes)";
        emit cancel();
        return;
    }

    // Read the file
    QString content = QString::fromLatin1(read.readAll());
    read.close();

    // Submit it to MUME
    qDebug() << "Edit session had changes" << content;
    emit save(content);
}

void RemoteEditProcess::onError(QProcess::ProcessError /*error*/)
{
    qWarning() << "View session encountered an error:" << m_process.errorString();
    qWarning() << "Output:" << m_process.readAll();
    emit cancel();
}

enum class StateEnum { Idle, Arg, QuotedArg };

QStringList RemoteEditProcess::splitCommandLine(const QString &cmdLine)
{
    // https://stackoverflow.com/questions/25068750/extract-parameters-from-string-included-quoted-regions-in-qt
    QStringList list;
    QString arg;
    bool escape = false;
    StateEnum state = StateEnum::Idle;
    for (const QChar &c : cmdLine) {
        if (!escape && c == '\\') {
            escape = true;
            continue;
        }
        switch (state) {
        case StateEnum::Idle:
            if (!escape && c == '"') {
                state = StateEnum::QuotedArg;
            } else if (escape || !c.isSpace()) {
                arg += c;
                state = StateEnum::Arg;
            }
            break;
        case StateEnum::Arg:
            if (!escape && c == '"') {
                state = StateEnum::QuotedArg;
            } else if (escape || !c.isSpace()) {
                arg += c;
            } else {
                list << arg;
                arg.clear();
                state = StateEnum::Idle;
            }
            break;
        case StateEnum::QuotedArg:
            if (!escape && c == '"') {
                state = arg.isEmpty() ? StateEnum::Idle : StateEnum::Arg;
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
