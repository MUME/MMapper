// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019-2024 The MMapper Authors

#include "mainwindow-async.h"

#include "../client/ClientWidget.h"
#include "../display/MapCanvasData.h"
#include "../display/mapcanvas.h"
#include "../display/mapwindow.h"
#include "../global/AnsiOstream.h"
#include "../global/AnsiTextUtils.h"
#include "../global/AsyncTasks.h"
#include "../global/macros.h"
#include "../global/thread_utils.h"
#include "../global/utils.h"
#include "../mapstorage/MapDestination.h"
#include "../mapstorage/MmpMapStorage.h"
#include "../mapstorage/PandoraMapStorage.h"
#include "../mapstorage/XmlMapStorage.h"
#include "../mapstorage/jsonmapstorage.h"
#include "../mapstorage/mapstorage.h"
#include "../pathmachine/mmapper2pathmachine.h"
#include "findroomsdlg.h"
#include "mainwindow.h"
#include "utils.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>

#include <QString>
#include <QXmlStreamReader>
#include <QtWidgets>

namespace { // anonymous

enum class NODISCARD PollResultEnum : uint8_t { Timeout, Finished };

constexpr auto green = getRawAnsi(AnsiColor16Enum::green);
constexpr auto yellow = getRawAnsi(AnsiColor16Enum::yellow);

namespace mwa_detail {

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
                                                   MainWindow *const mw)
{
    return std::make_unique<T>(data, mw);
}

class NODISCARD FileFormatHelper final
{
public:
    using DetectFn = bool (*)(QIODevice &);
    using MakeFn = std::unique_ptr<AbstractMapStorage> (*)(const AbstractMapStorage::Data &data,
                                                           MainWindow *mw);

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
                                                       MainWindow *mw) const
    {
        try {
            return m_make(data, mw);
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

NODISCARD bool hasRooms(const RawMapLoadData &data)
{
    return !data.rooms.empty();
}

NODISCARD bool hasMarkers(const RawMapLoadData &data)
{
    return !data.markers.empty();
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
    } catch (const ProgressCanceledException &) {
        const auto msg = "IO operation canceled.";
        mainWindow.slot_log("AbstractMapStorage", msg);
        qInfo().noquote() << msg;
    } catch (const std::exception &ex) {
        const auto msg = QString::asprintf("Exception: %s", ex.what());
        mainWindow.slot_log("AbstractMapStorage", msg);
        qWarning().noquote() << msg;
    }
    return std::nullopt;
}

} // namespace mwa_detail

namespace background {

NODISCARD std::optional<MapLoadData> load_map_data(AbstractMapStorage &storage)
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

NODISCARD std::optional<Map> merge_map_data(AbstractMapStorage &storage, const MapData &mapData)
{
    if (!storage.canLoad()) {
        return std::nullopt;
    }

    ProgressCounter &pc = storage.getProgressCounter();
    pc.setCurrentTask(ProgressMsg{"phase 1: load from disk"});
    std::optional<RawMapLoadData> opt_data = storage.loadData();

    if (!opt_data || !mwa_detail::hasValidData(*opt_data)) {
        return std::nullopt;
    }

    pc.setCurrentTask(ProgressMsg{"phase 2: merge the new map data"});
    // TODO: move ownership of the counter out of the storage object
    return MapData::mergeMapData(pc, mapData.getCurrentMap(), opt_data.value());
}

NODISCARD bool save(AbstractMapStorage &storage, const MapData &mapData, const SaveModeEnum mode)
{
    if (!storage.canSave()) {
        return false;
    }
    return storage.saveData(mapData, mode == SaveModeEnum::BASEMAP);
}

} // namespace background
} // namespace

struct NODISCARD MainWindow::AsyncBase
{
    using SharedDevice = std::shared_ptr<QIODevice>;
    using UniqueStorage = std::unique_ptr<AbstractMapStorage>;
    using SharedPc = std::shared_ptr<ProgressCounter>;

public:
    const QString taskName;

protected:
    MainWindow &m_mainWindow;
    const QString m_fileName;
    const SharedDevice m_pDevice;
    UniqueStorage m_pStorage;
    bool m_isFinished = false;

public:
    explicit AsyncBase(QString moved_taskName,
                       MainWindow &mw,
                       const QString &name,
                       SharedDevice pd,
                       UniqueStorage ps)
        : taskName{std::move(moved_taskName)}
        , m_mainWindow{mw}
        , m_fileName{name}
        , m_pDevice(std::move(pd))
        , m_pStorage{std::move(ps)}
    {
        if (!name.isEmpty() && !m_pStorage) {
            throw std::invalid_argument("ps");
        }
    }

