#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../global/Badge.h"
#include "../global/macros.h"
#include "filesaver.h"

#include <memory>

#include <QBuffer>
#include <QIODevice>
#include <QString>

enum class NODISCARD SaveModeEnum { FULL, BASEMAP };
enum class NODISCARD SaveFormatEnum { MM2, MM2XML, WEB, MMP };

class NODISCARD MapDestination final : public std::enable_shared_from_this<MapDestination>
{
private:
    QString m_fileName;
    std::shared_ptr<FileSaver> m_fileSaver;
    std::shared_ptr<QBuffer> m_buffer;

public:
    NODISCARD static std::shared_ptr<MapDestination> alloc(const QString fileName,
                                                           SaveFormatEnum format) CAN_THROW;

public:
    explicit MapDestination(Badge<MapDestination>,
                            const QString fileName,
                            std::shared_ptr<FileSaver> fileSaver,
                            std::shared_ptr<QBuffer> buffer);
    DELETE_CTORS(MapDestination);
    DELETE_COPY_ASSIGN_OP(MapDestination);

private:
    DEFAULT_MOVE_ASSIGN_OP(MapDestination);

public:
    NODISCARD bool isFileNative() const { return m_fileSaver != nullptr; }
    NODISCARD bool isFileWasm() const { return m_buffer != nullptr; }
    NODISCARD bool isDirectory() const { return !isFileNative() && !isFileWasm(); }

    NODISCARD const QString &getFileName() const { return m_fileName; }

    NODISCARD std::shared_ptr<QIODevice> getIODevice() const;
    NODISCARD const QByteArray getWasmBufferData() const;

    void finalize();
};
