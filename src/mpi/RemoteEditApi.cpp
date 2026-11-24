// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "RemoteEditApi.h"

#include "../global/AnsiOstream.h"
#include "MumeEditOptions.h"
#include "remoteedit.h"
#include "remoteeditsession.h"

#include <utility>

void RemoteEditApi::reportStatus(AnsiOstream &aos)
{
    m_remoteEdit.reportStatus(aos);
}

bool RemoteEditApi::reportStatus(AnsiOstream &aos, const uint32_t id)
{
    if (!m_remoteEdit.reportStatus(aos, RemoteInternalId{id})) {
        aos.write("Error: Invalid remote edit id.\n");
        return false;
    }
    return true;
}

bool RemoteEditApi::cancel(const uint32_t id)
{
    const auto &sessions = m_remoteEdit.getSessions();
    const auto it = sessions.find(RemoteInternalId{id});
    if (it == sessions.end()) {
        return false;
    }
    m_remoteEdit.cancelEdit(it->second.get());
    return true;
}

bool RemoteEditApi::discard(const uint32_t id)
{
    const auto &sessions = m_remoteEdit.getSessions();
    const auto it = sessions.find(RemoteInternalId{id});
    if (it == sessions.end()) {
        return false;
    }
    // A live edit has no draft-only state to discard; route it through cancelEdit()
    // (which also sends the GMCP cancel and deletes the draft) instead.
    if (it->second->isDraftView()) {
        m_remoteEdit.discardDraft(it->second.get());
    } else {
        m_remoteEdit.cancelEdit(it->second.get());
    }
    return true;
}

void RemoteEditApi::simulateEdit(const QString &title)
{
    // Large ids keep the fake session clear of anything MUME hands out.
    static int32_t nextFakeId = 1000000;

    // Behave like legacy MUME (Latin-1 only) so typing e.g. an emoji exercises
    // the disallowed-character warning.
    MumeEditOptions options;
    MumeEditOptions::AllowedRanges allowed;
    allowed.tryAddRange(0, 255);
    options.allowed = std::move(allowed);

    m_remoteEdit.slot_remoteEdit(RemoteSessionId{nextFakeId++},
                                 title,
                                 QString(
                                     "Simulated edit \"%1\".\nType here; nothing is sent to MUME.\n")
                                     .arg(title),
                                 options);
}