    virtual ~AsyncBase();

public:
    void background_worker(const SharedPc &sharedPc) { virt_background_worker(sharedPc); }
    void on_success(const SharedPc &sharedPc)
    {
        virt_finish(sharedPc);
        m_isFinished = true;
    }

private:
    virtual void virt_background_worker(const SharedPc &sharedPc) = 0;
    virtual void virt_finish(const SharedPc &sharedPc) = 0;
    NODISCARD virtual PollResultEnum virt_wait(std::chrono::milliseconds ms) = 0;

public:
    NODISCARD PollResultEnum poll() const
    {
        return m_isFinished ? PollResultEnum::Finished : PollResultEnum::Timeout;
    }
};

MainWindow::AsyncBase::~AsyncBase()
{
    if (!m_isFinished) {
        qWarning() << "Failed to call finish for task " << taskName;
    }
    // FIXME: how can mainWindow be expired here?
    m_mainWindow.asyncTaskEnded(taskName);
}

MainWindow::AsyncIO::AsyncIO(MainWindow *mainWindow)
    : QObject(mainWindow)
    , m_mainWindow(deref(mainWindow))
{}

MainWindow::AsyncIO::~AsyncIO()
{
    if (isRunningOnBackgroundThread()) {
        qWarning() << "Abandoning task" << *this << " in progress.";
        reset();
    }
}

bool MainWindow::AsyncIO::isRunningOnBackgroundThread() const
{
    if (hasCurrentTask()) {
        auto &task = getTask();
        return task.isRunningOnBackgroundThread();
    }

    return false;
}

void MainWindow::AsyncIO::beginAsyncIO(std::unique_ptr<AsyncBase> unique_task,
                                       const QString &progressDialogText,
                                       const AsyncIOTypeEnum type)
{
    if (!unique_task) {
        throw std::invalid_argument("task");
    }

    if (hasCurrentTask()) {
        throw std::runtime_error("already have an async task");
    }

    if (m_closedForBusiness) {
        throw std::runtime_error("mmapper is shutting down");
    }

    const bool isSave = type == AsyncIOTypeEnum::Save;
    const AllowCancelEnum allowCancel = !isSave ? AllowCancelEnum::Allow : AllowCancelEnum::Forbid;
    const bool preventMapChanges = !isSave;

    reset();
    {
        const auto shared_io_task = std::shared_ptr(std::move(unique_task));
        const auto weak_io_task = std::weak_ptr(shared_io_task);
        auto &io_task = deref(shared_io_task);
        m_asyncData.emplace(shared_io_task,
                            async_tasks::startAsyncTask2(
                                AsyncTaskTypeEnum::IO,
                                allowCancel,
                                mmqt::toStdStringUtf8(io_task.taskName),
                                [weak_io_task](const std::shared_ptr<ProgressCounter> &pc) {
                                    // background thread
                                    assert(!thread_utils::isOnMainThread());
                                    const auto shared = weak_io_task.lock();
                                    AsyncBase &task = deref(shared);
                                    task.background_worker(pc);
                                },
                                [weak_io_task](const std::shared_ptr<ProgressCounter> &pc) {
                                    ABORT_IF_NOT_ON_MAIN_THREAD();
                                    const auto shared = weak_io_task.lock();
                                    AsyncBase &task = deref(shared);
                                    task.on_success(pc);
                                }));
    }

    // FIXME: This assumes "Save" is the only thing that forbids cancel.
    if (preventMapChanges) {
        m_canvasDisabler.emplace(deref(m_mainWindow.m_mapWindow));
    }
    createNewProgressDialog(progressDialogText, allowCancel == AllowCancelEnum::Allow);
    if (preventMapChanges) {
        m_extraBlockers.emplace(m_mainWindow, deref(m_mainWindow.m_mapData));
    }

    {
        ProgressData &progress_data = m_progressData.emplace();
        QTimer &timer = progress_data.timer;
        timer.setInterval(25);
        connect(&timer, &QTimer::timeout, this, &AsyncIO::tick);
        timer.start();
    }

    qInfo() << "Async task" << *this << "started.";
}

