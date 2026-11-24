// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Kalev Lember <kalev@smartlink.ee> (Kalev)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "findroomsdlg.h"

#include "../configuration/configuration.h"
#include "../global/AsyncTasks.h"
#include "../global/parserutils.h"
#include "../global/progresscounter.h"
#include "../global/window_utils.h"
#include "../map/ExitDirection.h"
#include "../map/coordinate.h"
#include "../map/roomid.h"
#include "../mapdata/GenericFind.h"
#include "../mapdata/mapdata.h"
#include "../mapdata/roomfilter.h"
#include "../mapdata/roomselection.h"
#include "../mapdata/shortestpath.h"
#include "FindRoomTreeWidgetItem.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <QPointer>
#include <QString>
#include <QtGui>
#include <QtWidgets>

FindRoomsDlg::FindRoomsDlg(MapData &md, QWidget *const parent)
    : QDialog(parent)
    , m_mapData{md}
{
    setupUi(this);

    adjustResultTable();

    m_showSelectedRoom = std::make_unique<QShortcut>(QKeySequence(tr("Space", "Select result item")),
                                                     resultTable);
    m_showSelectedRoom->setContext(Qt::WidgetShortcut);

    selectButton->setEnabled(false);
    editButton->setEnabled(false);

    findButton->setDefault(true);

    connect(lineEdit, &QLineEdit::textChanged, this, &FindRoomsDlg::slot_enableFindButton);
    connect(findButton, &QAbstractButton::clicked, this, &FindRoomsDlg::slot_findClicked);
    connect(closeButton, &QAbstractButton::clicked, this, &QWidget::close);
    connect(resultTable,
            &QTreeWidget::itemDoubleClicked,
            this,
            &FindRoomsDlg::slot_itemDoubleClicked);
    connect(m_showSelectedRoom.get(),
            &QShortcut::activated,
            this,
            &FindRoomsDlg::slot_showSelectedRoom);
    connect(resultTable, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        if (resultTable->itemAt(pos) == nullptr) {
            return;
        }
        // Not QMenu::exec(): its nested event loop is unavailable on wasm.
        auto menu = std::make_unique<QMenu>(this);
        QAction *const copyPathAction = menu->addAction(tr("Copy &Path"));
        copyPathAction->setEnabled(!selectedPaths().isEmpty());
        connect(copyPathAction, &QAction::triggered, this, &FindRoomsDlg::slot_copyPaths);
        mmqt::popupMenu(std::move(menu), resultTable->viewport()->mapToGlobal(pos));
    });
    m_copyPaths = std::make_unique<QShortcut>(QKeySequence::Copy, resultTable);
    m_copyPaths->setContext(Qt::WidgetShortcut);
    connect(m_copyPaths.get(), &QShortcut::activated, this, &FindRoomsDlg::slot_copyPaths);
    connect(resultTable, &QTreeWidget::itemSelectionChanged, this, [this]() {
        const bool enabled = !resultTable->selectedItems().isEmpty();
        selectButton->setEnabled(enabled);
        editButton->setEnabled(enabled);
    });
    connect(selectButton, &QAbstractButton::clicked, this, [this]() {
        auto map = m_mapData.getCurrentMap();
        RoomIdSet tmpSet;
        glm::vec2 sum{0.f, 0.f};
        for (const auto &selectedItem : resultTable->selectedItems()) {
            const auto id = ExternalRoomId{selectedItem->text(0).toUInt()};
            if (const auto room = map.findRoomHandle(id)) {
                sum += room.getPosition().to_vec2();
                tmpSet.insert(room.getId());
            }
        }

        if (!tmpSet.empty()) {
            // note: half-room offset to the room center
            const glm::vec2 offset{0.5f, 0.5f};
            const auto worldPos = sum / static_cast<float>(tmpSet.size()) + offset;
            emit sig_center(worldPos);
        }

        auto shared = RoomSelection::createSelection(std::move(tmpSet));
        emit sig_newRoomSelection(SigRoomSelection{std::move(shared)});
    });
    connect(editButton, &QAbstractButton::clicked, this, [this]() {
        const auto &map = m_mapData;
        const auto &currentMap = map.getCurrentMap();
        RoomIdSet set;
        for (const auto &selectedItem : resultTable->selectedItems()) {
            const auto extid = ExternalRoomId{selectedItem->text(0).toUInt()};
            if (const auto room = currentMap.findRoomHandle(extid)) {
                set.insert(room.getId());
            }
        }
        auto tmpSel = RoomSelection::createSelection(std::move(set));
        emit sig_newRoomSelection(SigRoomSelection{std::move(tmpSel)});
        emit sig_editSelection();
    });

    label->setFocusProxy(lineEdit);

    readSettings();
}

