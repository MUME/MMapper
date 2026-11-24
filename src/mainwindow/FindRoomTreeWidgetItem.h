#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/macros.h"

#include <functional>
#include <optional>

#include <QString>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>

// Row of the Find Rooms results. Column 0 (room id) and column 2 (steps from the
// current room) sort numerically; the steps column breaks ties by path cost.
class NODISCARD FindRoomTreeWidgetItem final : public QTreeWidgetItem
{
public:
    static constexpr int PATH_COLUMN = 3;

    using QTreeWidgetItem::QTreeWidgetItem;

private:
    std::function<QString()> m_toolTipProvider;
    mutable std::optional<QString> m_toolTip;

public:
    ~FindRoomTreeWidgetItem() override;

    // nullopt means unreachable / no current room and is displayed as "N/A".
    void setSteps(std::optional<int> steps, double cost);

    // Compact pasteable route from the current room (e.g. "2n3eu"). If the route passes a
    // deathtrap, a warning is shown instead and there is nothing to copy.
    void setPath(const QString &path, bool deathtrap);
    // The copyable path; empty if there is none.
    NODISCARD QString path() const;

    // The tooltip is built on first request (on the thread that asks for it).
    void setToolTipProvider(std::function<QString()> provider);

    NODISCARD QVariant data(int column, int role) const override;
    NODISCARD bool operator<(const QTreeWidgetItem &other) const override;
};
