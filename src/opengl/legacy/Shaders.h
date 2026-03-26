#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "AbstractShaderProgram.h"

#include <memory>

namespace Legacy {

struct NODISCARD AColorPlainShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~AColorPlainShader() final;

private:
    void virt_setUniforms(const glm::mat4 &mvp, const GLRenderState::Uniforms &uniforms) final
    {
        setColor("uColor", uniforms.color);
        setMatrix("uMVP", mvp);
    }
};

struct NODISCARD UColorPlainShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~UColorPlainShader() final;

private:
    void virt_setUniforms(const glm::mat4 &mvp, const GLRenderState::Uniforms &uniforms) final
    {
        setColor("uColor", uniforms.color);
        setMatrix("uMVP", mvp);
    }
};

struct NODISCARD AColorTexturedShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~AColorTexturedShader() final;

private:
    void virt_setUniforms(const glm::mat4 &mvp, const GLRenderState::Uniforms &uniforms) final
    {
        assert(uniforms.textures[0] != INVALID_MM_TEXTURE_ID);

        setColor("uColor", uniforms.color);
        setMatrix("uMVP", mvp);
        setTexture("uTexture", 0);
    }
};

struct NODISCARD RoomQuadTexShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~RoomQuadTexShader() final;

private:
    void virt_setUniforms(const glm::mat4 &mvp, const GLRenderState::Uniforms &uniforms) final
    {
        assert(uniforms.textures[0] != INVALID_MM_TEXTURE_ID);

        setColor("uColor", uniforms.color);
        setMatrix("uMVP", mvp);
        setTexture("uTexture", 0);
    }
};

struct NODISCARD UColorTexturedShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~UColorTexturedShader() final;

private:
    void virt_setUniforms(const glm::mat4 &mvp, const GLRenderState::Uniforms &uniforms) final
    {
        assert(uniforms.textures[0] != INVALID_MM_TEXTURE_ID);

        setColor("uColor", uniforms.color);
        setMatrix("uMVP", mvp);
        setTexture("uTexture", 0);
    }
};

struct NODISCARD FontShader final : public AbstractShaderProgram
{
private:
    using Base = AbstractShaderProgram;

public:
    using Base::AbstractShaderProgram;

    ~FontShader() final;

private:
    void virt_setUniforms(const glm::mat4 &mvp, const GLRenderState::Uniforms &uniforms) final
    {
        assert(uniforms.textures[0] != INVALID_MM_TEXTURE_ID);
        auto functions = Base::m_functions.lock();

        setMatrix("uMVP3D", mvp);
        setTexture("uFontTexture", 0);
        setViewport("uPhysViewport", deref(functions).getPhysicalViewport());
    }
};

struct NODISCARD PointShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~PointShader() final;

private:
    void virt_setUniforms(const glm::mat4 &mvp, const GLRenderState::Uniforms &uniforms) final
    {
        setColor("uColor", uniforms.color);
        setMatrix("uMVP", mvp);
    }
};

struct NODISCARD BlitShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~BlitShader() final;

private:
    void virt_setUniforms(const glm::mat4 & /*mvp*/,
                          const GLRenderState::Uniforms & /*uniforms*/) final
    {}
};

struct NODISCARD FullScreenShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~FullScreenShader() final;

private:
    void virt_setUniforms(const glm::mat4 & /*mvp*/, const GLRenderState::Uniforms &uniforms) final
    {
        setColor("uColor", uniforms.color);
    }
};

struct NODISCARD AtmosphereShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~AtmosphereShader() final;

private:
    void virt_setUniforms(const glm::mat4 & /*mvp*/,
                          const GLRenderState::Uniforms & /*uniforms*/) final
    {
        setTexture("uTexture", 0);
    }
};

struct NODISCARD TimeOfDayShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~TimeOfDayShader() final;

private:
    void virt_setUniforms(const glm::mat4 & /*mvp*/,
                          const GLRenderState::Uniforms & /*uniforms*/) final
    {}
};

struct NODISCARD ParticleSimulationShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~ParticleSimulationShader() final;

private:
    void virt_setUniforms(const glm::mat4 & /*mvp*/,
                          const GLRenderState::Uniforms & /*uniforms*/) final
    {}
};

struct NODISCARD ParticleRenderShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~ParticleRenderShader() final;

private:
    void virt_setUniforms(const glm::mat4 & /*mvp*/,
                          const GLRenderState::Uniforms & /*uniforms*/) final
    {}
};

/* owned by Functions */
struct NODISCARD ShaderPrograms final
{
private:
    Functions &m_functions;

private:
    std::shared_ptr<AColorPlainShader> m_aColorShader;
    std::shared_ptr<UColorPlainShader> m_uColorShader;
    std::shared_ptr<AColorTexturedShader> m_aTexturedShader;
    std::shared_ptr<UColorTexturedShader> m_uTexturedShader;

private:
    std::shared_ptr<RoomQuadTexShader> m_roomQuadTexShader;

private:
    std::shared_ptr<FontShader> m_font;
    std::shared_ptr<PointShader> m_point;
    std::shared_ptr<BlitShader> m_blit;
    std::shared_ptr<FullScreenShader> m_fullscreen;
    std::shared_ptr<AtmosphereShader> m_atmosphere;
    std::shared_ptr<TimeOfDayShader> m_timeOfDay;
    std::shared_ptr<ParticleSimulationShader> m_particleSimulation;
    std::shared_ptr<ParticleRenderShader> m_particleRender;

public:
    explicit ShaderPrograms(Functions &functions)
        : m_functions{functions}
    {}

    DELETE_CTORS_AND_ASSIGN_OPS(ShaderPrograms);

private:
    NODISCARD Functions &getFunctions() { return m_functions; }

public:
    void resetAll();

public:
    // attribute color (aka "Colored")
    NODISCARD const std::shared_ptr<AColorPlainShader> &getPlainAColorShader();
    // uniform color (aka "Plain")
    NODISCARD const std::shared_ptr<UColorPlainShader> &getPlainUColorShader();
    // attribute color + textured (aka "ColoredTextured")
    NODISCARD const std::shared_ptr<AColorTexturedShader> &getTexturedAColorShader();
    // uniform color + textured (aka "Textured")
    NODISCARD const std::shared_ptr<UColorTexturedShader> &getTexturedUColorShader();

public:
    NODISCARD const std::shared_ptr<RoomQuadTexShader> &getRoomQuadTexShader();

public:
    NODISCARD const std::shared_ptr<FontShader> &getFontShader();
    NODISCARD const std::shared_ptr<PointShader> &getPointShader();
    NODISCARD const std::shared_ptr<BlitShader> &getBlitShader();
    NODISCARD const std::shared_ptr<FullScreenShader> &getFullScreenShader();
    NODISCARD const std::shared_ptr<AtmosphereShader> &getAtmosphereShader();
    NODISCARD const std::shared_ptr<TimeOfDayShader> &getTimeOfDayShader();
    NODISCARD const std::shared_ptr<ParticleSimulationShader> &getParticleSimulationShader();
    NODISCARD const std::shared_ptr<ParticleRenderShader> &getParticleRenderShader();

public:
    void early_init();
};

} // namespace Legacy
