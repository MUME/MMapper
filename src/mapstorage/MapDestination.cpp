// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "MapDestination.h"

#include "../global/ConfigConsts-Computed.h"
#include "../global/utils.h"
#include "filesaver.h"

#include <stdexcept>

#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QIODevice>

std::shared_ptr<MapDestination> MapDestination::alloc(QString fileName, SaveFormatEnum format)
{
    std::shared_ptr<FileSaver> fileSaver = nullptr;
    std::shared_ptr<QBuffer> buffer = nullptr;

    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        assert(format != SaveFormatEnum::WEB);
        buffer = std::make_shared<QBuffer>();
        if (!buffer->open(QIODevice::WriteOnly)) {
            throw std::runtime_error("Cannot open QBuffer for writing");
        }

    } else if (format == SaveFormatEnum::WEB) {
        QDir destDir(fileName);
        if (!destDir.exists()) {
            if (!QDir().mkpath(fileName)) {
                throw std::runtime_error("Cannot create directory");
            }
        }
        if (!QFileInfo(fileName).isWritable()) {
            throw std::runtime_error("Directory is not writable");
        }
    } else {
        fileSaver = std::make_shared<FileSaver>();
        try {
            fileSaver->open(fileName);
        } catch (const std::exception &e) {
            throw std::runtime_error(e.what());
        }
    }

    return std::make_shared<MapDestination>(Badge<MapDestination>{},
                                            std::move(fileName),
                                            std::move(fileSaver),
                                            std::move(buffer));
}

MapDestination::MapDestination(Badge<MapDestination>,
                               QString fileName,
                               std::shared_ptr<FileSaver> fileSaver,
                               std::shared_ptr<QBuffer> buffer)
    : m_fileName(std::move(fileName))
    , m_fileSaver(std::move(fileSaver))
    , m_buffer(std::move(buffer))
{
    if (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        assert(isFileWasm());
        std::ignore = deref(m_buffer);
    } else if (isFileNative()) {
        std::ignore = deref(m_fileSaver);
    } else {
        assert(isDirectory());
    }
}

MapDestination::~MapDestination()
{
    // finalize() calls FileSaver::close() which can throw on rename failure;
    // catch here to avoid std::terminate() if this destructor runs during
    // stack unwinding from a prior finalize() error.
    try {
        finalize();
    } catch (const std::exception &ex) {
        qWarning() << "MapDestination::~MapDestination() caught exception:" << ex.what();
    } catch (...) {
        qWarning() << "MapDestination::~MapDestination() caught unknown exception";
    }
}

std::shared_ptr<QIODevice> MapDestination::getIODevice() const
{
    if (isFileNative()) {
        return deref(m_fileSaver).getSharedFile();
    }
    if (isFileWasm()) {
        return m_buffer;
    }
    return nullptr;
}

QByteArray MapDestination::getWasmBufferData() const
{
    if (isFileWasm()) {
        return deref(m_buffer).data();
    }
    return {};
}

void MapDestination::finalize()
{
    if (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        assert(isFileWasm());
        std::ignore = deref(m_buffer);
    } else if (isFileNative()) {
        deref(m_fileSaver).close();
    } else {
        assert(isDirectory());
    }
}
