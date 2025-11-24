// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "MapSource.h"

#include "../global/TextUtils.h"

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
            throw std::runtime_error(mmqt::toStdStringUtf8(pBuffer->errorString()));
        }
        return std::make_shared<MapSource>(Badge<MapSource>{},
                                           std::move(fileName),
                                           std::move(pBuffer));
    } else {
        auto pFile = std::make_shared<QFile>(fileName);
        if (!pFile->open(QFile::ReadOnly)) {
            throw std::runtime_error(mmqt::toStdStringUtf8(pFile->errorString()));
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
