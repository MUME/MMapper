// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019-2024 The MMapper Authors

#include "../adventure/adventuretracker.h"
#include "../adventure/adventurewidget.h"
#include "../client/ClientWidget.h"
#include "../clock/mumeclock.h"
#include "../display/InfoMarkSelection.h"
#include "../display/MapCanvasData.h"
#include "../display/mapcanvas.h"
#include "../display/mapwindow.h"
#include "../global/AnsiOstream.h"
#include "../global/AnsiTextUtils.h"
#include "../global/macros.h"
#include "../logger/autologger.h"
#include "../mapstorage/MmpMapStorage.h"
#include "../mapstorage/PandoraMapStorage.h"
#include "../mapstorage/XmlMapStorage.h"
#include "../mapstorage/filesaver.h"
#include "../mapstorage/jsonmapstorage.h"
#include "../mapstorage/mapstorage.h"
#include "../pandoragroup/groupwidget.h"
#include "../pathmachine/mmapper2pathmachine.h"
#include "../preferences/configdialog.h"
#include "../proxy/connectionlistener.h"
#include "../roompanel/RoomManager.h"
#include "../roompanel/RoomWidget.h"
#include "UpdateDialog.h"
#include "findroomsdlg.h"
#include "mainwindow.h"
#include "utils.h"

#include <deque>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <QSize>
#include <QString>
#include <QtWidgets>

enum class NODISCARD CancelDispositionEnum : uint8_t { Forbid, Allow };
enum class NODISCARD PollResultEnum : uint8_t { Timeout, Finished };

namespace { // anonymous
namespace mwa_detail {

// MMapper2 XML map (as opposed to Pandora XML map)
NODISCARD bool detectMm2Xml(const QString &fileName)
{
    QFile f{fileName};
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray line = f.readLine(64);
    if (!line.contains("xml version")) {
        return false;
    }

    const QByteArray line2 = f.readLine(64);
    return line2.contains("mmapper2xml");
}

NODISCARD bool hasRooms(const RawMapLoadData &data)
{
    return !data.rooms.empty();
}

NODISCARD bool hasMarkers(const RawMapLoadData &data)
{
    return data.markerData && !data.markerData->empty();
}

// true if the map contains either rooms or markers;
// e.g. the map might ONLY contain markers.
NODISCARD bool hasValidData(const RawMapLoadData &data)
{
    return hasRooms(data) || hasMarkers(data);
}

template<typename T>
NODISCARD PollResultEnum wait_for(std::future<T> &future, const std::chrono::milliseconds ms)
{
    return future.wait_for(ms) == std::future_status::timeout ? PollResultEnum::Timeout
                                                              : PollResultEnum::Finished;
}

template<typename T>
NODISCARD std::optional<T> extract(std::future<std::optional<T>> &future, MainWindow &mainWindow)
{
    try {
        return future.get();
    } catch (const MapStorageError &ex) {
        QMessageBox::critical(&mainWindow,
                              MainWindow::tr("MapStorage Error"),
                              mmqt::toQStringUtf8(ex.what()));
    } catch (const std::exception &ex) {
        const auto msg = QString::asprintf("Exception: %s", ex.what());
        mainWindow.slot_log("AbstractMapStorage", msg);
        qWarning().noquote() << msg;
    }
    return std::nullopt;
}

} // namespace mwa_detail

namespace background {

NODISCARD static InfomarkDb getInformarkDb(ProgressCounter &pc,
                                           std::optional<RawMarkerData> optMarkers)
{
    InfomarkDb newMarks;
    if (optMarkers) {
        auto &markers = optMarkers.value().markers;
        pc.increaseTotalStepsBy(markers.size());
        for (const auto &mark : markers) {
            std::ignore = newMarks.addMarker(mark);
            pc.step();
        }
    }
    return newMarks;
}

NODISCARD std::optional<MapLoadData> load_map_data(ProgressCounter &pc, AbstractMapStorage &storage)
{
    if (!storage.canLoad()) {
        return std::nullopt;
    }

    pc.setCurrentTask(ProgressMsg{/*"phase 1: "*/ "load from disk"});
    std::optional<RawMapLoadData> opt_data = storage.loadData();
    if (!opt_data) {
        return std::nullopt;
    }

    auto &data = opt_data.value();
    pc.reset();

    pc.setCurrentTask(ProgressMsg{/*"phase 2: "*/ "construct map from raw rooms"});
    auto mapPair = Map::fromRooms(pc, std::exchange(data.rooms, {}));

    pc.setCurrentTask(ProgressMsg{/*"phase 3: "*/ "construct markers databse"});
    auto markerData = getInformarkDb(pc, std::exchange(data.markerData, {}));

    pc.setCurrentTask(ProgressMsg{"finished building map"});

    MapLoadData result;
    result.mapPair = std::exchange(mapPair, {});
    result.markerData = std::exchange(markerData, {});
    result.position = data.position;
    result.filename = data.filename;
    result.readonly = data.readonly;

    return result;
}

NODISCARD std::optional<std::pair<Map, InfomarkDb>> merge_map_data(ProgressCounter &pc,
                                                                   AbstractMapStorage &storage,
                                                                   const MapData &mapData)
{
    if (!storage.canLoad()) {
        return std::nullopt;
    }

    pc.setCurrentTask(ProgressMsg{"phase 1: load from disk"});
    std::optional<RawMapLoadData> opt_data = storage.loadData();

    if (!opt_data || !mwa_detail::hasValidData(*opt_data)) {
        return std::nullopt;
    }

    pc.setCurrentTask(ProgressMsg{"phase 2: merge the new map data"});
    // TODO: move ownership of the counter out of the storage object
    return MapData::mergeMapData(pc,
                                 mapData.getCurrentMap(),
                                 mapData.getCurrentMarks(),
                                 opt_data.value());
}

NODISCARD bool save(AbstractMapStorage &storage,
                    const MapData &mapData,
                    MainWindow::SaveModeEnum mode)
{
    if (!storage.canSave()) {
        return false;
    }
    return storage.saveData(mapData, mode == MainWindow::SaveModeEnum::BASEMAP);
}

} // namespace background
} // namespace

