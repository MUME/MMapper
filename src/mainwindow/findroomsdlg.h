#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Kalev Lember <kalev@smartlink.ee> (Kalev)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/AsyncTasks.h"
#include "../map/roomid.h"
#include "../mapdata/roomselection.h"
#include "../parser/abstractparser.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <QDialog>
#include <QString>
#include <QtCore>
#include <QtGlobal>
#include <QtWidgets/QTreeWidgetItem>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
#include "ui_findroomsdlg.h" // auto-generated
#ifdef __clang__
#pragma clang diagnostic pop
#endif

class MapCanvasWindow;
class MapData;
class ProgressCounter;
class QCloseEvent;
class QObject;
class QShortcut;
class QTreeWidgetItem;
class QWidget;
class RoomFilter;

class NODISCARD_QOBJECT FindRoomsDlg final : public QDialog, private Ui::FindRoomsDlg
{
    Q_OBJECT

private:
    MapData &m_mapData;
    std::unique_ptr<QShortcut> m_showSelectedRoom;
    std::unique_ptr<QShortcut> m_copyPaths;
    std::optional<async_tasks::AsyncTaskHandle> m_currentTask;
    uint64_t m_searchGeneration = 0; // main thread only

private:
    void closeEvent(QCloseEvent *event) final;

public:
    explicit FindRoomsDlg(MapData &, QWidget *parent);
    ~FindRoomsDlg() final;

    void showEvent(QShowEvent *event) override;
    void readSettings();
    void writeSettings();

private:
    void adjustResultTable();
    struct NODISCARD FoundRoom final
    {
        RoomId id;
        ExternalRoomId extId;
        QString name;
        std::optional<int> steps; // nullopt if unreachable or no current room
        double cost = 0.0;        // tie-breaker when sorting by steps
        bool deathtrap = false;   // route passes a deathtrap: show a warning, not a path
        QString path;             // e.g. "2n3eu"; empty if unreachable or already there
    };
    struct NODISCARD SearchResult final
    {
        Map map; // snapshot the rows (and their lazy tooltips) refer to
        std::vector<FoundRoom> rows;
        QString error;                          // non-empty if the search failed
        bool canceled = false;                  // canceled, e.g. from the tasks panel
        std::optional<ExternalRoomId> originId; // room the steps/paths are relative to
        QString originName;

        explicit SearchResult(Map m)
            : map{std::move(m)}
        {}
    };

private:
    std::shared_ptr<const SearchResult> m_lastResult; // shown results; null if none

private:
    NODISCARD static QString constructToolTip(const RoomHandle &);
    NODISCARD QStringList selectedPaths() const;
    // Steps and paths are relative to the room the search started from.
    NODISCARD bool hasMovedSinceSearch() const;
    void updateResultsLabel();
    // Cancels any running search and drops results/callbacks that belong to it.
    void invalidateSearch();
    static void findRooms(SearchResult &,
                          const RoomHandle &origin,
                          const RoomFilter &,
                          ProgressCounter &);
    void showResults(const std::shared_ptr<const SearchResult> &);

signals:
    void sig_center(glm::vec2 worldPos);
    void sig_newRoomSelection(const SigRoomSelection &);
    void sig_editSelection();
    void sig_log(const QString &, const QString &);

private slots:
    void slot_copyPaths();
    void slot_on_lineEdit_textChanged();
    void slot_findClicked();
    void slot_enableFindButton(const QString &text);
    void slot_itemDoubleClicked(QTreeWidgetItem *inputItem);
    void slot_showSelectedRoom();

public slots:
    void slot_closeEvent(QCloseEvent *event)
    {
        /* virtual */
        closeEvent(event);
    }
};
