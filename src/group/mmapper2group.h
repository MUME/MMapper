#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/JsonValue.h"
#include "CGroupChar.h"
#include "ColorGenerator.h"
#include "GroupManagerApi.h"
#include "mmapper2character.h"

#include <memory>

#include <QArgument>
#include <QMap>
#include <QObject>

class GmcpMessage;

class NODISCARD_QOBJECT Mmapper2Group final : public QObject
{
    Q_OBJECT

private:
    SharedGroupChar m_self;
    GroupVector m_charIndex;
    // deleted in destructor as member of charIndex
    ColorGenerator m_colorGenerator;

private:
    void log(const QString &msg) { emit sig_log("Group", msg); }

public:
    explicit Mmapper2Group(QObject *parent);
    ~Mmapper2Group() final;

public:
    void resetChars();
    NODISCARD const GroupVector &selectAll() { return m_charIndex; }

public:
    NODISCARD GroupManagerApi &getGroupManagerApi() { return deref(m_groupManagerApi); }

private:
    friend GroupManagerApi;
    std::unique_ptr<GroupManagerApi> m_groupManagerApi;

private:
    void init();
    void characterChanged(bool updateCanvas);

private:
    void parseGmcpCharName(const JsonObj &obj);
    void parseGmcpCharStatusVars(const JsonObj &obj);
    void parseGmcpCharVitals(const JsonObj &obj);
    void parseGmcpGroupAdd(const JsonObj &obj);
    void parseGmcpGroupUpdate(const JsonObj &obj);
    void parseGmcpGroupRemove(const JsonInt i);
    void parseGmcpGroupSet(const JsonArray &arr);
    void parseGmcpRoomInfo(const JsonObj &obj);

private:
    NODISCARD SharedGroupChar addChar(const GroupId id);
    void removeChar(const GroupId id);
    NODISCARD bool updateChar(SharedGroupChar sharedCh,
                              const JsonObj &json); // updates given char from GMCP

private:
    NODISCARD CharacterTypeEnum getCharacterType(const JsonObj &json);
    NODISCARD GroupId getGroupId(const JsonObj &json);

public:
    NODISCARD SharedGroupChar getCharById(const GroupId id) const;
    NODISCARD SharedGroupChar getCharByName(const CharacterName &name) const;

public:
    void onReset();

signals:
    // MainWindow::log (via MainWindow)
    void sig_log(const QString &, const QString &);
    // MapCanvas::requestUpdate (via MainWindow)
    void sig_updateMapCanvas(); // redraw the opengl screen

    // GroupWidget::updateLabels (via GroupWidget)
    void sig_updateWidget(); // update group widget

public slots:
    void slot_parseGmcpInput(const GmcpMessage &msg);
    void slot_setCharRoomIdEstimated(ServerRoomId serverId, ExternalRoomId externalId);
};
