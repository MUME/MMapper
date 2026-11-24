#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/JsonObj.h"
#include "../global/macros.h"
#include "MumeEditOptions.h"

/// Reads the limits MUME attaches to a MUME.Client.Edit message ("max-size" and
/// "allowed-ranges"). An absent "allowed-ranges" key allows everything; a present
/// key always restricts. Malformed entries are ignored.
NODISCARD MumeEditOptions parseMumeEditOptions(const JsonObj &obj);