FindRoomsDlg::~FindRoomsDlg()
{
    if (m_currentTask) {
        m_currentTask->requestCancel();
        m_currentTask.reset();
    }
    resultTable->clear();
}

void FindRoomsDlg::showEvent(QShowEvent *const event)
{
    QDialog::showEvent(event);
    lineEdit->setFocus();
}

void FindRoomsDlg::readSettings()
{
    restoreGeometry(getConfig().findRoomsDialog.geometry);
}

void FindRoomsDlg::writeSettings()
{
    setConfig().findRoomsDialog.geometry = saveGeometry();
}

void FindRoomsDlg::slot_findClicked()
{
    const Qt::CaseSensitivity cs = caseCheckBox->isChecked() ? Qt::CaseSensitive
                                                             : Qt::CaseInsensitive;
    const bool regex = regexCheckBox->isChecked();

    const std::string text = mmqt::toStdStringUtf8(lineEdit->text());

    if (m_currentTask) {
        m_currentTask->requestCancel();
        m_currentTask.reset();
    }
    const uint64_t gen = ++m_searchGeneration;

    resultTable->clear();
    roomsFoundLabel->setText(tr("Searching..."));
    selectButton->setEnabled(false);
    editButton->setEnabled(false);

    auto kind = PatternKindsEnum::ALL;
    if (nameRadioButton->isChecked()) {
        kind = PatternKindsEnum::NAME;
    } else if (descRadioButton->isChecked()) {
        kind = PatternKindsEnum::DESC;
    } else if (contentsRadioButton->isChecked()) {
        kind = PatternKindsEnum::CONTENTS;
    } else if (exitsRadioButton->isChecked()) {
        kind = PatternKindsEnum::EXITS;
    } else if (notesRadioButton->isChecked()) {
        kind = PatternKindsEnum::NOTE;
    } else if (flagsRadioButton->isChecked()) {
        kind = PatternKindsEnum::FLAGS;
    } else if (areaRadioButton->isChecked()) {
        kind = PatternKindsEnum::AREA;
    }

    try {
        RoomFilter filter(text, cs, regex, kind);
        const Map map = m_mapData.getCurrentMap();
        // Derive the origin from the same snapshot so the RoomIds are consistent.
        const RoomHandle current = m_mapData.getCurrentRoom();
        const RoomHandle origin = current.exists() ? map.findRoomHandle(current.getId())
                                                   : RoomHandle{};

        auto result = std::make_shared<SearchResult>(SearchResult{map, {}, {}});

        auto backgroundWorker =
            [filter, origin, result](const std::shared_ptr<ProgressCounter> &pc) {
                try {
                    findRooms(*result, origin, filter, *pc);
                } catch (const ProgressCanceledException &) {
                    throw; // the task framework handles cancellation
                } catch (const std::exception &ex) {
                    // onSuccess still runs, so the dialog can report the failure instead of
                    // staying on "Searching...".
                    result->rows.clear();
                    result->error = QString::fromUtf8(ex.what());
                }
            };

        // Runs on the main thread.
        auto onSuccess = [self = QPointer<FindRoomsDlg>(this), gen, result](
                             const std::shared_ptr<ProgressCounter> &) {
            if (self != nullptr && self->m_searchGeneration == gen) {
                self->showResults(result);
            }
        };

        m_currentTask = async_tasks::startAsyncTask2(AsyncTaskTypeEnum::Task,
                                                     AllowCancelEnum::Allow,
                                                     "Find Rooms",
                                                     std::move(backgroundWorker),
                                                     std::move(onSuccess));
    } catch (const std::exception &ex) {
        qWarning() << "Exception: " << ex.what();
        mmqt::showCritical(this,
                           "Internal Error",
                           QString::asprintf("An exception occurred: %s\n", ex.what()));
        roomsFoundLabel->clear();
    }
}

