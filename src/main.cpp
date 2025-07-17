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
#include "./opengl/OpenGL.h"

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

    mw.loadFile(absoluteFilePath);
    return true;
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

static void setSurfaceFormat()
{
    // Probe for supported OpenGL versions
    QSurfaceFormat fmt = OpenGL::createDefaultSurfaceFormat();

    const auto &config = getConfig().canvas;
    fmt.setSamples(config.antialiasingSamples);
    QSurfaceFormat::setDefaultFormat(fmt);
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
    setSurfaceFormat();

    tryLoadEmojis(getResourceFilenameRaw("emojis", "short-codes.json"));
    auto mw = std::make_unique<MainWindow>();
    tryAutoLoadMap(*mw);
    const int ret = QApplication::exec();
    mw.reset();
    getConfig().write();
    return ret;
}