struct NODISCARD MainWindow::AsyncBase
{
public:
    const std::shared_ptr<ProgressCounter> progressCounter;

public:
    explicit AsyncBase(std::shared_ptr<ProgressCounter> pc)
        : progressCounter{std::move(pc)}
    {
        if (!progressCounter) {
            throw std::invalid_argument("pc");
        }
    }

public:
    virtual ~AsyncBase();

private:
    NODISCARD virtual PollResultEnum virt_poll(std::chrono::milliseconds ms) = 0;
    virtual void virt_request_cancel() = 0;

public:
    NODISCARD PollResultEnum poll(std::chrono::milliseconds ms) { return virt_poll(ms); }
    NODISCARD PollResultEnum poll() { return poll(std::chrono::milliseconds{0}); }
    void request_cancel();
    NODISCARD bool requested_cancel() const;
};

MainWindow::AsyncBase::~AsyncBase() = default;

void MainWindow::AsyncBase::request_cancel()
{
    progressCounter->requestCancel();
    virt_request_cancel();
}

bool MainWindow::AsyncBase::requested_cancel() const
{
    return progressCounter->requestedCancel();
}

MainWindow::AsyncTask::AsyncTask(QObject *parent)
    : QObject(parent)
{}

MainWindow::AsyncTask::~AsyncTask()
{
    if (isWorking()) {
        qWarning() << "Abandoning task in progress.";
        reset();
    }
}

void MainWindow::AsyncTask::begin(std::unique_ptr<AsyncBase> task)
{
    if (m_task) {
        throw std::runtime_error("already have an async task");
    }

    reset();

    m_task = std::move(task);
    QTimer &timer = m_timer.emplace(this);

    timer.setInterval(25);
    connect(&timer, &QTimer::timeout, this, &AsyncTask::tick);
    timer.start();

    qInfo() << "Async task started.";
}

void MainWindow::AsyncTask::tick()
{
    if (!isWorking()) {
        qWarning() << "tick called while not working";
        return;
    }

    if (m_task->poll() != PollResultEnum::Finished) {
        return;
    }

    reset();
    qInfo() << "Async task finished.";
}

void MainWindow::AsyncTask::request_cancel()
{
    m_task->request_cancel();
}

void MainWindow::AsyncTask::reset()
{
    m_task.reset();
    if (m_timer) {
        m_timer->stop();
        m_timer.reset();
    }
}

struct NODISCARD MainWindow::AsyncHelper : public AsyncBase
{
    using SharedFile = std::shared_ptr<QFile>;
    using UniqueStorage = std::unique_ptr<AbstractMapStorage>;

