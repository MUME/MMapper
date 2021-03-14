#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "AbstractShaderProgram.h"

namespace Legacy {

struct NODISCARD AColorPlainShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~AColorPlainShader() override;

protected:
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

    ~UColorPlainShader() override;

protected:
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

    ~AColorTexturedShader() override;

protected:
    void virt_setUniforms(const glm::mat4 &mvp, const GLRenderState::Uniforms &uniforms) final
    {
        assert(uniforms.textures[0]);

        setColor("uColor", uniforms.color);
        setMatrix("uMVP", mvp);
        setTexture("uTexture", 0);
    }
};

struct NODISCARD UColorTexturedShader final : public AbstractShaderProgram
{
public:
    using AbstractShaderProgram::AbstractShaderProgram;

    ~UColorTexturedShader() override;

protected:
    void virt_setUniforms(const glm::mat4 &mvp, const GLRenderState::Uniforms &uniforms) final
    {
        assert(uniforms.textures[0]);

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

    ~FontShader() override;

protected:
    void virt_setUniforms(const glm::mat4 &mvp, const GLRenderState::Uniforms &uniforms) final
    {
        assert(uniforms.textures[0]);
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

    ~PointShader() override;

protected:
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
    std::shared_ptr<AColorPlainShader> aColorShader;
    std::shared_ptr<UColorPlainShader> uColorShader;
    std::shared_ptr<AColorTexturedShader> aTexturedShader;
    std::shared_ptr<UColorTexturedShader> uTexturedShader;
    std::shared_ptr<FontShader> font;
    std::shared_ptr<PointShader> point;

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
        aColorShader.reset();
        uColorShader.reset();
        aTexturedShader.reset();
        uTexturedShader.reset();
        font.reset();
        point.reset();
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
