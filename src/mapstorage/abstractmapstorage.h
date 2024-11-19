#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "../global/progresscounter.h"
#include "../map/Map.h"
#include "../map/coordinate.h"
#include "../map/infomark.h"
#include "../mapdata/MarkerList.h"
#include "RawMapData.h"

#include <memory>
#include <optional>

#include <QObject>
#include <QString>
#include <QtCore>

class MapData;
class ProgressCounter;
class QFile;

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
        const std::shared_ptr<ProgressCounter> progressCounter;
        QString fileName;
        QFile *file = nullptr;

        explicit Data(std::shared_ptr<ProgressCounter> moved_pc, QString moved_name, QFile &f)
            : progressCounter(std::move(moved_pc))
            , fileName(std::move(moved_name))
            , file(&f)
        {
            if (!progressCounter) {
                throw std::invalid_argument("pc");
            }
        }
        explicit Data(std::shared_ptr<ProgressCounter> moved_pc, QString moved_name)
            : progressCounter(std::move(moved_pc))
            , fileName(std::move(moved_name))
        {
            if (!progressCounter) {
                throw std::invalid_argument("pc");
            }
        }
    };

private:
    Data m_data;

public:
    explicit AbstractMapStorage(const Data &data, QObject *parent);
    ~AbstractMapStorage() override;

protected:
    NODISCARD const QString &getFilename() const { return m_data.fileName; }
    NODISCARD ProgressCounter &getProgressCounter() const { return *m_data.progressCounter; }
    NODISCARD QFile *getFile() const
    {
        if (QFile *const pFile = m_data.file) {
            return pFile;
        }
        throw NullPointerException();
    }

public:
    NODISCARD bool canLoad() const { return virt_canLoad(); }
    NODISCARD bool canSave() const { return virt_canSave(); }
    NODISCARD std::optional<RawMapLoadData> loadData();
    NODISCARD bool saveData(const MapData &mapData, bool baseMapOnly);

private:
    NODISCARD virtual bool virt_canLoad() const = 0;
    NODISCARD virtual bool virt_canSave() const = 0;
    NODISCARD virtual std::optional<RawMapLoadData> virt_loadData() = 0;
    NODISCARD virtual bool virt_saveData(const RawMapData &map) = 0;

signals:
    void sig_log(const QString &, const QString &);
};