    struct NODISCARD ExtraBlockers final
    {
        ActionDisabler actionDisabler;
        // REVISIT: make this optional, so it's not done during map saving.
        CanvasHider canvasHider;
        MapFrontendBlocker blocker;

        explicit ExtraBlockers(MainWindow &mw, MapData &md)
            : actionDisabler{mw}
            , canvasHider{mw}
            , blocker{md}
        {}
    };

protected:
    MainWindow &mainWindow;

    const QString fileName;
    const SharedFile pFile;

    CanvasDisabler canvasDisabler;
    ProgressDialogLifetime progressDlg;
    UniqueStorage pStorage;

    std::unique_ptr<ExtraBlockers> extraBlockers;

private:
    ProgressMsg m_lastMsg;
    size_t m_lastPoll = 0;
    bool m_calledFinish = false;

public:
    explicit AsyncHelper(std::shared_ptr<ProgressCounter> pc,
                         MainWindow &mw,
                         const QString &name,
                         SharedFile pf,
                         UniqueStorage ps,
                         const QString &dialogText,
                         const CancelDispositionEnum allow_cancel)
        : AsyncBase{std::move(pc)}
        , mainWindow{mw}
        , fileName{name}
        , pFile(std::move(pf))
        , canvasDisabler{deref(mw.getCanvas())}
        , progressDlg{mw.createNewProgressDialog(dialogText,
                                                 allow_cancel == CancelDispositionEnum::Allow)}
        , pStorage{std::move(ps)}
        , extraBlockers{std::make_unique<ExtraBlockers>(mw, deref(mw.m_mapData))}
    {
        if (!name.isEmpty() && !pStorage) {
            throw std::invalid_argument("ps");
        }
    }

    ~AsyncHelper() override;

private:
    NODISCARD virtual PollResultEnum virt_wait(std::chrono::milliseconds ms) = 0;
    virtual void virt_finish() = 0;

    void virt_request_cancel() final { this->progressCounter->requestCancel(); }

private:
    void updateStatus()
    {
        auto &pc = *progressCounter;
        const ProgressCounter::Status status = pc.getStatus();
        const ProgressMsg &msg = status.msg;

        if (msg != m_lastMsg) {
            m_lastMsg = msg;
            mainWindow.slot_log("Async task", mmqt::toQStringUtf8(msg.getStdStringViewUtf8()));
            if (auto dlg = mainWindow.m_progressDlg.get()) {
                dlg->setLabelText(mmqt::toQStringUtf8(msg.getStdStringViewUtf8()) + "...");
            }
        }

        const size_t pct = status.percent();
        if (pct != m_lastPoll) {
            m_lastPoll = pct;
            mainWindow.percentageChanged(std::min(uint32_t(pct), 99u));
        }
    }

private:
    NODISCARD PollResultEnum virt_poll(std::chrono::milliseconds ms) final
    {
        if (m_calledFinish) {
            return PollResultEnum::Finished;
        }

        // update status before waiting
        updateStatus();

        const auto status = virt_wait(ms);

        // also update status again after waiting
        updateStatus();

        if (status == PollResultEnum::Timeout) {
            return PollResultEnum::Timeout;
        }

        progressDlg.reset();
        m_calledFinish = true;
        virt_finish();
        return PollResultEnum::Finished;
    }
};

MainWindow::AsyncHelper::~AsyncHelper()
{
    if (!m_calledFinish) {
        qWarning() << "Failed to call finish";
    }
}

struct NODISCARD MainWindow::AsyncLoader final : public MainWindow::AsyncHelper
{
private:
    using Result = std::optional<MapLoadData>;
    using Future = std::future<Result>;

private:
    Future future;

public:
    explicit AsyncLoader(std::shared_ptr<ProgressCounter> moved_pc,
                         MainWindow &mw,
                         const QString &name,
                         SharedFile pf,
                         UniqueStorage ps)
        : AsyncHelper{std::move(moved_pc),
                      mw,
                      name,
                      std::move(pf),
                      std::move(ps),
                      "Loading map...",
                      CancelDispositionEnum::Allow}
        , future{std::async(std::launch::async, &AsyncLoader::background_load, this)}
    {}
    ~AsyncLoader() final;

private:
    NODISCARD Result background_load() const
    {
        ProgressCounter &pc = deref(progressCounter);
        AbstractMapStorage &storage = deref(pStorage);
        return background::load_map_data(pc, storage);
    }

private:
    NODISCARD PollResultEnum virt_wait(const std::chrono::milliseconds ms) final
    {
        return mwa_detail::wait_for(future, ms);
    }