void MainWindow::AsyncIO::updateStatus()
{
    auto &pc = getProgressCounter();
    const ProgressCounter::Status status = pc.getStatus();
    auto &progresssData = deref(m_progressData);
    auto &lastMsg = progresssData.lastMsg;
    auto &lastPoll = progresssData.lastPoll;

    if (const ProgressMsg &msg = status.msg; msg != lastMsg) {
        lastMsg = msg;
        m_mainWindow.slot_log("Async task", mmqt::toQStringUtf8(msg.getStdStringViewUtf8()));
        if (auto dlg = m_mainWindow.getAsyncIO().getProgressDlg()) {
            dlg->setLabelText(mmqt::toQStringUtf8(msg.getStdStringViewUtf8()) + "...");
        }
    }

    if (const size_t pct = status.percent(); pct != lastPoll) {
        lastPoll = pct;
        m_mainWindow.getAsyncIO().percentageChanged(std::min(uint32_t(pct), 99u));
    }
}

void MainWindow::AsyncIO::tick()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();

    if (!hasCurrentTask()) {
        qWarning() << "tick called with no current task";
        return;
    }

    auto &data = deref(m_asyncData);
    auto &io = deref(data.asyncIo);
    if (io.poll() != PollResultEnum::Finished) {
        updateStatus();
    } else {
        reset();
        qInfo() << "Async task finished.";
    }
}

void MainWindow::AsyncIO::request_cancel()
{
    getProgressCounter().requestCancel();
}

bool MainWindow::AsyncIO::is_allowed_to_cancel() const
{
    return getProgressCounter().allowCancel() == AllowCancelEnum::Allow;
}

QDebug &operator<<(QDebug &debug, const MainWindow::AsyncIO &task)
{
    const auto str = task.getTask().getStatusAsPlaintext();
    return debug << mmqt::toQStringUtf8(str);
}

NODISCARD size_t MainWindow::AsyncIO::getTaskId() const
{
    return getTask().getId();
}
NODISCARD const std::string &MainWindow::AsyncIO::getTaskName() const
{
    return getTask().getName();
}
NODISCARD QString MainWindow::AsyncIO::getTaskNameAsQString() const
{
    return mmqt::toQStringUtf8(getTaskName());
}
NODISCARD std::shared_ptr<ProgressCounter> MainWindow::AsyncIO::getSharedProgressCounter() const
{
    return getTask().getSharedProgressCounter();
}
NODISCARD ProgressCounter &MainWindow::AsyncIO::getProgressCounter() const
{
    const auto shared = getSharedProgressCounter();
    return deref(shared);
}

void MainWindow::AsyncIO::resetExtraBlockers()
{
    // NOTE: ~ExtraBlockers() calls ~CanvasHider(), which calls MapCanvas::show(),
    // which calls MapCanvas::paintGL(), which kicks off an async job to create the map
    // batches, so this should not be called before mapCanvas.slot_dataLoaded(),
    // since that function flags async map batches to be ignored. When that happens,
    // we have to build the meshes twice before they're displayed.
    m_extraBlockers.reset();
}

void MainWindow::AsyncIO::reset()
{
    m_progressDlg.reset();
    m_progressData.reset();
    m_asyncData.reset();
    resetExtraBlockers();
    assert(!hasCurrentTask());
}

void MainWindow::asyncTaskEnded(const QString &taskName)
{
    if (getAsyncIO().isWaitingForSaveAtShutdown()) {
        qInfo() << "async task" << taskName << "ended; closing mmapper.";
        show();
        close();
    }
}

struct NODISCARD MainWindow::AsyncLoader final : public AsyncBase
{
private:
    using Result = std::optional<MapLoadData>;
    using Promise = std::promise<Result>;
    using Future = std::future<Result>;

private:
    Promise m_promise;
    Future m_future = m_promise.get_future();

public:
    explicit AsyncLoader(MainWindow &mw, const QString &name, SharedDevice pd, UniqueStorage ps)
        : AsyncBase{"Map Loader", mw, name, std::move(pd), std::move(ps)}
    {}
    ~AsyncLoader() final;

private:
    void virt_background_worker(const std::shared_ptr<ProgressCounter> &sharedPc) final
    {
        try {
            m_promise.set_value(background_load(sharedPc));
        } catch (...) {
            m_promise.set_exception(std::current_exception());
        }
    }

private:
    NODISCARD Result background_load(const std::shared_ptr<ProgressCounter> &sharedPc) const
    {
        AbstractMapStorage &storage = deref(m_pStorage);
        storage.setProgressCounter(sharedPc);
        return background::load_map_data(storage);
    }

private:
    NODISCARD PollResultEnum virt_wait(const std::chrono::milliseconds ms) final
    {
        return mwa_detail::wait_for(m_future, ms);
    }

