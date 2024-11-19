#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)

#include "../global/macros.h"
#include "../global/range.h"
#include "DoorFlags.h"
#include "ExitDirection.h"
#include "ExitFieldVariant.h"
#include "ExitFlags.h"
#include "roomid.h"

#include <cassert>
#include <set>
#include <stdexcept>

// X(_Type, _Name, _OptInit)
#define XFOREACH_EXIT_STRING_PROPERTY(X) X(DoorName, doorName, )

// X(_Type, _Name, _OptInit)
#define XFOREACH_EXIT_FLAG_PROPERTY(X) \
    X(ExitFlags, exitFlags, ) \
    X(DoorFlags, doorFlags, )

// X(_Type, _Name, _OptInit)
#define XFOREACH_EXIT_PROPERTY(X) \
    XFOREACH_EXIT_STRING_PROPERTY(X) \
    XFOREACH_EXIT_FLAG_PROPERTY(X)