    void virt_finish() final
    {
        const Result result = mwa_detail::extract(future, mainWindow);

        // REVISIT: what if you just wanted to load markers?
        if (!result || result->mapPair.modified.getRoomsCount() == 0) {
            mainWindow.showAsyncFailure(fileName,
                                        AsyncTypeEnum::Load,
                                        progressCounter->requestedCancel());
            return;
        }

        // REVISIT: why are the extraBlockers reset after this?
        mainWindow.onSuccessfulLoad(*result);

        // NOTE: ~ExtraBlockers() calls ~CanvasHider(), which calls MapCanvas::show(),
        // which calls MapCanvas::paintGL(), which kicks off an async job to create the map
        // batches, so this should not be called before mapCanvas.slot_dataLoaded(),
        // since that function flags async map batches to be ignored. When that happens,
        // we have to build the meshes twice before they're displayed.
        extraBlockers.reset();
    }
};

MainWindow::AsyncLoader::~AsyncLoader() = default;

struct NODISCARD MainWindow::AsyncMerge final : public AsyncHelper
{
private:
    using Result = std::optional<std::pair<Map, InfomarkDb>>;
    using Future = std::future<Result>;

private:
    Future future;

public:
    explicit AsyncMerge(std::shared_ptr<ProgressCounter> pc,
                        MainWindow &mw,
                        const QString &name,
                        SharedFile pf,
                        UniqueStorage ps)
        : AsyncHelper{std::move(pc),
                      mw,
                      name,
                      std::move(pf),
                      std::move(ps),
                      "Merging map...",
                      CancelDispositionEnum::Allow}
        , future{std::async(std::launch::async,
                            &AsyncMerge::background_merge,
                            this,
                            std::ref(deref(mw.m_mapData)))}
    {}
    ~AsyncMerge() final;

private:
    NODISCARD Result background_merge(const MapData &mapData) const
    {
        ProgressCounter &pc = deref(progressCounter);
        AbstractMapStorage &storage = deref(pStorage);
        return background::merge_map_data(pc, storage, mapData);
    }

private:
    NODISCARD PollResultEnum virt_wait(const std::chrono::milliseconds ms) final
    {
        return mwa_detail::wait_for(future, ms);
    }

    void virt_finish() final
    {
        const Result result = mwa_detail::extract(future, mainWindow);
        if (!result) {
            mainWindow.showAsyncFailure(fileName,
                                        AsyncTypeEnum::Merge,
                                        progressCounter->requestedCancel());
            return;
        }

        extraBlockers.reset();
        mainWindow.onSuccessfulMerge(result->first, result->second);
    }
};

MainWindow::AsyncMerge::~AsyncMerge() = default;

struct NODISCARD MainWindow::AsyncSaver final : public AsyncHelper
{
public:
    using SharedFileSaver = std::shared_ptr<FileSaver>;

private:
    using Result = std::optional<bool>;
    using Future = std::future<Result>;

private:
    const SaveModeEnum mode;
    const SaveFormatEnum format;
    const SharedFileSaver pFilesSaver;
    Future future;

public:
    explicit AsyncSaver(std::shared_ptr<ProgressCounter> pc,
                        MainWindow &mw,
                        const QString &name,
                        SharedFileSaver pfs,
                        UniqueStorage ps,
                        const SaveModeEnum _mode,
                        const SaveFormatEnum _format)
        // REVISIT: The shared file outlives the saver. Is that okay?
        : AsyncHelper{std::move(pc),
                      mw,
                      name,
                      pfs->getSharedFile(),
                      std::move(ps),
                      "Saving map...",
                      CancelDispositionEnum::Forbid}
        , mode{_mode}
        , format{_format}
        , pFilesSaver(std::move(pfs))
        , future{std::async(std::launch::async, &AsyncSaver::background_save, this)}
    {
        if (!pFilesSaver) {
            throw std::invalid_argument("pfs");
        }
    }
    ~AsyncSaver() final;

private:
    NODISCARD Result background_save() const
    {
        AbstractMapStorage &storage = deref(pStorage);
        const MapData &mapData = deref(mainWindow.m_mapData);
        return background::save(storage, mapData, mode);
    }

private:
    NODISCARD PollResultEnum virt_wait(const std::chrono::milliseconds ms) final
    {
        return mwa_detail::wait_for(future, ms);
    }

