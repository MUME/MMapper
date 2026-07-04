#include "FunctionsGL33.h"

#include "../../global/ConfigConsts.h"

#include <optional>

namespace Legacy {

FunctionsGL33::FunctionsGL33(Badge<Functions> badge, UboManager &uboManager)
    : Functions(badge, uboManager)
{}

std::optional<GLenum> FunctionsGL33::virt_toGLenum(const DrawModeEnum mode)
{
    switch (mode) {
    case DrawModeEnum::POINTS:
        return GL_POINTS;
    case DrawModeEnum::LINES:
        return GL_LINES;
    case DrawModeEnum::TRIANGLES:
        return GL_TRIANGLES;
    case DrawModeEnum::QUADS:
    case DrawModeEnum::INSTANCED_QUADS:
    case DrawModeEnum::INVALID:
        break;
    }

    return std::nullopt;
}

const char *FunctionsGL33::virt_getShaderVersion() const
{
    return "#version 330\n\n";
}

void FunctionsGL33::virt_enableProgramPointSize(const bool enable)
{
#ifndef MMAPPER_NO_OPENGL
    if (enable) {
        Base::glEnable(GL_PROGRAM_POINT_SIZE);
    } else {
        Base::glDisable(GL_PROGRAM_POINT_SIZE);
    }
#endif
}

} // namespace Legacy
