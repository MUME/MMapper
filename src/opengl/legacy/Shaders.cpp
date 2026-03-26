// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "Shaders.h"

#include "../../global/TextUtils.h"
#include "ShaderUtils.h"

#include <memory>

#include <QFile>

NODISCARD static std::string readWholeResourceFile(const std::string &fullPath)
{
    QFile f{QString(fullPath.c_str())};
    if (!f.exists() || !f.open(QIODevice::OpenModeFlag::ReadOnly | QIODevice::OpenModeFlag::Text)) {
        throw std::runtime_error("error opening file");
    }
    QTextStream in(&f);
    return mmqt::toStdStringUtf8(in.readAll());
}

NODISCARD static ShaderUtils::Source readWholeShader(const std::string &dir, const std::string &name)
{
    const auto fullPathName = ":/shaders/legacy/" + dir + "/" + name;
    return ShaderUtils::Source{fullPathName, readWholeResourceFile(fullPathName)};
}

namespace Legacy {

AColorPlainShader::~AColorPlainShader() = default;
UColorPlainShader::~UColorPlainShader() = default;
AColorTexturedShader::~AColorTexturedShader() = default;
UColorTexturedShader::~UColorTexturedShader() = default;

RoomQuadTexShader::~RoomQuadTexShader() = default;

FontShader::~FontShader() = default;
PointShader::~PointShader() = default;
BlitShader::~BlitShader() = default;
FullScreenShader::~FullScreenShader() = default;
AtmosphereShader::~AtmosphereShader() = default;
TimeOfDayShader::~TimeOfDayShader() = default;
ParticleSimulationShader::~ParticleSimulationShader() = default;
ParticleRenderShader::~ParticleRenderShader() = default;

void ShaderPrograms::early_init()
{
    std::ignore = getPlainAColorShader();
    std::ignore = getPlainUColorShader();
    std::ignore = getTexturedAColorShader();
    std::ignore = getTexturedUColorShader();

    std::ignore = getRoomQuadTexShader();

    std::ignore = getFontShader();
    std::ignore = getPointShader();
    std::ignore = getBlitShader();
    std::ignore = getFullScreenShader();
    std::ignore = getAtmosphereShader();
    std::ignore = getTimeOfDayShader();
    std::ignore = getParticleSimulationShader();
    std::ignore = getParticleRenderShader();
}

void ShaderPrograms::resetAll()
{
    m_aColorShader.reset();
    m_uColorShader.reset();
    m_aTexturedShader.reset();
    m_uTexturedShader.reset();

    m_roomQuadTexShader.reset();

    m_font.reset();
    m_point.reset();
    m_blit.reset();
    m_fullscreen.reset();
    m_atmosphere.reset();
    m_timeOfDay.reset();
    m_particleSimulation.reset();
    m_particleRender.reset();
}

// essentially a private member of ShaderPrograms
template<typename T>
NODISCARD static std::shared_ptr<T> loadSimpleShaderProgram(Functions &functions,
                                                            const std::string &dir)
{
    static_assert(std::is_base_of_v<AbstractShaderProgram, T>);

    const auto getSource = [&dir](const std::string &name) -> ShaderUtils::Source {
        return ::readWholeShader(dir, name);
    };

    auto program = ShaderUtils::loadShaders(functions,
                                            getSource("vert.glsl"),
                                            getSource("frag.glsl"));
    return std::make_shared<T>(dir, functions.shared_from_this(), std::move(program));
}

// essentially a private member of ShaderPrograms
template<typename T>
NODISCARD static const std::shared_ptr<T> &getInitialized(std::shared_ptr<T> &shader,
                                                          Functions &functions,
                                                          const std::string &dir)
{
    if (!shader) {
        shader = Legacy::loadSimpleShaderProgram<T>(functions, dir);
    }
    return shader;
}

const std::shared_ptr<AColorPlainShader> &ShaderPrograms::getPlainAColorShader()
{
    return getInitialized<AColorPlainShader>(m_aColorShader, getFunctions(), "plain/acolor");
}

const std::shared_ptr<UColorPlainShader> &ShaderPrograms::getPlainUColorShader()
{
    return getInitialized<UColorPlainShader>(m_uColorShader, getFunctions(), "plain/ucolor");
}

const std::shared_ptr<AColorTexturedShader> &ShaderPrograms::getTexturedAColorShader()
{
    return getInitialized<AColorTexturedShader>(m_aTexturedShader, getFunctions(), "tex/acolor");
}

const std::shared_ptr<RoomQuadTexShader> &ShaderPrograms::getRoomQuadTexShader()
{
    return getInitialized<RoomQuadTexShader>(m_roomQuadTexShader, getFunctions(), "room/tex/acolor");
}

const std::shared_ptr<UColorTexturedShader> &ShaderPrograms::getTexturedUColorShader()
{
    return getInitialized<UColorTexturedShader>(m_uTexturedShader, getFunctions(), "tex/ucolor");
}

const std::shared_ptr<FontShader> &ShaderPrograms::getFontShader()
{
    return getInitialized<FontShader>(m_font, getFunctions(), "font");
}

const std::shared_ptr<PointShader> &ShaderPrograms::getPointShader()
{
    return getInitialized<PointShader>(m_point, getFunctions(), "point");
}

const std::shared_ptr<BlitShader> &ShaderPrograms::getBlitShader()
{
    return getInitialized<BlitShader>(m_blit, getFunctions(), "blit");
}

const std::shared_ptr<FullScreenShader> &ShaderPrograms::getFullScreenShader()
{
    return getInitialized<FullScreenShader>(m_fullscreen, getFunctions(), "fullscreen");
}

const std::shared_ptr<AtmosphereShader> &ShaderPrograms::getAtmosphereShader()
{
    return getInitialized<AtmosphereShader>(m_atmosphere, getFunctions(), "weather/atmosphere");
}

const std::shared_ptr<TimeOfDayShader> &ShaderPrograms::getTimeOfDayShader()
{
    return getInitialized<TimeOfDayShader>(m_timeOfDay, getFunctions(), "weather/timeofday");
}

const std::shared_ptr<ParticleSimulationShader> &ShaderPrograms::getParticleSimulationShader()
{
    if (!m_particleSimulation) {
        Functions &funcs = getFunctions();
        const auto getSource = [&funcs](const std::string &path) -> ShaderUtils::Source {
            const auto fullPathName = ":/shaders/legacy/weather/simulation/" + path;
            std::string src;
            try {
                src = ::readWholeResourceFile(fullPathName);
            } catch (...) {
                // Fallback for frag.glsl if file missing, as some drivers require it
                if (path == "frag.glsl") {
                    src = std::string(funcs.getShaderVersion())
                          + "\nprecision mediump float;\nvoid main() {}\n";
                } else {
                    throw;
                }
            }
            return ShaderUtils::Source{fullPathName, src};
        };

        std::vector<const char *> varyings = {"vPos", "vLife"};
        auto program = ShaderUtils::loadTransformFeedbackShaders(funcs,
                                                                 getSource("vert.glsl"),
                                                                 getSource("frag.glsl"),
                                                                 varyings);
        m_particleSimulation = std::make_shared<ParticleSimulationShader>("weather/simulation",
                                                                          funcs.shared_from_this(),
                                                                          std::move(program));
    }
    return m_particleSimulation;
}

const std::shared_ptr<ParticleRenderShader> &ShaderPrograms::getParticleRenderShader()
{
    return getInitialized<ParticleRenderShader>(m_particleRender,
                                                getFunctions(),
                                                "weather/particle");
}

} // namespace Legacy