    void virt_finish() final
    {
        const Result result = mwa_detail::extract(future, mainWindow);
        const bool success = result.has_value() && result.value();
        finish_saving(success);
    }

    void finish_saving(const bool success)
    {
        pFilesSaver->close();
        extraBlockers.reset();

        if (!success) {
            mainWindow.showAsyncFailure(fileName,
                                        AsyncTypeEnum::Save,
                                        progressCounter->requestedCancel());
            return;
        }

        mainWindow.onSuccessfulSave(mode, format, fileName);
    }
};

MainWindow::AsyncSaver::~AsyncSaver() = default;

std::unique_ptr<AbstractMapStorage> MainWindow::getLoadOrMergeMapStorage(
    const std::shared_ptr<ProgressCounter> &pc,
    const QString &fileName,
    std::shared_ptr<QFile> &pFile)
{
    auto tmp = [this, pc, &fileName, pFile]() -> std::unique_ptr<AbstractMapStorage> {
        auto &file = deref(pFile);
        const auto fileNameLower = fileName.toLower();

        const AbstractMapStorage::Data data{pc, fileName, file};
        if (fileNameLower.endsWith(".xml")) {
            if (mwa_detail::detectMm2Xml(fileName)) {
                // MMapper2 XML map
                return std::make_unique<XmlMapStorage>(data, this);
            } else {
                // Pandora map
                return std::make_unique<PandoraMapStorage>(data, this);
            }
        } else if (fileNameLower.endsWith(".mm2xml")) {
            // MMapper2 XML map
            return std::make_unique<XmlMapStorage>(data, this);
        } else {
            // MMapper2 binary map
            return std::make_unique<MapStorage>(data, this);
        }
    }();

    AbstractMapStorage *const pStorage = tmp.get();
    connect(pStorage, &AbstractMapStorage::sig_log, this, &MainWindow::slot_log);
    return tmp;
}

bool MainWindow::tryStartNewAsync()
{
    if (m_asyncTask) {
        showStatusShort(tr("Async operation already in progress"));
        return false;
    }
    return true;
}

void MainWindow::loadFile(const QString &fileName)
{
    if (!tryStartNewAsync()) {
        return;
    }

    if (fileName.isEmpty()) {
        showStatusShort(tr("No filename provided"));
        return;
    }

    auto pFile = std::make_shared<QFile>(fileName);
    auto &file = deref(pFile);
    if (!file.open(QFile::ReadOnly)) {
        showWarning(tr("Cannot read file %1:\n%2.").arg(fileName, file.errorString()));
        return;
    }

    // Immediately discard the old map.
    forceNewFile();

    auto pc = std::make_shared<ProgressCounter>();
    auto pStorage = getLoadOrMergeMapStorage(pc, fileName, pFile);

    m_asyncTask.begin(std::make_unique<AsyncLoader>(std::move(pc),
                                                    *this,
                                                    fileName,
                                                    std::move(pFile),
                                                    std::move(pStorage)));
}

void MainWindow::slot_merge()
{
    if (!tryStartNewAsync()) {
        return;
    }

    const QString fileName = chooseLoadOrMergeFileName();
    if (fileName.isEmpty()) {
        return;
    }

    auto pFile = std::make_shared<QFile>(fileName);
    auto &file = deref(pFile);
    if (!file.open(QFile::ReadOnly)) {
        showWarning(tr("Cannot read file %1:\n%2.").arg(fileName, file.errorString()));
        return;
    }

    auto pc = std::make_shared<ProgressCounter>();
    auto pStorage = getLoadOrMergeMapStorage(pc, fileName, pFile);
    connect(pStorage.get(), &AbstractMapStorage::sig_log, this, &MainWindow::slot_log);

    getCanvas()->slot_clearAllSelections();
    m_asyncTask.begin(std::make_unique<AsyncMerge>(std::move(pc),
                                                   *this,
                                                   fileName,
                                                   std::move(pFile),
                                                   std::move(pStorage)));
}

