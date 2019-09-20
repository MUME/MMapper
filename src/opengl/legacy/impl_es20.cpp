#include "Legacy.h"

namespace Legacy {

bool Functions::canRenderQuads()
{
    return false;
}

std::optional<GLenum> Functions::toGLenum(const DrawModeEnum mode)
{
    switch (mode) {
    case DrawModeEnum::POINTS:
        return GL_POINTS;
    case DrawModeEnum::LINES:
        return GL_LINES;
    case DrawModeEnum::TRIANGLES:
        return GL_TRIANGLES;

    case DrawModeEnum::INVALID:
    case DrawModeEnum::QUADS:
        break;
    }

    return std::nullopt;
}

const char *Functions::getShaderVersion()
{
    return "#version 100 // OpenGL ES 2.0\n\nprecision highp float;\n\n";
}

void Functions::enableProgramPointSize(bool /* enable */)
{
    // nop
}

bool Functions::tryEnableMultisampling(int /* requestedSamples */)
{
    return false;
}

} // namespace Legacy
