#include "FunctionsGL33.h"

#include "../OpenGLConfig.h"

#include <optional>

namespace Legacy {

FunctionsGL33::FunctionsGL33(Badge<Functions> badge)
    : Functions(badge)
{}

bool FunctionsGL33::virt_canRenderQuads()
{
    return OpenGLConfig::getIsCompat();
}

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
#ifndef MMAPPER_NO_OPENGL
        return canRenderQuads() ? std::make_optional(GL_QUADS) : std::nullopt;
#endif
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

bool FunctionsGL33::virt_tryEnableMultisampling(const int requestedSamples)
{
#ifndef MMAPPER_NO_OPENGL
    const auto getSampleBuffers = [this]() -> GLint {
        GLint buffers;
        Base::glGetIntegerv(GL_SAMPLE_BUFFERS, &buffers);
        return buffers;
    };

    const auto getSamples = [this]() -> GLint {
        GLint samples;
        Base::glGetIntegerv(GL_SAMPLES, &samples);
        return samples;
    };

    const bool hasMultisampling = getSampleBuffers() > 1 || getSamples() > 1;
    if (hasMultisampling && requestedSamples > 0) {
        Base::glEnable(GL_MULTISAMPLE);
        Base::glEnable(GL_LINE_SMOOTH);
        Base::glDisable(GL_POLYGON_SMOOTH);
        Base::glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        return true;
    } else {
        // NOTE: Currently we can use OpenGL 2.1 to fake multisampling with point/line/polygon smoothing.
        // TODO: We can use OpenGL 3.x FBOs to do multisampling even if the default framebuffer doesn't support it.
        if (requestedSamples > 0) {
            Base::glEnable(GL_LINE_SMOOTH);
            Base::glDisable(GL_POLYGON_SMOOTH);
            Base::glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
            return true;
        } else {
            Base::glDisable(GL_LINE_SMOOTH);
            Base::glDisable(GL_POLYGON_SMOOTH);
            return false;
        }
    }
#else
    return false;
#endif
}

} // namespace Legacy
