// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "./configuration/configuration.h"
#include "./display/Filenames.h"
#include "./global/ConfigConsts.h"
#include "./global/WinSock.h"
#include "./global/emojis.h"
#include "./mainwindow/WinDarkMode.h"
#include "./mainwindow/mainwindow.h"
#include "./opengl/OpenGLConfig.h"
#include "./opengl/OpenGLProber.h"

#include <memory>
#include <optional>

#include <QPixmap>
#include <QtCore>
#include <QtWidgets>

#ifdef WITH_DRMINGW
#include <exchndl.h>
#endif

static void setHighDpiScaleFactorRoundingPolicy()
{
    // High Dpi is enabled by default in Qt6
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
}

static void tryInitDrMingw()
{
#ifdef WITH_DRMINGW
    ExcHndlInit();
    // Set the log file path to %LocalAppData%\mmappercrash.log
    QString logFile = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                          .replace(L'/', L'\\')
                      + QStringLiteral("\\mmappercrash.log");
    ExcHndlSetLogFileNameA(logFile.toUtf8().constData());
#endif
}

NODISCARD static bool tryLoad(MainWindow &mw, const QDir &dir, const QString &input_filename)
{
    const auto getAbsoluteFileName = [&dir, &input_filename]() -> std::optional<QString> {
        if (QFileInfo{input_filename}.isAbsolute()) {
            return input_filename;
        }

        if (!dir.exists()) {
            qInfo() << "[main] Directory" << dir.absolutePath() << "does not exist.";
            return std::nullopt;
        }

        return dir.absoluteFilePath(input_filename);
    };

    const auto maybeFilename = getAbsoluteFileName();
    if (!maybeFilename) {
        return false;
    }

    const auto &absoluteFilePath = maybeFilename.value();
    if (!QFile{absoluteFilePath}.exists()) {
        qInfo() << "[main] File " << absoluteFilePath << "does not exist.";
        return false;
    }

    try {
        mw.loadFile(MapSource::alloc(absoluteFilePath, std::nullopt));
        return true;
    } catch (const std::runtime_error &e) {
        qCritical() << "Failed to load autoload map:" << e.what();
        return false;
    }
}

static void tryAutoLoadMap(MainWindow &mw)
{
    const auto &settings = getConfig().autoLoad;
    if (settings.autoLoadMap) {
        if (!settings.fileName.isEmpty()
            && tryLoad(mw, QDir{settings.lastMapDirectory}, settings.fileName)) {
            return;
        }
        if (!NO_MAP_RESOURCE && tryLoad(mw, QDir(":/"), "arda")) {
            return;
        }
        qInfo() << "[main] Unable to autoload map";
    }
}

static bool setSurfaceFormat()
{
    OpenGLProber prober;
    auto probeResult = prober.probe();
    if (probeResult.backendType == OpenGLProber::BackendType::None) {
        QString msg = "No compatible rendering backend found.\n\nThe applications requires ";
        if constexpr (!NO_OPENGL) {
            msg += "OpenGL 3.3";
        }
        if constexpr (!NO_OPENGL && !NO_GLES) {
            msg += " or ";
        }
        if constexpr (!NO_GLES) {
            msg += "OpenGL ES 3.0";
        }
        msg += " support to run.";
        QMessageBox::critical(nullptr, "Fatal Error", msg);
        return false;
    }

    QSurfaceFormat fmt = probeResult.format;
    OpenGLConfig::setHighestReportableVersionString(probeResult.highestVersionString);
    OpenGLConfig::setBackendType(probeResult.backendType);
    OpenGLConfig::setIsCompat(probeResult.isCompat);
    fmt.setSamples(getConfig().canvas.antialiasingSamples);
    QSurfaceFormat::setDefaultFormat(fmt);
    return true;
}

int main(int argc, char **argv)
{
    setHighDpiScaleFactorRoundingPolicy();
    setEnteredMain();
    if constexpr (IS_DEBUG_BUILD) {
        // see http://doc.qt.io/qt-5/qtglobal.html#qSetMessagePattern
        // also allows environment variable QT_MESSAGE_PATTERN
        qSetMessagePattern(
            "[%{time} %{threadid}] %{type} in %{function} (at %{file}:%{line}): %{message}");
    }

    QApplication app(argc, argv);
    tryInitDrMingw();
    auto tryLoadingWinSock = std::make_unique<WinSock>();
    auto tryLoadingWinDarkMode = std::make_unique<WinDarkMode>(&app);
    if (!setSurfaceFormat()) {
        return 1;
    }

    tryLoadEmojis(getResourceFilenameRaw("emojis", "short-codes.json"));
    auto mw = std::make_unique<MainWindow>();
    tryAutoLoadMap(*mw);
    const int ret = QApplication::exec();
    mw.reset();
    getConfig().write();
    return ret;
}
