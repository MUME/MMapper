#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "Legacy.h"

namespace Legacy {

class FunctionsES30 final : public Functions
{
public:
    explicit FunctionsES30(Badge<Functions>);
    ~FunctionsES30() override = default;

    DELETE_CTORS_AND_ASSIGN_OPS(FunctionsES30);

private:
    void virt_enableProgramPointSize(bool enable) override;
    NODISCARD bool virt_tryEnableMultisampling(int requestedSamples) override;
    NODISCARD const char *virt_getShaderVersion() const override;
    NODISCARD bool virt_canRenderQuads() override;
    NODISCARD std::optional<GLenum> virt_toGLenum(DrawModeEnum mode) override;
};

} // namespace Legacy
