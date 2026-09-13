#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "CGroupChar.h"
#include "GroupModel.h"
#include "mmapper2character.h"

#include <QSortFilterProxyModel>
#include <QString>
#include <QStyledItemDelegate>
#include <QWidget>
#include <QtCore>

class QAction;
class MapData;
class Mmapper2Group;
class QObject;
class QTableView;
class QTimer;

class NODISCARD_QOBJECT GroupDelegate final : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit GroupDelegate(QObject *parent);
    ~GroupDelegate() final;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    NODISCARD QSize sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const override;
};

class NODISCARD_QOBJECT GroupWidget final : public QWidget
{
    Q_OBJECT

private:
    QTableView *m_table = nullptr;
    Mmapper2Group *m_group = nullptr;
    MapData *m_map = nullptr;
    GroupProxyModel *m_proxyModel = nullptr;
    GroupModel *m_model = nullptr;
    QTimer *m_pulseTimer = nullptr;

    void updateColumnVisibility();
    void updatePulseTimer();

private:
    QAction *m_center = nullptr;
    QAction *m_recolor = nullptr;
    SharedGroupChar selectedCharacter;

public:
    explicit GroupWidget(Mmapper2Group *group, MapData *md, QWidget *parent);
    ~GroupWidget() final;

protected:
    NODISCARD QSize sizeHint() const override;

signals:
    void sig_kickCharacter(const QString &);
    void sig_center(glm::vec2);

public slots:
    void slot_mapUnloaded() { deref(m_model).setMapLoaded(false); }
    void slot_mapLoaded() { deref(m_model).setMapLoaded(true); }

private slots:
    void slot_onCharacterAdded(SharedGroupChar character);
    void slot_onCharacterRemoved(GroupId characterId);
    void slot_onCharacterUpdated(SharedGroupChar character);
    void slot_onGroupReset(const GroupVector &newCharacterList);
};
