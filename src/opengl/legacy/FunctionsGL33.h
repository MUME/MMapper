#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "Legacy.h"

namespace Legacy {

class FunctionsGL33 final : public Functions
{
public:
    explicit FunctionsGL33(Badge<Functions>);
    ~FunctionsGL33() override = default;

    DELETE_CTORS_AND_ASSIGN_OPS(FunctionsGL33);

private:
    void virt_enableProgramPointSize(bool enable) override;
    NODISCARD const char *virt_getShaderVersion() const override;
    NODISCARD bool virt_canRenderQuads() override;
    NODISCARD std::optional<GLenum> virt_toGLenum(DrawModeEnum mode) override;
};

} // namespace Legacy