bool MainWindow::saveFile(const QString &fileName,
                          const SaveModeEnum mode,
                          const SaveFormatEnum format)
{
    if (!tryStartNewAsync()) {
        return false;
    }

    auto pFileSaver = std::make_shared<FileSaver>();
    // REVISIT: You can still test a directory for writing...
    if (format != SaveFormatEnum::WEB) { // Web uses a whole directory
        try {
            pFileSaver->open(fileName);
        } catch (const std::exception &e) {
            showWarning(tr("Cannot write file %1:\n%2.").arg(fileName, e.what()));
            return false;
        }
    }

    auto pc = std::make_shared<ProgressCounter>();

    auto pStorage =
        [this, pc, format, &fileName, pFileSaver]() -> std::unique_ptr<AbstractMapStorage> {
        auto storage =
            [this, pc, format, &fileName, pFileSaver]() -> std::unique_ptr<AbstractMapStorage> {
            AbstractMapStorage::Data data{pc, fileName, pFileSaver->getFile()};
            switch (format) {
            case SaveFormatEnum::MM2:
                return std::make_unique<MapStorage>(data, this);
            case SaveFormatEnum::MM2XML:
                return std::make_unique<XmlMapStorage>(data, this);
            case SaveFormatEnum::MMP:
                return std::make_unique<MmpMapStorage>(data, this);
            case SaveFormatEnum::WEB:
                // JsonMapStorage opens the filename and as a QDir, not a QFile.
                data.file = nullptr;
                return std::make_unique<JsonMapStorage>(data, this);
            }
            assert(false);
            return {};
        }();
        connect(storage.get(), &AbstractMapStorage::sig_log, this, &MainWindow::slot_log);
        return storage;
    }();

    if (!pStorage || !pStorage->canSave()) {
        showWarning(tr("Selected format cannot save."));
        return false;
    }

    m_asyncTask.begin(std::make_unique<AsyncSaver>(
        std::move(pc), *this, fileName, std::move(pFileSaver), std::move(pStorage), mode, format));
    return true;
}

bool MainWindow::slot_checkMapConsistency()
{
    if (!tryStartNewAsync()) {
        return false;
    }

    class NODISCARD AsyncCheckConsistency final : public AsyncHelper
    {
    public:
        using Result = std::optional<bool>;

    private:
        std::future<Result> future;

    public:
        explicit AsyncCheckConsistency(std::shared_ptr<ProgressCounter> pc, MainWindow &mw)
            : AsyncHelper{std::move(pc),
                          mw,
                          QString{},
                          SharedFile{},
                          UniqueStorage{},
                          "Checking map consistency...",
                          CancelDispositionEnum::Allow}
            , future{std::async(std::launch::async, &AsyncCheckConsistency::background_check, this)}
        {}
        ~AsyncCheckConsistency() final = default;

    private:
        NODISCARD Result background_check()
        {
            const MapData &mapData = deref(mainWindow.m_mapData);
            auto &pc = deref(progressCounter);
            mapData.getCurrentMap().checkConsistency(pc);
            return true;
        }

    private:
        PollResultEnum virt_wait(std::chrono::milliseconds ms) override
        {
            return mwa_detail::wait_for(future, ms);
        }
        void virt_finish() override
        {
            const Result result = mwa_detail::extract(future, mainWindow);
            const bool success = result.has_value() && result.value();
            if (success) {
                mainWindow.showWarning("Map is consistent.");
            } else {
                mainWindow.showWarning("ERROR: Failed map consistency check.");
            }
        }
    };

    auto pc = std::make_shared<ProgressCounter>();
    m_asyncTask.begin(std::make_unique<AsyncCheckConsistency>(std::move(pc), *this));
    return true;
}

