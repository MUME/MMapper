// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "MapSource.h"

#include <stdexcept>

#include <QBuffer>
#include <QFile>

std::shared_ptr<MapSource> MapSource::alloc(const QString fileName,
                                            const std::optional<QByteArray> &fileContent)
{
    if (fileContent.has_value()) {
        auto pBuffer = std::make_shared<QBuffer>();
        pBuffer->setData(fileContent.value());
        if (!pBuffer->open(QIODevice::ReadOnly)) {
            throw std::runtime_error(QString("Failed to open QBuffer for reading: %1.")
                                         .arg(pBuffer->errorString())
                                         .toStdString());
        }
        return std::make_shared<MapSource>(Badge<MapSource>{},
                                           std::move(fileName),
                                           std::move(pBuffer));
    } else {
        auto pFile = std::make_shared<QFile>(fileName);
        if (!pFile->open(QFile::ReadOnly)) {
            throw std::runtime_error(QString("Failed to open file \"%1\" for reading: %2.")
                                         .arg(fileName, pFile->errorString())
                                         .toStdString());
        }
        return std::make_shared<MapSource>(Badge<MapSource>{},
                                           std::move(fileName),
                                           std::move(pFile));
    }
}

MapSource::MapSource(Badge<MapSource>, const QString fileName, std::shared_ptr<QIODevice> device)
    : m_fileName(std::move(fileName))
    , m_device(std::move(device))
{}
