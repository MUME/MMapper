// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "TasksPanel.h"

#include "../global/AsyncTasks.h"
#include "../global/MakeQPointer.h"
#include "../global/PrintUtils.h"
#include "../global/SendToUser.h"
#include "../global/thread_utils.h"
#include "AsyncTypes.h"
#include "mainwindow.h"

#include <utility>

#include <QTimer>

namespace {
NODISCARD bool try_cancel(const size_t task_id)
{
    bool canceled = false;
    global::logAndSendToUser(global::LogDestEnum::Info, [&canceled, task_id](AnsiOstream &aos) {
        canceled = async_tasks::cancel(aos, task_id);
    });
    return canceled;
}

class NODISCARD CancelTaskButton final : public QPushButton
{
private:
    async_tasks::AsyncTaskHandle m_task;

public:
    explicit CancelTaskButton(async_tasks::AsyncTaskHandle moved_task)
        : m_task(std::move(moved_task))
    {
        setText((QString("Cancel Task #%1").arg(m_task.getId())));
        if (m_task.getCanCancel() != AllowCancelEnum::Allow) {
            setEnabled(false);
        } else {
            connect(this, &QPushButton::clicked, this, [this]() {
                m_task.requestCancel();
                setEnabled(false);
            });
        }
    }
    ~CancelTaskButton() override;
    DELETE_CTORS_AND_ASSIGN_OPS(CancelTaskButton);
};

CancelTaskButton::~CancelTaskButton() = default;
} // namespace

struct NODISCARD TasksPanel::TaskHandle final
{
    async_tasks::AsyncTaskHandle task;
};

struct NODISCARD TasksPanel::TaskMap final : std::map<size_t, TaskHandle>
{};

struct NODISCARD TasksPanel::ListItem final : public QWidget
{
private:
    async_tasks::AsyncTaskHandle m_task;
    QScopedPointer<QLabel> m_label = mmqt::makeQScopedPointer<QLabel>();
    QScopedPointer<QProgressBar> m_progress = std::invoke([]() -> QScopedPointer<QProgressBar> {
        auto progress = std::make_unique<QProgressBar>();
        progress->setMinimum(0);
        progress->setMaximum(100);
        return QScopedPointer<QProgressBar>{progress.release()};
    });
    QScopedPointer<CancelTaskButton> m_cancelButton;

public:
    explicit ListItem(async_tasks::AsyncTaskHandle moved_task)
        : m_task{std::move(moved_task)}
        , m_cancelButton{mmqt::makeQScopedPointer<CancelTaskButton>(m_task)}
    {
        const auto layout = mmqt::makeQPointer<QVBoxLayout>(this);
        layout->addWidget(m_label.get());
        layout->addWidget(m_progress.get());
        layout->addWidget(m_cancelButton.get());
        layout->insertStretch(-1); // must be after all the addWidget() calls
        updateProgress();
    }
    ~ListItem() override;
    DELETE_CTORS_AND_ASSIGN_OPS(ListItem);

public:
    NODISCARD size_t get_task_id() const { return m_task.getId(); }

public:
    void updateProgress()
    {
        const auto &pc = m_task.getProgressCounter();
        const auto status = pc.getStatus();
        const bool is_removed = m_task.getIsRemoved();
        const auto pct = std::invoke([&pc, &status, is_removed]() -> int {
            if (is_removed) {
                return pc.hasRequestedCancel() ? 0 : 100;
            }
            return static_cast<int>(status.percent());
        });

        const auto desc = std::invoke([this, &pc, &status, is_removed, pct]() -> QString {
            const auto elapsed = std::invoke([this]() -> QString {
                std::ostringstream oss;
                {
                    AnsiOstream aos{oss};
                    async_tasks::formatElapsedSeconds(aos, m_task.getElapsedTime());
                }
                return mmqt::toQStringUtf8(strip_ansi(oss.str()));
            });

            const auto statusMsg = std::invoke([&pc, &status, is_removed, pct]() -> QString {
                if (is_removed) {
                    return pc.hasRequestedCancel() ? "Canceled." : "Finished.";
                }

                auto tmp = status.msg.toQString();
                if (!tmp.isEmpty()) {
                    tmp[0] = tmp[0].toUpper();
                }
                return QString("%1... (%2%)").arg(tmp).arg(pct);
            });

            return QString("Task #%1: %2 (%3) [elapsed: %4]\nStatus: %5")
                .arg(m_task.getId())
                .arg(mmqt::toQStringUtf8(m_task.getName()))
                .arg((m_task.getCanCancel() == AllowCancelEnum::Allow) ? "cancelable"
                                                                       : "non-cancelable")
                .arg(elapsed)
                .arg(statusMsg);
        });

        deref(m_label).setText(desc);
        deref(m_progress).setValue(pct);
        if (pc.hasRequestedCancel()) {
            deref(m_cancelButton).setEnabled(false);
        }

        if (is_removed) {
            setEnabled(false);
        }
    }

public:
    NODISCARD static ListItem *cast_or_die(QWidget *const w)
    {
        if (auto *const p = dynamic_cast<ListItem *>(w)) {
            return p;
        }

        assert(false);
        abort();
    }
};

