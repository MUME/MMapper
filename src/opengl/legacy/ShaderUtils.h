#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <string>
#include <vector>

#include "Legacy.h"

namespace ShaderUtils {
using Functions = Legacy::Functions;

struct NODISCARD Source final
{
    const std::string filename;
    const std::string source;

    explicit Source() = default;
    explicit Source(std::nullptr_t) = delete;
    explicit Source(std::string moved_filename, std::string moved_source)
        : filename{std::move(moved_filename)}
        , source{std::move(moved_source)}
    {}

    explicit operator bool() const { return !filename.empty() || !source.empty(); }
};

NODISCARD GLuint loadShaders(Functions &gl, const Source &vert, const Source &frag);

} // namespace ShaderUtils