    void virt_finish(const std::shared_ptr<ProgressCounter> &sharedPc) final
    {
        const bool wasCanceled = deref(sharedPc).hasRequestedCancel();
        const Result result = mwa_detail::extract(m_future, m_mainWindow);

        // REVISIT: what if you just wanted to load markers?
        if (wasCanceled || !result || result->mapPair.modified.getRoomsCount() == 0) {
            m_mainWindow.showAsyncFailure(m_fileName, AsyncIOTypeEnum::Load, wasCanceled);
            return;
        }

        // REVISIT: why are the extraBlockers reset after this?
        m_mainWindow.onSuccessfulLoad(*result);

        m_mainWindow.getAsyncIO().resetExtraBlockers();
    }
};

MainWindow::AsyncLoader::~AsyncLoader() = default;

struct NODISCARD MainWindow::AsyncMerge final : public AsyncBase
{
private:
    using Result = std::optional<Map>;

    using Promise = std::promise<Result>;
    using Future = std::future<Result>;

private:
    MapData &m_mapdata;
    Promise m_promise;
    Future m_future = m_promise.get_future();

public:
    explicit AsyncMerge(MainWindow &mw, const QString &name, SharedDevice pd, UniqueStorage ps)
        : AsyncBase{"Map Merge", mw, name, std::move(pd), std::move(ps)}
        , m_mapdata{deref(mw.m_mapData)}
    {}
    ~AsyncMerge() final;

private:
    void virt_background_worker(const std::shared_ptr<ProgressCounter> &sharedPc) override
    {
        try {
            m_promise.set_value(background_merge(sharedPc, m_mapdata));
        } catch (...) {
            m_promise.set_exception(std::current_exception());
        }
    }

private:
    NODISCARD Result background_merge(const std::shared_ptr<ProgressCounter> &sharedPc,
                                      const MapData &mapData) const
    {
        AbstractMapStorage &storage = deref(m_pStorage);
        storage.setProgressCounter(sharedPc);
        return background::merge_map_data(storage, mapData);
    }

private:
    NODISCARD PollResultEnum virt_wait(const std::chrono::milliseconds ms) final
    {
        return mwa_detail::wait_for(m_future, ms);
    }

    void virt_finish(const std::shared_ptr<ProgressCounter> &sharedPc) final
    {
        const bool wasCanceled = deref(sharedPc).hasRequestedCancel();
        const Result result = mwa_detail::extract(m_future, m_mainWindow);
        if (wasCanceled || !result) {
            m_mainWindow.showAsyncFailure(m_fileName, AsyncIOTypeEnum::Merge, wasCanceled);
            return;
        }

        m_mainWindow.getAsyncIO().resetExtraBlockers();
        m_mainWindow.onSuccessfulMerge(*result);
    }
};

MainWindow::AsyncMerge::~AsyncMerge() = default;

struct NODISCARD MainWindow::AsyncSaver final : public AsyncBase
{
public:
    using SharedMapDestination = std::shared_ptr<MapDestination>;

private:
    using Result = std::optional<bool>;
    using Promise = std::promise<Result>;
    using Future = std::future<Result>;

private:
    const SaveModeEnum mode;
    const SaveFormatEnum format;
    const SharedMapDestination pMapDestination;

