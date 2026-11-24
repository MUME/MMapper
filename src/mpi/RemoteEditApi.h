#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <cstdint>

#include <QString>

class AnsiOstream;
class RemoteEdit;

/// What the parser's `_edits` command may do to the remote edit sessions.
class NODISCARD RemoteEditApi final
{
private:
    RemoteEdit &m_remoteEdit;

public:
    explicit RemoteEditApi(RemoteEdit &remoteEdit)
        : m_remoteEdit{remoteEdit}
    {}
    ~RemoteEditApi() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(RemoteEditApi);

public:
    void reportStatus(AnsiOstream &aos);
    NODISCARD bool reportStatus(AnsiOstream &aos, uint32_t id);
    NODISCARD bool cancel(uint32_t id);
    NODISCARD bool discard(uint32_t id);
    /// Testing aid: behaves exactly as if MUME had sent MUME.Client.Edit.
    void simulateEdit(const QString &title);
};
