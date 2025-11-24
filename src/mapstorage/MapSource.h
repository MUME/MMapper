#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../global/Badge.h"
#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <memory>
#include <optional>

#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>

class NODISCARD MapSource final : public std::enable_shared_from_this<MapSource>
{
private:
    QString m_fileName;
    std::shared_ptr<QIODevice> m_device;

public:
    NODISCARD static std::shared_ptr<MapSource> alloc(
        const QString fileName,
        const std::optional<QByteArray> &fileContent = std::nullopt) CAN_THROW;

public:
    explicit MapSource(Badge<MapSource>, const QString fileName, std::shared_ptr<QIODevice> device);
    DELETE_CTORS(MapSource);
    DELETE_COPY_ASSIGN_OP(MapSource);

private:
    DEFAULT_MOVE_ASSIGN_OP(MapSource);

public:
    NODISCARD std::shared_ptr<QIODevice> getIODevice() { return m_device; }
    NODISCARD const QString &getFileName() const { return m_fileName; }
};
