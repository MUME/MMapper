// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "./configuration/configuration.h"
#include "./display/Filenames.h"
#include "./global/Debug.h"
#include "./global/Version.h"
#include "./global/WinSock.h"
#include "./global/utils.h"
#include "./mainwindow/mainwindow.h"

#include <memory>
#include <optional>
#include <set>
#include <thread>

#include <QPixmap>
#include <QtCore>
#include <QtWidgets>

#ifdef WITH_DRMINGW
#include <exchndl.h>
#endif

// REVISIT: move splash files somewhere else?
// (presumably not in the "root" src/ directory?)
struct NODISCARD ISplash
{
public:
    virtual ~ISplash();

private:
    virtual void virt_finish(QWidget *) = 0;

public:
    void finish(QWidget *const w) { virt_finish(w); }
};

ISplash::~ISplash() = default;

struct NODISCARD FakeSplash final : public ISplash
{
public:
    ~FakeSplash() final;

private:
    void virt_finish(QWidget *) final {}
};

FakeSplash::~FakeSplash() = default;

class NODISCARD Splash final : public ISplash
{
private:
    QPixmap pixmap;
    QSplashScreen splash;

public:
    Splash()
        : pixmap(getPixmapFilenameRaw("splash.png"))
        , splash(pixmap)
    {
        const auto message = QString("%1").arg(QLatin1String(getMMapperVersion()), -9);
        splash.showMessage(message, Qt::AlignBottom | Qt::AlignRight, Qt::yellow);
        splash.show();
    }
    ~Splash() final;

private:
    void virt_finish(QWidget *const w) override { splash.finish(w); }
};

Splash::~Splash() = default;

static void useHighDpi()
{
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
}

static void setHighDpiScaleFactorRoundingPolicy()
{
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
    ExcHndlSetLogFileNameA(logFile.toLocal8Bit());
#endif
}

NODISCARD static bool tryLoad(MainWindow &mw, const QDir &dir, const QString &input_filename)
{
    const auto getAbsoluteFileName = [](const QDir &dir,
                                        const QString &input_filename) -> std::optional<QString> {
        if (QFileInfo{input_filename}.isAbsolute())
            return input_filename;

        if (!dir.exists()) {
            qInfo() << "[main] Directory" << dir.absolutePath() << "does not exist.";
            return std::nullopt;
        }

        return dir.absoluteFilePath(input_filename);
    };

    const auto maybeFilename = getAbsoluteFileName(dir, input_filename);
    if (!maybeFilename)
        return false;

    const auto &absoluteFilePath = maybeFilename.value();
    if (!QFile{absoluteFilePath}.exists()) {
        qInfo() << "[main] File " << absoluteFilePath << "does not exist.";
        return false;
    }

    mw.loadFile(absoluteFilePath);
    return true;
}

static void tryAutoLoad(MainWindow &mw)
{
    const auto &settings = getConfig().autoLoad;
    if (settings.autoLoadMap) {
        if (!settings.fileName.isEmpty()
            && tryLoad(mw, QDir{settings.lastMapDirectory}, settings.fileName))
            return;
        if (!NO_MAP_RESOURCE && tryLoad(mw, QDir(":/"), "arda.mm2"))
            return;
        qInfo() << "[main] Unable to autoload map";
    }
}

static void setSurfaceFormat()
{
    const auto &config = getConfig().canvas;
    if (config.softwareOpenGL) {
        QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Linux) {
            qputenv("LIBGL_ALWAYS_SOFTWARE", "1");
        }
    } else if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows) {
        // Windows Intel drivers cause black screens if we don't specify OpenGL
        QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    }

    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    QSurfaceFormat::FormatOptions options = QSurfaceFormat::DebugContext
                                            | QSurfaceFormat::DeprecatedFunctions;
    fmt.setOptions(options);
    fmt.setSamples(config.antialiasingSamples);
    fmt.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(fmt);
}

int main(int argc, char **argv)
{
    useHighDpi();
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
    setSurfaceFormat();

    const auto &config = getConfig();
    std::unique_ptr<ISplash> splash = !config.general.noSplash
                                          ? static_upcast<ISplash>(std::make_unique<Splash>())
                                          : static_upcast<ISplash>(std::make_unique<FakeSplash>());
    auto mw = std::make_unique<MainWindow>();
    tryAutoLoad(*mw);
    mw->show();
    splash->finish(mw.get());
    splash.reset();
    const int ret = QApplication::exec();
    mw.reset();
    config.write();
    return ret;
}
