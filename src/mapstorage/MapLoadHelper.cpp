// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "MapLoadHelper.h"

#include "../global/AnsiOstream.h"
#include "../global/AnsiTextUtils.h"
#include "../global/macros.h"
#include "../global/utils.h"
#include "../mapdata/mapdata.h"
#include "PandoraMapStorage.h"
#include "XmlMapStorage.h"
#include "mapstorage.h"

#include <array>
#include <stdexcept>
#include <utility>

#include <QXmlStreamReader>

namespace { // anonymous

NODISCARD bool detectMm2Binary(QIODevice &device)
{
    auto result = getMM2FileVersion(device);
    device.seek(0);
    return result.has_value();
}

// MMapper2 XML map (as opposed to Pandora XML map)
NODISCARD bool detectMm2Xml(QIODevice &device)
{
    const QByteArray line = device.readLine(64);
    const QByteArray line2 = device.readLine(64);
    device.seek(0);
    return line.contains("xml version") && line2.contains("mmapper2xml");
}

// Pandora XML map
NODISCARD bool detectPandora(QIODevice &device)
{
    QXmlStreamReader xml(&device);
    xml.readNextStartElement();
    if (xml.error() != QXmlStreamReader::NoError) {
        device.seek(0);
        return false;
    }
    if (xml.name() != QStringLiteral("map")) {
        device.seek(0);
        return false;
    }
    if (xml.attributes().isEmpty() || !xml.attributes().hasAttribute("rooms")) {
        device.seek(0);
        return false;
    }
    device.seek(0);
    return true;
}

template<typename T>
NODISCARD std::unique_ptr<AbstractMapStorage> make(const AbstractMapStorage::Data &data,
                                                   QObject *const parent)
{
    return std::make_unique<T>(data, parent);
}

class NODISCARD FileFormatHelper final
{
public:
    using DetectFn = bool (*)(QIODevice &);
    using MakeFn = std::unique_ptr<AbstractMapStorage> (*)(const AbstractMapStorage::Data &data,
                                                           QObject *parent);

private:
    DetectFn m_detect = nullptr;
    MakeFn m_make = nullptr;

public:
    explicit FileFormatHelper(const DetectFn d, const MakeFn m)
        : m_detect{d}
        , m_make{m}
    {
        assert(m_detect != nullptr);
        assert(m_make != nullptr);
        if (m_detect == nullptr || m_make == nullptr) {
            std::abort();
        }
    }

private:
    static void logException(const mm::source_location loc)
    {
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception &ex) {
            mm::WarningOstream{loc} << ex.what();
        } catch (...) {
            mm::WarningOstream{loc} << "Unknown exception.";
        }
    }

public:
    NODISCARD bool detect(QIODevice &device) const
    {
        try {
            return m_detect(device);
        } catch (...) {
            logException(MM_SOURCE_LOCATION());
            return false;
        }
    }
    NODISCARD std::unique_ptr<AbstractMapStorage> make(const AbstractMapStorage::Data &data,
                                                       QObject *const parent) const
    {
        try {
            return m_make(data, parent);
        } catch (...) {
            logException(MM_SOURCE_LOCATION());
            throw;
        }
    }
};

const std::array<FileFormatHelper, 3> formats{
    FileFormatHelper{&detectMm2Binary, &make<MapStorage>},
    FileFormatHelper{&detectMm2Xml, &make<XmlMapStorage>},
    FileFormatHelper{&detectPandora, &make<PandoraMapStorage>},
};

} // namespace

namespace maploadhelper {

std::unique_ptr<AbstractMapStorage> detectAndCreateStorage(const std::shared_ptr<MapSource> &pSource,
                                                           QObject *const parent)
{
    const AbstractMapStorage::Data data{pSource};
    auto &source = deref(pSource);
    auto pDevice = source.getIODevice();
    auto &device = deref(pDevice);
    for (const auto &fmt : formats) {
        if (!device.seek(0)) {
            throw std::runtime_error("Failed to seek to beginning.");
        }
        if (fmt.detect(device)) {
            return fmt.make(data, parent);
        }
    }
    throw std::runtime_error("Unrecognized file format");
}

std::optional<MapLoadData> loadMapData(AbstractMapStorage &storage)
{
    if (!storage.canLoad()) {
        return std::nullopt;
    }

    ProgressCounter &pc = storage.getProgressCounter();
    pc.setCurrentTask(ProgressMsg{/*"phase 1: "*/ "load from disk"});
    std::optional<RawMapLoadData> opt_data = storage.loadData();
    if (!opt_data) {
        return std::nullopt;
    }

    auto &data = opt_data.value();
    pc.reset();

    pc.setCurrentTask(ProgressMsg{/*"phase 2: "*/ "construct map from raw rooms and infomarks"});
    auto mapPair = Map::fromRooms(pc,
                                  std::exchange(data.rooms, {}),
                                  std::exchange(data.markers, {}));

    pc.setCurrentTask(ProgressMsg{"finished building map"});

    MapLoadData result;
    result.mapPair = std::exchange(mapPair, {});
    result.position = data.position;
    result.filename = data.filename;
    result.readonly = data.readonly;

    return result;
}

std::optional<Map> mergeMapData(AbstractMapStorage &storage, const Map &currentMap)
{
    if (!storage.canLoad()) {
        return std::nullopt;
    }

    ProgressCounter &pc = storage.getProgressCounter();
    pc.setCurrentTask(ProgressMsg{"phase 1: load from disk"});
    std::optional<RawMapLoadData> opt_data = storage.loadData();

    if (!opt_data || (opt_data->rooms.empty() && opt_data->markers.empty())) {
        return std::nullopt;
    }

    pc.setCurrentTask(ProgressMsg{"phase 2: merge the new map data"});
    return MapData::mergeMapData(pc, currentMap, opt_data.value());
}

} // namespace maploadhelper