// Runs on a background thread: it must not touch widgets or the config.
void FindRoomsDlg::findRooms(SearchResult &result,
                             const RoomHandle &origin,
                             const RoomFilter &filter,
                             ProgressCounter &pc)
{
    const Map &map = result.map;
    const auto foundSet = ::genericFind(map, filter);
    pc.step(0); // throws if canceled

    // The search below re-applies the filter to reachable rooms. That's intentional: it lets
    // us stop early, while genericFind() still lists rooms that cannot be reached.
    struct NODISCARD Reach final
    {
        int steps = 0;
        double cost = 0.0;
        std::string path;       // compact form, e.g. "2n3eu"
        bool deathtrap = false; // a room on the route (including the destination) is a deathtrap
    };
    std::unordered_map<RoomId, Reach> reach;

    if (origin.exists() && !foundSet.empty()) {
        class NODISCARD Recipient final : public ShortestPathRecipient
        {
        private:
            std::unordered_map<RoomId, Reach> &m_reach;

        public:
            explicit Recipient(std::unordered_map<RoomId, Reach> &reach)
                : m_reach{reach}
            {}

        private:
            void virt_receiveShortestPath(QVector<SPNode> spnodes, const int endpoint) final
            {
                const std::string dirs = getPathDirections(spnodes, endpoint);
                bool deathtrap = false;
                for (int i = endpoint; i >= 0; i = spnodes[i].parent) {
                    const auto flags = spnodes[i].r.getLoadFlags();
                    deathtrap |= flags.contains(RoomLoadFlagEnum::DEATHTRAP);
                }
                m_reach.emplace(spnodes[endpoint].r.getId(),
                                Reach{static_cast<int>(dirs.size()),
                                      spnodes[endpoint].dist,
                                      formatDirRuns(dirs),
                                      deathtrap});
            }
        };

        Recipient recipient{reach};
        // Stop as soon as every match has been reached.
        MapData::shortestPathSearch(pc,
                                    origin,
                                    recipient,
                                    filter,
                                    static_cast<int>(foundSet.size()),
                                    0);
    }

    result.hasOrigin = origin.exists();
    result.rows.reserve(foundSet.size());
    for (const RoomId id : foundSet) {
        const auto room = map.getRoomHandle(id);
        FoundRoom row{id, room.getIdExternal(), room.getName().toQString(), std::nullopt, 0.0, {}};
        if (const auto it = reach.find(id); it != reach.end()) {
            row.steps = it->second.steps;
            row.cost = it->second.cost;
            row.path = QString::fromStdString(it->second.path);
            row.deathtrap = it->second.deathtrap;
        }
        result.rows.push_back(std::move(row));
    }
}