TasksPanel::ListItem::~ListItem() = default;

TasksPanel::TasksPanel(MainWindow &mainWindow)
    : QScrollArea(&mainWindow)
    , m_mainWindow{mainWindow}
    , m_knownTasks{std::make_unique<TaskMap>()}
{
    const auto window = mmqt::makeQPointer<QWidget>(this);
    m_layout = mmqt::makeQPointer<QVBoxLayout>(window.get());
    m_layout->setSizeConstraint(QLayout::SetMinimumSize);

    QScrollArea *const scrollArea = this;
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(window);

    initTimer();
    initContextMenu();
}

TasksPanel::~TasksPanel() = default;

// tasks in our list
TasksPanel::TaskMap &TasksPanel::getKnownTasks()
{
    return deref(m_knownTasks);
}

// tasks in our list
const TasksPanel::TaskMap &TasksPanel::getKnownTasks() const
{
    return deref(m_knownTasks);
}

void TasksPanel::initTimer()
{
    // REVISIT: can we just use QWidget::startTimer() ?
    // NOLINTNEXTLINE (no, this is not leaked; Qt manages it)
    auto *const t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [this]() {
        refresh_data();
        QWidget &base = *this;
        base.update();
    });
    t->start(250);
}

void TasksPanel::initContextMenu()
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QListView::customContextMenuRequested, this, &TasksPanel::contextMenuClick);
}

void TasksPanel::contextMenuClick(const QPoint &pos)
{
    ABORT_IF_NOT_ON_MAIN_THREAD();
    const ListItem *const item = itemAt(pos);
    if (item == nullptr) {
        return;
    }

    const auto task_id = deref(item).get_task_id();

    // REVISIT: Should the context menu parent be "this" or "&m_mainWindow"?
    constexpr bool use_this_as_parent = true;
    QWidget *const parent = use_this_as_parent ? static_cast<QWidget *>(this)
                                               : static_cast<QWidget *>(&m_mainWindow);

    QMenu contextMenu{parent};
    auto add_entry = [this, &contextMenu, task_id](const auto &name, auto callback) {
        contextMenu.addAction(name,
                              this,
                              // note: QPointer becomes null if we're deleted
                              [pw = QPointer<TasksPanel>{this}, task_id, cb = std::move(callback)]() {
                                  if (pw) {
                                      std::invoke(cb, deref(pw), task_id);
                                  }
                              });
    };

    add_entry(QString("Cancel task %1").arg(task_id), &TasksPanel::try_cancel_by_key);
    contextMenu.exec(getScrollArea().viewport()->mapToGlobal(pos));
}

TasksPanel::ListItem *TasksPanel::lookup_by_key(const size_t task_id) const
{
    const auto size = getLayout().count();
    for (int i = 0; i < size; ++i) {
        if (ListItem *const item = itemAt(i)) {
            if (deref(item).get_task_id() == task_id) {
                return item;
            }
        }
    }
    return nullptr;
}