    Promise m_promise;
    Future m_future = m_promise.get_future();

public:
    explicit AsyncSaver(MainWindow &mw,
                        SharedMapDestination pDest,
                        UniqueStorage ps,
                        const SaveModeEnum _mode,
                        const SaveFormatEnum _format)
        : AsyncBase{"Map Saver", mw, pDest->getFileName(), pDest->getIODevice(), std::move(ps)}
        , mode{_mode}
        , format{_format}
        , pMapDestination(std::move(pDest))
    {
        if (!pMapDestination) {
            throw std::invalid_argument("pDest cannot be null");
        }
    }
    ~AsyncSaver() final;

private:
    void virt_background_worker(const std::shared_ptr<ProgressCounter> &sharedPc) final
    {
        try {
            m_promise.set_value(background_save(sharedPc));
        } catch (...) {
            m_promise.set_exception(std::current_exception());
        }
    }

private:
    NODISCARD Result background_save(const std::shared_ptr<ProgressCounter> &sharedPc) const
    {
        AbstractMapStorage &storage = deref(m_pStorage);
        storage.setProgressCounter(sharedPc);
        const MapData &mapData = deref(m_mainWindow.m_mapData);
        return background::save(storage, mapData, mode);
    }

private:
    NODISCARD PollResultEnum virt_wait(const std::chrono::milliseconds ms) final
    {
        return mwa_detail::wait_for(m_future, ms);
    }

    void virt_finish(const std::shared_ptr<ProgressCounter> &sharedPc) final
    {
        assert(deref(sharedPc).allowCancel() == AllowCancelEnum::Forbid);
        assert(deref(sharedPc).hasRequestedCancel() == false);

        const Result result = mwa_detail::extract(m_future, m_mainWindow);
        const bool success = result.has_value() && result.value();
        finish_saving(success);
    }

    void finish_saving(const bool success)
    {
        pMapDestination->finalize();
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
            if (success) {
                assert(pMapDestination->isFileWasm());
                QFileDialog::saveFileContent(pMapDestination->getWasmBufferData(),
                                             pMapDestination->getFileName());
            }
        }

        m_mainWindow.getAsyncIO().resetExtraBlockers();

        if (!success) {
            m_mainWindow.showAsyncFailure(m_fileName, AsyncIOTypeEnum::Save, false);
            return;
        }

        m_mainWindow.onSuccessfulSave(mode, format, m_fileName);
    }
};

MainWindow::AsyncSaver::~AsyncSaver() = default;

std::unique_ptr<AbstractMapStorage> MainWindow::getLoadOrMergeMapStorage(
    std::shared_ptr<MapSource> &pSource)
{
    auto tmp = std::invoke([this, &pSource]() -> std::unique_ptr<AbstractMapStorage> {
        const AbstractMapStorage::Data data{pSource};
        auto &source = deref(pSource);
        auto pDevice = source.getIODevice();
        auto &device = deref(pDevice);
        for (const auto &fmt : mwa_detail::formats) {
            if (!device.seek(0)) {
                throw std::runtime_error("Failed to seek to beginning.");
            }
            if (fmt.detect(device)) {
                return fmt.make(data, this);
            }
        }
        throw std::runtime_error("Unrecognized file format");
    });

    AbstractMapStorage *const pStorage = tmp.get();
    connect(pStorage, &AbstractMapStorage::sig_log, this, &MainWindow::slot_log);
    return tmp;
}

bool MainWindow::tryStartNewAsync()
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    if (getAsyncIO().isRunningOnBackgroundThread()) {
        showStatusShort(tr("IO Task already in progress"));
        return false;
    }
    return true;
}

void MainWindow::loadFile(std::shared_ptr<MapSource> pSource)
{
    auto &source = deref(pSource);
    try {
        if (!tryStartNewAsync()) {
            return;
        }

        if (source.getFileName().isEmpty()) {
            showStatusShort(tr("No filename provided"));
            return;
        }

        // Immediately discard the old map.
        forceNewFile();

        auto pStorage = getLoadOrMergeMapStorage(pSource);

        getAsyncIO().beginAsyncIO(std::make_unique<AsyncLoader>(*this,
                                                                source.getFileName(),
                                                                source.getIODevice(),
                                                                std::move(pStorage)),
                                  "Loading map...",
                                  AsyncIOTypeEnum::Load);
    } catch (const std::exception &ex) {
        showWarning(tr("Cannot open file %1:\n%2.").arg(source.getFileName(), ex.what()));
        return;
    } catch (...) {
        showWarning(tr("Cannot open file %1:\n%2.").arg(source.getFileName(), "Unknown exception"));
        return;
    }
}

