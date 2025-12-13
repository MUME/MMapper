#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <string>

#include <QSurfaceFormat>

class NODISCARD OpenGLProber final
{
public:
    enum class BackendType { None, GL, GLES };

    struct ProbeResult
    {
        BackendType backendType = BackendType::None;
        QSurfaceFormat format;
        std::string highestVersionString = "Unknown";
        bool isCompat = false;
    };

public:
    OpenGLProber() = default;
    DELETE_CTORS_AND_ASSIGN_OPS(OpenGLProber);
    DTOR(OpenGLProber) = default;

    NODISCARD ProbeResult probe();
};
