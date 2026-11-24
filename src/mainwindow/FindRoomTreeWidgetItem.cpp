// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "FindRoomTreeWidgetItem.h"

#include <utility>

#include <QHeaderView>

static constexpr int STEPS_COLUMN = 2;
static constexpr int ROLE_STEPS = Qt::UserRole;
static constexpr int ROLE_COST = Qt::UserRole + 1;
static constexpr int ROLE_PATH = Qt::UserRole;

FindRoomTreeWidgetItem::~FindRoomTreeWidgetItem() = default;

void FindRoomTreeWidgetItem::setSteps(const std::optional<int> steps, const double cost)
{
    if (!steps) {
        setText(STEPS_COLUMN, QObject::tr("N/A"));
        setData(STEPS_COLUMN, ROLE_STEPS, QVariant{});
        return;
    }
    setText(STEPS_COLUMN, QString::number(*steps));
    setData(STEPS_COLUMN, ROLE_STEPS, *steps);
    setData(STEPS_COLUMN, ROLE_COST, cost);
}

void FindRoomTreeWidgetItem::setPath(const QString &path, const bool deathtrap)
{
    if (deathtrap) {
        setText(PATH_COLUMN, QObject::tr("Avoid: route has a deathtrap"));
        setData(PATH_COLUMN, ROLE_PATH, QString{});
        return;
    }
    setText(PATH_COLUMN, path);
    setData(PATH_COLUMN, ROLE_PATH, path);
}

QString FindRoomTreeWidgetItem::path() const
{
    return data(PATH_COLUMN, ROLE_PATH).toString();
}

void FindRoomTreeWidgetItem::setToolTipProvider(std::function<QString()> provider)
{
    m_toolTipProvider = std::move(provider);
    m_toolTip.reset();
}

QVariant FindRoomTreeWidgetItem::data(const int column, const int role) const
{
    if (role == Qt::ToolTipRole && m_toolTipProvider) {
        if (!m_toolTip) {
            m_toolTip = m_toolTipProvider();
        }
        return *m_toolTip;
    }
    return QTreeWidgetItem::data(column, role);
}

bool FindRoomTreeWidgetItem::operator<(const QTreeWidgetItem &other) const
{
    const int col = treeWidget() ? treeWidget()->sortColumn() : 0;
    if (col == 0) {
        return text(0).toUInt() < other.text(0).toUInt();
    }
    if (col == STEPS_COLUMN) {
        const QVariant a = data(col, ROLE_STEPS);
        const QVariant b = other.data(col, ROLE_STEPS);
        if (a.isValid() && b.isValid()) {
            if (a.toInt() != b.toInt()) {
                return a.toInt() < b.toInt();
            }
            return data(col, ROLE_COST).toDouble() < other.data(col, ROLE_COST).toDouble();
        }
        if (a.isValid() != b.isValid()) {
            // Keep "N/A" rows last in both sort orders (Qt inverts the result when descending).
            const bool ascending = treeWidget() == nullptr
                                   || treeWidget()->header()->sortIndicatorOrder()
                                          == Qt::AscendingOrder;
            return ascending == a.isValid();
        }
        return false;
    }
    return QTreeWidgetItem::operator<(other);
}