void MainWindow::slot_merge()
{
    if (!tryStartNewAsync()) {
        return;
    }

    auto mergeFile = [this](const QString &fileName, const std::optional<QByteArray> &fileContent) {
        if (fileName.isEmpty()) {
            showStatusShort(tr("No filename provided"));
            return;
        }

        try {
            auto pSource = MapSource::alloc(fileName, fileContent);
            auto &source = deref(pSource);
            auto pStorage = getLoadOrMergeMapStorage(pSource);
            connect(pStorage.get(), &AbstractMapStorage::sig_log, this, &MainWindow::slot_log);

            getCanvas()->slot_clearAllSelections();
            getAsyncIO().beginAsyncIO(std::make_unique<AsyncMerge>(*this,
                                                                   source.getFileName(),
                                                                   source.getIODevice(),
                                                                   std::move(pStorage)),
                                      "Merging map...",
                                      AsyncIOTypeEnum::Merge);
        } catch (const std::runtime_error &ex) {
            showWarning(tr("Cannot open file %1:\n%2.").arg(fileName, ex.what()));
            return;
        } catch (...) {
            showWarning(tr("Cannot open file %1:\n%2.").arg(fileName, "Unknown exception"));
            return;
        }
    };

    const auto nameFilter = QStringLiteral("MMapper2 maps (*.mm2)"
                                           ";;MMapper2 XML or Pandora maps (*.xml)"
                                           ";;Alternate suffix for MMapper2 XML maps (*.mm2xml)");
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        QFileDialog::getOpenFileContent(nameFilter, mergeFile);
    } else {
        const QString &savedLastMapDir = setConfig().autoLoad.lastMapDirectory;
        const QString fileName = QFileDialog::getOpenFileName(this,
                                                              "Choose map file ...",
                                                              savedLastMapDir,
                                                              nameFilter);
        mergeFile(fileName, std::nullopt);
    }
}

bool MainWindow::saveFile(const QString &fileName,
                          const SaveModeEnum mode,
                          const SaveFormatEnum format)
{
    if (!tryStartNewAsync()) {
        return false;
    }

    std::shared_ptr<MapDestination> pDest = nullptr;
    try {
        pDest = MapDestination::alloc(fileName, format);
    } catch (const std::exception &e) {
        showWarning(tr("Cannot set up save destination %1:\n%2.").arg(fileName, e.what()));
        return false;
    }

    auto pStorage = std::invoke([this, format, pDest]() -> std::unique_ptr<AbstractMapStorage> {
        auto storage = std::invoke([this, format, pDest]() -> std::unique_ptr<AbstractMapStorage> {
            AbstractMapStorage::Data data{pDest};
            switch (format) {
            case SaveFormatEnum::MM2:
                return std::make_unique<MapStorage>(data, this);
            case SaveFormatEnum::MM2XML:
                return std::make_unique<XmlMapStorage>(data, this);
            case SaveFormatEnum::MMP:
                return std::make_unique<MmpMapStorage>(data, this);
            case SaveFormatEnum::WEB:
                return std::make_unique<JsonMapStorage>(data, this);
            }
            assert(false);
            return {};
        });
        connect(storage.get(), &AbstractMapStorage::sig_log, this, &MainWindow::slot_log);
        return storage;
    });

    if (!pStorage || !pStorage->canSave()) {
        showWarning(tr("Selected format cannot save."));
        return false;
    }

    getAsyncIO()
        .beginAsyncIO(std::make_unique<AsyncSaver>(*this, pDest, std::move(pStorage), mode, format),
                      "Saving map...",
                      AsyncIOTypeEnum::Save);
    return true;
}

