// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors
#pragma once

#include "../global/macros.h"
#include "../global/progresscounter.h"

#include <new>

#include <QListWidget>
#include <QScrollArea>
#include <QVBoxLayout>

class MainWindow;

class NODISCARD TasksPanel final : public QScrollArea
{
public:
    struct ListItem;

private:
    struct TaskMap;
    using base = QWidget;

    QPointer<QVBoxLayout> m_layout;
    MainWindow &m_mainWindow;
    std::unique_ptr<TaskMap> m_knownTasks;

public:
    explicit TasksPanel(MainWindow &mainWindow);
    ~TasksPanel() override;
    DELETE_CTORS_AND_ASSIGN_OPS(TasksPanel);

public:
    void refresh_data();

private:
    void initTimer();
    void initContextMenu();

private:
    NODISCARD static TaskMap getCurrentTasks();
    NODISCARD TaskMap &getKnownTasks();
    NODISCARD const TaskMap &getKnownTasks() const;

private:
    struct TaskHandle;
    void add_new_item(const TaskHandle &handle);
    void try_remove(const ListItem *item);

private:
    void contextMenuClick(const QPoint &pos);
    NODISCARD ListItem *lookup_by_key(size_t task_id) const;
    void try_cancel_by_key(size_t task_id);

private:
    NODISCARD ListItem *itemAt(int pos) const;
    NODISCARD ListItem *itemAt(QPoint pos) const;
    NODISCARD std::optional<int> indexFromItem(const ListItem *) const;
    NODISCARD ListItem *takeItem(int pos);
    NODISCARD QVBoxLayout &getLayout() { return deref(m_layout); }
    NODISCARD const QVBoxLayout &getLayout() const { return deref(m_layout); }
    NODISCARD QScrollArea &getScrollArea() { return *this; }
};
