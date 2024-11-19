#include "Legacy.h"

#include <optional>

namespace Legacy {

bool Functions::canRenderQuads()
{
    return true;
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
    case DrawModeEnum::QUADS:
        return GL_QUADS;
    case DrawModeEnum::INVALID:
        break;
    }

    return std::nullopt;
}

const char *Functions::getShaderVersion()
{
    return "#version 110\n\n";
}

void Functions::enableProgramPointSize(const bool enable)
{
    if (enable)
        Base::glEnable(GL_PROGRAM_POINT_SIZE);
    else
        Base::glDisable(GL_PROGRAM_POINT_SIZE);
}

bool Functions::tryEnableMultisampling(const int requestedSamples)
{
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

        Base::glEnable(GL_POINT_SMOOTH);
        Base::glEnable(GL_LINE_SMOOTH);
        Base::glDisable(GL_POLYGON_SMOOTH);
        Base::glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        Base::glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        return true;
    } else {
        // NOTE: Currently we can use OpenGL 2.1 to fake multisampling with point/line/polygon smoothing.
        // TODO: We can use OpenGL 3.x FBOs to do multisampling even if the default framebuffer doesn't support it.
        if (requestedSamples > 0) {
            Base::glEnable(GL_POINT_SMOOTH);
            Base::glEnable(GL_LINE_SMOOTH);
            Base::glDisable(GL_POLYGON_SMOOTH);
            Base::glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
            Base::glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
            return true;
        } else {
            Base::glDisable(GL_POINT_SMOOTH);
            Base::glDisable(GL_LINE_SMOOTH);
            Base::glDisable(GL_POLYGON_SMOOTH);
            return false;
        }
    }
}

} // namespace Legacy