bool MainWindow::slot_generateBaseMap()
{
    if (!tryStartNewAsync()) {
        return false;
    }

    class NODISCARD AsyncGenerateBaseMap final : public AsyncBase
    {
    public:
        struct NODISCARD BaseMapData final
        {
            Map map;
            std::optional<RoomId> pNewRoom;
        };
        using Result = std::optional<BaseMapData>;
        using Promise = std::promise<Result>;
        using Future = std::future<Result>;

    private:
        std::optional<RoomId> m_currentRoom;
        Promise m_promise;
        Future m_future = m_promise.get_future();

    public:
        explicit AsyncGenerateBaseMap(MainWindow &mw)
            : AsyncBase{"Generate Base Map", mw, QString{}, SharedDevice{}, UniqueStorage{}}
            , m_currentRoom{mw.m_mapData->getCurrentRoomId()}
        {}
        ~AsyncGenerateBaseMap() final = default;

    private:
        void virt_background_worker(const std::shared_ptr<ProgressCounter> &sharedPc) final
        {
            try {
                m_promise.set_value(background_generate_base_map(sharedPc, m_currentRoom));
            } catch (...) {
                m_promise.set_exception(std::current_exception());
            }
        }

    private:
        // Walks the old map looking for the nearest room that's also in the new map.
        // Assumes that newMap is a filtered version of oldMap.
        NODISCARD static auto findNearestCommonRoom(ProgressCounter &pc,
                                                    const Map &oldMap,
                                                    const std::optional<RoomId> pOldRoom,
                                                    const Map &newMap) -> std::optional<RoomId>
        {
            if (!pOldRoom) {
                return std::nullopt;
            }

            const RoomId oldStart = *pOldRoom;
            if (newMap.findRoomHandle(oldStart)) {
                return pOldRoom;
            }

            // revisit: does this mean new start room?
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
                    for (const RoomId to : ex.outgoing) {
                        if (!seen.contains(to)) {
                            todo.push_back(to);
                        }
                    }
                }
                pc.step();
            }
            return std::nullopt;
        }

        NODISCARD Result background_generate_base_map(
            const std::shared_ptr<ProgressCounter> &sharedPc, const std::optional<RoomId> pOldRoom)
        {
            auto &pc = deref(sharedPc);
            MapData &mapData = deref(m_mainWindow.m_mapData);
            const auto oldMap = mapData.getCurrentMap();

            BaseMapData result;
            result.map = oldMap.filterBaseMap(pc);
            result.pNewRoom = findNearestCommonRoom(pc, oldMap, pOldRoom, result.map);
            return result;
        }

    private:
        PollResultEnum virt_wait(const std::chrono::milliseconds ms) override
        {
            return mwa_detail::wait_for(m_future, ms);
        }
        void virt_finish(const std::shared_ptr<ProgressCounter> &sharedPc) override
        {
            const bool wasCanceled = deref(sharedPc).hasRequestedCancel();
            Result result = mwa_detail::extract(m_future, m_mainWindow);
            if (wasCanceled || !result) {
                const char *const msg = wasCanceled ? "User canceled generation of the base map"
                                                    : "Failed to generate the base map";
                m_mainWindow.showWarning(tr(msg));
                return;
            }
            onSuccess(result.value());
        }
        void onSuccess(const BaseMapData &result)
        {
            auto &mapData = deref(m_mainWindow.m_mapData);

            const auto oldMap = mapData.getCurrentMap();
            const auto pOldRoom = mapData.getCurrentRoomId();
            auto &newMap = result.map;
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

    getAsyncIO().beginAsyncIO(std::make_unique<AsyncGenerateBaseMap>(*this),
                              "Generating base map...",
                              // REVISIT: This is like a load, but it's not loading from disk.
                              // Consider making this a regular non-IO async "change" command?
                              AsyncIOTypeEnum::Load);
    return true;
}

void MainWindow::AsyncIO::createNewProgressDialog(const QString &text, const bool allow_cancel)
{
    auto &mainWindow = getMainWindow();
    auto progressDlg = std::make_unique<QProgressDialog>(&mainWindow);
    QProgressDialog &progress = deref(progressDlg);

    if (allow_cancel) {
        progress.setCancelButtonText("Cancel");
    } else {
        auto *const cb = new QPushButton("Cancel", &progress);
        deref(cb).setEnabled(false);
        progress.setCancelButton(cb);
    }

    progress.setWindowTitle(text);
    progress.setMinimum(0);
    progress.setMaximum(100);
    progress.setValue(0);
    progress.setMinimumWidth(mainWindow.width() / 4);
    progress.showNormal();
    progress.raise();
    progress.activateWindow();
#if 0
    // REVISIT: This was probably supposed to be progress.setFocus(),
    // since focusWidget() does not "do" anything. It just returns a value.
    focusWidget();
#endif

    connect(&progress, &QProgressDialog::canceled, this, [this, allow_cancel]() {
        if (allow_cancel) {
            qInfo() << "QProgressDialog::canceled()";
            this->request_cancel();
        }
    });
    connect(&progress, &QProgressDialog::rejected, this, [this, allow_cancel]() {
        if (allow_cancel) {
            qInfo() << "QProgressDialog::rejected()";
            this->request_cancel();
        }
    });

    m_progressDlg = std::move(progressDlg);
}
