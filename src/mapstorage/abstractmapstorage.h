#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "../global/progresscounter.h"
#include "../map/Map.h"
#include "MapDestination.h"
#include "MapSource.h"
#include "RawMapData.h"

#include <memory>
#include <optional>

#include <QObject>
#include <QString>

class MapData;
class ProgressCounter;

struct NODISCARD MapStorageError final : public std::runtime_error
{
    explicit MapStorageError(const std::string &why)
        : std::runtime_error(why)
    {}
    ~MapStorageError() final;
};

class NODISCARD_QOBJECT AbstractMapStorage : public QObject
{
    Q_OBJECT

public:
    struct NODISCARD Data final
    {
        enum class Type { File, Directory };

        Type destinationType = Type::File;
        const std::shared_ptr<ProgressCounter> progressCounter;
        std::shared_ptr<MapSource> loadSource;
        std::shared_ptr<MapDestination> saveDestination;

        explicit Data(std::shared_ptr<ProgressCounter> moved_pc,
                      std::shared_ptr<MapSource> moved_src)
            : progressCounter(std::move(moved_pc))
            , loadSource(std::move(moved_src))
        {
            if (!progressCounter) {
                throw std::invalid_argument("pc");
            }
            if (!loadSource) {
                throw std::invalid_argument("src");
            }
        }

        explicit Data(std::shared_ptr<ProgressCounter> moved_pc,
                      std::shared_ptr<MapDestination> moved_dest)
            : progressCounter(std::move(moved_pc))
            , saveDestination(std::move(moved_dest))
        {
            if (!progressCounter) {
                throw std::invalid_argument("pc");
            }
            if (!saveDestination) {
                throw std::invalid_argument("dest");
            }
            destinationType = saveDestination->isDirectory() ? Type::Directory : Type::File;
        }
    };

private:
    Data m_data;

public:
    explicit AbstractMapStorage(const Data &data, QObject *parent);
    ~AbstractMapStorage() override;

protected:
    NODISCARD const QString &getFilename() const;
    NODISCARD ProgressCounter &getProgressCounter() const { return *m_data.progressCounter; }
    NODISCARD QIODevice &getDevice() const;

public:
    NODISCARD bool canLoad() const { return virt_canLoad(); }
    NODISCARD bool canSave() const { return virt_canSave(); }
    NODISCARD std::optional<RawMapLoadData> loadData();
    NODISCARD bool saveData(const MapData &mapData, bool baseMapOnly);

private:
    NODISCARD virtual bool virt_canLoad() const = 0;
    NODISCARD virtual bool virt_canSave() const = 0;
    NODISCARD virtual std::optional<RawMapLoadData> virt_loadData() = 0;
    NODISCARD virtual bool virt_saveData(const MapLoadData &map) = 0;

signals:
    void sig_log(const QString &, const QString &);
};
