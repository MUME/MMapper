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
        setVec3("uPlayerPos", uniforms.playerPos);
        setInt("uCurrentLayer", uniforms.currentLayer);
        setInt("uEnableRadial", uniforms.enableRadialTransparency ? 1 : 0);
        setInt("uIsNight", uniforms.isNight ? 1 : 0);
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
        setVec3("uPlayerPos", uniforms.playerPos);
        setInt("uCurrentLayer", uniforms.currentLayer);
        setInt("uEnableRadial", uniforms.enableRadialTransparency ? 1 : 0);
        setInt("uTexturesDisabled", uniforms.texturesDisabled ? 1 : 0);
        setInt("uIsNight", uniforms.isNight ? 1 : 0);
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
        setVec3("uPlayerPos", uniforms.playerPos);
        setInt("uCurrentLayer", uniforms.currentLayer);
        setInt("uEnableRadial", uniforms.enableRadialTransparency ? 1 : 0);
        setInt("uIsNight", uniforms.isNight ? 1 : 0);
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

/* owned by Functions */
struct NODISCARD ShaderPrograms final
{
private:
    Functions &m_functions;
    std::shared_ptr<AColorPlainShader> m_aColorShader;
    std::shared_ptr<UColorPlainShader> m_uColorShader;
    std::shared_ptr<AColorTexturedShader> m_aTexturedShader;
    std::shared_ptr<UColorTexturedShader> m_uTexturedShader;
    std::shared_ptr<FontShader> m_font;
    std::shared_ptr<PointShader> m_point;

public:
    explicit ShaderPrograms(Functions &functions)
        : m_functions{functions}
    {}

    DELETE_CTORS_AND_ASSIGN_OPS(ShaderPrograms);

private:
    NODISCARD Functions &getFunctions() { return m_functions; }

public:
    void resetAll()
    {
        m_aColorShader.reset();
        m_uColorShader.reset();
        m_aTexturedShader.reset();
        m_uTexturedShader.reset();
        m_font.reset();
        m_point.reset();
    }

public:
    // attribute color (aka "Colored")
    NODISCARD const std::shared_ptr<AColorPlainShader> &getPlainAColorShader();
    // uniform color (aka "Plain")
    NODISCARD const std::shared_ptr<UColorPlainShader> &getPlainUColorShader();
    // attribute color + textured (aka "ColoredTextured")
    NODISCARD const std::shared_ptr<AColorTexturedShader> &getTexturedAColorShader();
    // uniform color + textured (aka "Textured")
    NODISCARD const std::shared_ptr<UColorTexturedShader> &getTexturedUColorShader();
    NODISCARD const std::shared_ptr<FontShader> &getFontShader();
    NODISCARD const std::shared_ptr<PointShader> &getPointShader();
};

} // namespace Legacy
