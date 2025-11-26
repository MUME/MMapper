// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "MapDestination.h"

#include "filesaver.h"

#include <stdexcept>

#include <QBuffer>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QIODevice>

std::shared_ptr<MapDestination> MapDestination::alloc(const QString fileName, SaveFormatEnum format)
{
    std::shared_ptr<FileSaver> fileSaver = nullptr;

    if (format == SaveFormatEnum::WEB) {
        QDir destDir(fileName);
        if (!destDir.exists()) {
            if (!destDir.mkpath(fileName)) {
                throw std::runtime_error(
                    QString("Cannot create directory %1.").arg(fileName).toStdString());
            }
        }
        if (!QFileInfo(fileName).isWritable()) {
            throw std::runtime_error(
                QString("Directory %1 is not writable.").arg(fileName).toStdString());
        }
    } else {
        fileSaver = std::make_shared<FileSaver>();
        try {
            fileSaver->open(fileName);
        } catch (const std::exception &e) {
            throw std::runtime_error(
                QString("Cannot write file %1:\n%2.").arg(fileName, e.what()).toStdString());
        }
    }

    return std::make_shared<MapDestination>(Badge<MapDestination>{},
                                            std::move(fileName),
                                            std::move(fileSaver));
}

MapDestination::MapDestination(Badge<MapDestination>,
                               const QString fileName,
                               std::shared_ptr<FileSaver> fileSaver)
    : m_fileName(std::move(fileName))
    , m_fileSaver(std::move(fileSaver))
{}

std::shared_ptr<QIODevice> MapDestination::getIODevice() const
{
    if (isFileNative()) {
        assert(m_fileSaver);
        return m_fileSaver->getSharedFile();
    }
    return nullptr;
}

void MapDestination::finalize(bool /*success*/)
{
    if (isFileNative()) {
        assert(m_fileSaver);
        m_fileSaver->close();
    } else {
        assert(isDirectory());
    }
}
