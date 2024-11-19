// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "remoteeditprocess.h"

#include "../configuration/configuration.h"
#include "../global/Consts.h"
#include "../global/TextUtils.h"
#include "../global/io.h"
#include "../global/random.h"

#include <sstream>
#include <stdexcept>
#include <string>

#include <QDateTime>
#include <QMessageLogContext>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

static constexpr const std::string_view VALID
    = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static constexpr const auto VALID_LEN = VALID.length();

NODISCARD static std::string randomString(int length)
{
    std::ostringstream os;
    for (int i = 0; i < length; i++) {
        os << VALID[getRandom(VALID_LEN)];
    }
    return os.str();
}

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
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &RemoteEditProcess::slot_onFinished);
    connect(&m_process, &QProcess::errorOccurred, this, &RemoteEditProcess::slot_onError);

    // Set the file template
    QString fileTemplate = QString("%1MMapper.%2.pid%3.%4")
                               .arg(QDir::tempPath() + QDir::separator())    // %1
                               .arg(m_editSession ? "edit" : "view")         // %2
                               .arg(QCoreApplication::applicationPid())      // %3
                               .arg(mmqt::toQStringLatin1(randomString(6))); // %4 // ASCII
    QFile file(fileTemplate);

    // Try opening up the temporary file
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qCritical() << "View session was unable to create a temporary file";
        throw std::runtime_error("failed to start");
    }

    m_fileName = file.fileName();
    qDebug() << "View session file template" << m_fileName;
    file.write(m_body.toLatin1()); // MPI is always Latin1
    file.flush();

    // REVISIT: check return value?
    std::ignore = io::fsyncNoexcept(file);
    file.close();
    m_previousTime = QFileInfo{m_fileName}.lastModified();
    qDebug() << "File written with last modified timestamp" << m_previousTime;

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
    QFile file(m_fileName);
    file.remove();
}

void RemoteEditProcess::virt_onFinished(int exitCode, QProcess::ExitStatus status)
{
    qDebug() << "Edit session process finished with code" << exitCode;
    if (status != QProcess::NormalExit) {
        qWarning() << "File process did not end normally";
        qWarning() << "Output:" << m_process.readAll();
        emit sig_cancel();
        return;
    }

    if (!m_editSession) {
        emit sig_cancel();
        return;
    }

    QFile file(m_fileName);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Edit session unable to read file!";
        emit sig_cancel();
        return;
    }

    // See if the file was modified since we created it
    QDateTime currentTime = QFileInfo{file}.lastModified();
    if (m_previousTime == currentTime) {
        qDebug() << "Edit session canceled (no changes)";
        emit sig_cancel();
        return;
    }

    // Read the file
    QString content = QString::fromLatin1(file.readAll()); // MPI is always Latin1
    file.close();

    // Submit it to MUME
    qDebug() << "Edit session had changes" << content;
    emit sig_save(content);
}

void RemoteEditProcess::virt_onError(QProcess::ProcessError /*error*/)
{
    qWarning() << "View session encountered an error:" << m_process.errorString();
    qWarning() << "Output:" << m_process.readAll();
    emit sig_cancel();
}

enum class NODISCARD StateEnum { Idle, Arg, QuotedArg };

QStringList RemoteEditProcess::splitCommandLine(const QString &cmdLine)
{
    using char_consts::C_DQUOTE;
    // https://stackoverflow.com/questions/25068750/extract-parameters-from-string-included-quoted-regions-in-qt
    QStringList list;
    QString arg;
    bool escape = false;
    StateEnum state = StateEnum::Idle;
    for (const QChar c : cmdLine) {
        if (!escape && c == char_consts::C_BACKSLASH) {
            escape = true;
            continue;
        }
        switch (state) {
        case StateEnum::Idle:
            if (!escape && c == C_DQUOTE) {
                state = StateEnum::QuotedArg;
            } else if (escape || !c.isSpace()) {
                arg += c;
                state = StateEnum::Arg;
            }
            break;
        case StateEnum::Arg:
            if (!escape && c == C_DQUOTE) {
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
            if (!escape && c == C_DQUOTE) {
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