bool MainWindow::slot_generateBaseMap()
{
    if (!tryStartNewAsync()) {
        return false;
    }

    static constexpr auto green = getRawAnsi(AnsiColor16Enum::green);
    static constexpr auto yellow = getRawAnsi(AnsiColor16Enum::yellow);

    class NODISCARD AsyncGenerateBaseMap final : public AsyncHelper
    {
    public:
        struct NODISCARD BaseMapData final
        {
            Map map;
            std::optional<RoomId> pNewRoom;
        };
        using Result = std::optional<BaseMapData>;

    private:
        std::future<Result> future;

    public:
        explicit AsyncGenerateBaseMap(std::shared_ptr<ProgressCounter> pc, MainWindow &mw)
            : AsyncHelper{std::move(pc),
                          mw,
                          QString{},
                          SharedFile{},
                          UniqueStorage{},
                          "Generating base map...",
                          CancelDispositionEnum::Allow}
            , future{std::async(std::launch::async,
                                &AsyncGenerateBaseMap::background_generate_base_map,
                                this,
                                mainWindow.m_mapData->getCurrentRoomId())}
        {}
        ~AsyncGenerateBaseMap() final = default;

    private:
        NODISCARD Result background_generate_base_map(std::optional<RoomId> pOldRoom)
        {
            using OptRoomId = std::optional<RoomId>;
            auto &pc = deref(progressCounter);
            MapData &mapData = deref(mainWindow.m_mapData);
            const auto oldMap = mapData.getCurrentMap();

            BaseMapData result;
            result.map = oldMap.filterBaseMap(pc);

            const auto &newMap = result.map;
            result.pNewRoom = [&pOldRoom, &oldMap, &newMap, &pc]() -> OptRoomId {
                if (!pOldRoom) {
                    return std::nullopt;
                }

                const RoomId oldStart = *pOldRoom;
                if (newMap.findRoomHandle(oldStart)) {
                    return pOldRoom;
                }

                pc.setNewTask(ProgressMsg{"Finding a new room"}, oldMap.getRoomsCount());
                RoomIdSet seen;
                std::deque<RoomId> todo;
                todo.push_back(oldStart);
                while (!todo.empty()) {
                    const RoomId maybe = utils::pop_front(todo);
                    if (seen.contains(maybe)) {
                        continue;
                    }
                    if (newMap.findRoomHandle(maybe)) {
                        return maybe;
                    }
                    seen.insert(maybe);
                    const auto oldr = oldMap.findRoomHandle(maybe);
                    for (const auto &ex : oldr.getExits()) {
                        for (const auto to : ex.outgoing) {
                            if (!seen.contains(to)) {
                                todo.push_back(to);
                            }
                        }
                    }
                    pc.step();
                }
                return std::nullopt;
            }();
            return result;
        }

    private:
        PollResultEnum virt_wait(std::chrono::milliseconds ms) override
        {
            return mwa_detail::wait_for(future, ms);
        }
        void virt_finish() override
        {
            const Result result = mwa_detail::extract(future, mainWindow);
            if (!result) {
                const bool wasCanceled = progressCounter->requestedCancel();
                const char *const msg = wasCanceled ? "User canceled generation of the base map"
                                                    : "Failed to generate the base map";
                mainWindow.showWarning(tr(msg));
                return;
            }
            onSuccess(std::move(*result));
        }
        void onSuccess(BaseMapData result)
        {
            auto &mapData = deref(mainWindow.m_mapData);

            const auto oldMap = mapData.getCurrentMap();
            const auto pOldRoom = mapData.getCurrentRoomId();
            const auto &newMap = result.map;
            const auto &pNewRoom = result.pNewRoom;

            std::ostringstream oss;
            AnsiOstream aos{oss};
            aos << "Base map generated (see below for details).\n";
            aos << "Old map: ";
            aos.writeWithColor(green, oldMap.getRoomsCount());
            aos << " room(s).\n";

            aos << "New map: ";
            aos.writeWithColor(green, newMap.getRoomsCount());
            aos << " room(s).\n";

            mapData.setCurrentMap(newMap);

            if (pNewRoom && pNewRoom != pOldRoom) {
                mapData.setRoom(*pNewRoom);
                aos << "Moved";
                if (pOldRoom) {
                    aos << " from ";
                    aos.writeQuotedWithColor(green,
                                             yellow,
                                             oldMap.findRoomHandle(*pOldRoom)
                                                 .getName()
                                                 .getStdStringViewUtf8());
                }
                aos << " to ";
                aos.writeQuotedWithColor(green,
                                         yellow,
                                         newMap.findRoomHandle(*pNewRoom)
                                             .getName()
                                             .getStdStringViewUtf8());
                aos << ".\n";
            }

            qInfo().noquote() << mmqt::toQStringUtf8(oss.str());
        }
    };

    auto pc = std::make_shared<ProgressCounter>();
    m_asyncTask.begin(std::make_unique<AsyncGenerateBaseMap>(std::move(pc), *this));
    return true;
}