void FindRoomsDlg::showResults(const std::shared_ptr<const SearchResult> &result)
{
    resultTable->setSortingEnabled(false);
    resultTable->clear();

    if (!result->error.isEmpty()) {
        roomsFoundLabel->setText(tr("Search failed: %1").arg(result->error));
        resultTable->setSortingEnabled(true);
        return;
    }

    QList<QTreeWidgetItem *> items;
    items.reserve(static_cast<qsizetype>(result->rows.size()));
    for (const FoundRoom &row : result->rows) {
        auto *const treeItem = new FindRoomTreeWidgetItem();
        treeItem->setText(0, QString::number(row.extId.asUint32()));
        treeItem->setText(1, row.name);
        treeItem->setSteps(row.steps, row.cost);
        treeItem->setPath(row.path, row.deathtrap);
        // previewRoom() reads the config, so it must run on the main thread, and only when
        // a tooltip is actually requested.
        treeItem->setToolTipProvider(
            [result, id = row.id]() { return constructToolTip(result->map.getRoomHandle(id)); });
        items.push_back(treeItem);
    }
    resultTable->addTopLevelItems(items);

    const int count = resultTable->topLevelItemCount();
    roomsFoundLabel->setText(tr("%1 room%2 found").arg(count).arg((count == 1) ? "" : "s"));

    // Nearest rooms first; without a current room there are no distances, so use the name.
    // (Set the indicator before enabling sorting so the table is sorted only once.)
    resultTable->header()->setSortIndicator(result->hasOrigin ? 2 : 1, Qt::AscendingOrder);
    resultTable->setSortingEnabled(true);
}

QString FindRoomsDlg::constructToolTip(const RoomHandle &room)
{
    // Tooltip doesn't support ANSI, and there's no way to add formatted text.
    return mmqt::previewRoom(room, mmqt::StripAnsiEnum::Yes, mmqt::PreviewStyleEnum::ForDisplay);
}

QStringList FindRoomsDlg::selectedPaths() const
{
    QStringList paths;
    for (const QTreeWidgetItem *const item : resultTable->selectedItems()) {
        if (const auto *const row = dynamic_cast<const FindRoomTreeWidgetItem *>(item)) {
            if (!row->path().isEmpty()) {
                paths << row->path();
            }
        }
    }
    return paths;
}

void FindRoomsDlg::slot_copyPaths()
{
    const QStringList paths = selectedPaths();
    if (!paths.isEmpty()) {
        QGuiApplication::clipboard()->setText(paths.join('\n'));
    }
}

void FindRoomsDlg::slot_showSelectedRoom()
{
    slot_itemDoubleClicked(resultTable->currentItem());
}

void FindRoomsDlg::slot_itemDoubleClicked(QTreeWidgetItem *const inputItem)
{
    if (inputItem == nullptr) {
        return;
    }

    const auto &map = m_mapData.getCurrentMap();
    const auto extId = ExternalRoomId{inputItem->text(0).toUInt()};
    if (const auto r = map.findRoomHandle(extId)) {
        // REVISIT: should this block be a stand-alone function?
        {
            assert(r.getIdExternal() == extId);
            const Coordinate c = r.getPosition();
            const auto worldPos = c.to_vec2() + glm::vec2{0.5f, 0.5f};
            emit sig_center(worldPos); // connects to MapWindow
        }
        emit sig_log("FindRooms", inputItem->toolTip(0));
    }
}

void FindRoomsDlg::adjustResultTable()
{
    resultTable->setColumnCount(4);
    resultTable->setHeaderLabels(QStringList()
                                 << tr("Room ID") << tr("Room Name") << tr("Steps") << tr("Path"));
    // The ID is only the lookup key for double-click/select/edit, not useful to players.
    resultTable->setColumnHidden(0, true);
    resultTable->setContextMenuPolicy(Qt::CustomContextMenu);
    resultTable->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    resultTable->setRootIsDecorated(false);
    resultTable->setAlternatingRowColors(true);
    resultTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    resultTable->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
}

void FindRoomsDlg::slot_enableFindButton(const QString &text)
{
    findButton->setEnabled(!text.isEmpty());
}

void FindRoomsDlg::closeEvent(QCloseEvent *event)
{
    if (m_currentTask) {
        m_currentTask->requestCancel();
        m_currentTask.reset();
    }
    ++m_searchGeneration;

    writeSettings();
    resultTable->clear();
    roomsFoundLabel->clear();
    lineEdit->clear();
    lineEdit->setFocus();
    selectButton->setEnabled(false);
    editButton->setEnabled(false);
    event->accept();
}

void FindRoomsDlg::slot_on_lineEdit_textChanged()
{
    findButton->setEnabled(lineEdit->hasAcceptableInput());
}
