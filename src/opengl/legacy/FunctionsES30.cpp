#include "FunctionsES30.h"

namespace Legacy {

FunctionsES30::FunctionsES30(Badge<Functions> badge)
    : Functions(badge)
{
    assert(!OpenGLConfig::getIsCompat());
}

bool FunctionsES30::virt_canRenderQuads()
{
    return false;
}

std::optional<GLenum> FunctionsES30::virt_toGLenum(const DrawModeEnum mode)
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

const char *FunctionsES30::virt_getShaderVersion() const
{
    return "#version 300 es\n\nprecision highp float;\n\n";
}

void FunctionsES30::virt_enableProgramPointSize(bool /* enable */)
{
    // nop
}

bool FunctionsES30::virt_tryEnableMultisampling(int /* requestedSamples */)
{
    return false;
}

} // namespace Legacy
