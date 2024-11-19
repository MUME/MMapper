#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "RuleOf5.h"
#include "macros.h"

#include <memory>

namespace mmqt {

struct NODISCARD HideQDebugOptions final
{
    bool hideDebug = true;
    bool hideInfo = true;
    bool hideWarning = false;
};

class HideDebugPimpl;

// NOTE: The current implementation doesn't allow you to "restore" a hidden message type
// within the liftime of HideQDebug, except by manually calling qInstallMessageHandler(),
// but it is possible to have more than one HideQDebug in the current scope;
// see the HideQDebug test case for a demonstration.
//
// If no other handlers are installed, then "effectively" each HideQDebug object adds
// to a static reference count for the suppression of QDebug and/or QInfo,
//
// but if you call qInstallMessageHandler() with another function, then it will receive
// info and debug messages unless you add another HideQDebug object.
//
// // pseudocode
// void someFunction()
// {
//   {
//      HideQDebug forThisScope;
//      qInfo() << "this will be hidden";
//      qDebug() << "this will be hidden";
//      qWarning() << "this will be shown";
//   }
//   qInfo() << "this will be shown";
//   qDebug() << "this will be shown";
// }
//
class NODISCARD HideQDebug final
{
private:
    std::shared_ptr<HideDebugPimpl> m_pimpl;

public:
    explicit HideQDebug(HideQDebugOptions options = HideQDebugOptions{});
    ~HideQDebug();
    DELETE_CTORS_AND_ASSIGN_OPS(HideQDebug);
};

} // namespace mmqt