void TasksPanel::try_cancel_by_key(const size_t task_id)
{
    if (MAYBE_UNUSED ListItem *const item = lookup_by_key(task_id)) {
        std::ignore = try_cancel(task_id);
    }
}

// actual "current" tasks
TasksPanel::TaskMap TasksPanel::getCurrentTasks()
{
    TaskMap currentTasks;
    async_tasks::for_each([&currentTasks](const async_tasks::AsyncTaskHandle &task) {
        const auto task_id = task.getId();
        currentTasks.emplace(task_id, task);
    });
    return currentTasks;
}

NODISCARD TasksPanel::ListItem *TasksPanel::itemAt(const QPoint pos) const
{
    const QWidget *const vp = QScrollArea::viewport();
    for (QWidget *widget = vp->childAt(pos); widget != nullptr && widget != vp && widget != this;
         widget = widget->parentWidget()) {
        if (auto *const list_item = dynamic_cast<ListItem *>(widget)) {
            return list_item;
        }
    }
    return nullptr;
}

NODISCARD std::optional<int> TasksPanel::indexFromItem(const ListItem *const item) const
{
    const auto idx = getLayout().indexOf(item);
    if (idx < 0) {
        assert(false);
        return std::nullopt;
    }
    return idx;
}

NODISCARD TasksPanel::ListItem *TasksPanel::itemAt(const int pos) const
{
    if (const QLayoutItem *const item = getLayout().itemAt(pos)) {
        if (QWidget *const widget = item->widget()) {
            return ListItem::cast_or_die(widget);
        }
    }
    return nullptr;
}

NODISCARD TasksPanel::ListItem *TasksPanel::takeItem(const int pos)
{
    if (const QLayoutItem *const item = getLayout().takeAt(pos)) {
        if (QWidget *const widget = item->widget()) {
            return ListItem::cast_or_die(widget);
        }
    }
    return nullptr;
}

void TasksPanel::refresh_data()
{
    if (!isVisible()) {
        return;
    }

    const bool allowRemoval = !underMouse();
    const auto currentTasks = getCurrentTasks();
    std::map<size_t, bool> seen;

    // step 1: refresh and remove
    {
        std::vector<ListItem *> toRemove;
        const auto size = getLayout().count();
        for (int i = 0; i < size; ++i) {
            ListItem *const item = itemAt(i);
            if (item == nullptr) {
                assert(false);
                continue;
            }
            const auto task_id = deref(item).get_task_id();
            if (auto it = currentTasks.find(task_id); it != currentTasks.end()) {
                item->updateProgress();
                seen[task_id] = true;
            } else if (!allowRemoval) {
                // update existing value, even if it's no longer running...
                const auto &known = getKnownTasks();
                if (const auto known_it = known.find(task_id); known_it != known.end()) {
                    item->updateProgress();
                }
            } else {
                toRemove.push_back(item); // or schedule for removal
            }
        }

        if (allowRemoval && !toRemove.empty()) {
            for (const ListItem *const item : toRemove) {
                try_remove(item);
            }
        }
    }

    // step 2: add items that weren't already in the list
    for (const auto &[key, handle] : currentTasks) {
        if (const auto id = handle.task.getId(); !seen[id]) {
            seen[id] = true;
            add_new_item(handle);
        }
    }
}

void TasksPanel::add_new_item(const TaskHandle &handle)
{
    const auto &task = handle.task;
    // NOLINTNEXTLINE (no, this is not leaked; Qt manages it)
    if (auto *const item = new ListItem(task)) {
        getLayout().addWidget(item);
        getKnownTasks().emplace(task.getId(), task);
    }
}

void TasksPanel::try_remove(const ListItem *const item)
{
    if (item == nullptr) {
        assert(false);
        return;
    }

    const auto task_id = deref(item).get_task_id();
    getKnownTasks().erase(task_id);

    if (const auto index = indexFromItem(item); index.has_value()) {
        const ListItem *const got = takeItem(index.value());
        assert(got == item);
        if (item == got) {
            delete item;
        }
    }
}
