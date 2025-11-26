#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/RuleOf5.h"
#include "../map/coordinate.h"
#include "../mapdata/mapdata.h"
#include "../mapfrontend/mapfrontend.h"
#include "abstractmapstorage.h"

#include <cstdint>
#include <optional>

#include <QArgument>
#include <QObject>
#include <QString>
#include <QtGlobal>

class QDataStream;
class QFile;
class QObject;

class NODISCARD_QOBJECT MapStorage final : public AbstractMapStorage
{
    Q_OBJECT

public:
    explicit MapStorage(const AbstractMapStorage::Data &, QObject *parent);

private:
    NODISCARD bool virt_canLoad() const final { return true; }
    NODISCARD std::optional<RawMapLoadData> virt_loadData() final;

private:
    NODISCARD bool virt_canSave() const final { return true; }
    NODISCARD bool virt_saveData(const MapLoadData &map) final;

private:
    NODISCARD static ExternalRawRoom loadRoom(QDataStream &stream, uint32_t version);
    static void loadExits(ExternalRawRoom &room, QDataStream &stream, uint32_t version);
    NODISCARD static RawInfomark loadMark(QDataStream &stream, uint32_t version);
    static void saveMark(const RawInfomark &mark, QDataStream &stream);
    static void saveRoom(const ExternalRawRoom &room, QDataStream &stream);
    static void saveExits(const ExternalRawRoom &room, QDataStream &stream);
    void log(const QString &msg);
};

struct NODISCARD MM2FileVersion final
{
    enum class NODISCARD Relative : uint8_t { Older, Current, Newer };

    uint32_t version = 0;
    Relative relative = Relative::Older;
};

NODISCARD extern std::optional<MM2FileVersion> getMM2FileVersion(QIODevice &file);
