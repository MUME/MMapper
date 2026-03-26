#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../OpenGL.h"
#include "../OpenGLTypes.h"
#include "AttributeLessMeshes.h"
#include "Binders.h"
#include "Legacy.h"
#include "Shaders.h"
#include "TFO.h"
#include "VAO.h"
#include "VBO.h"

namespace Legacy {

class NODISCARD AtmosphereMesh final : public FullScreenMesh<AtmosphereShader>
{
public:
    explicit AtmosphereMesh(SharedFunctions sharedFunctions,
                            std::shared_ptr<AtmosphereShader> program);
    ~AtmosphereMesh() override;
};

class NODISCARD TimeOfDayMesh final : public FullScreenMesh<TimeOfDayShader>
{
public:
    using FullScreenMesh::FullScreenMesh;
    ~TimeOfDayMesh() override;
};

class NODISCARD ParticleSimulationMesh final : public IRenderable
{
private:
    const SharedFunctions m_shared_functions;
    Functions &m_functions;
    const std::shared_ptr<ParticleSimulationShader> m_program;

    TFO m_tfo;
    VBO m_vbos[2];
    VAO m_vaos[2];

    uint32_t m_currentBuffer = 0;
    uint32_t m_numParticles = 512;
    bool m_initialized = false;

public:
    explicit ParticleSimulationMesh(SharedFunctions shared_functions,
                                    std::shared_ptr<ParticleSimulationShader> program);
    ~ParticleSimulationMesh() override;

private:
    void init();
    void virt_clear() override {}
    void virt_reset() override;
    void virt_render(const GLRenderState &renderState) override;

public:
    NODISCARD bool virt_isEmpty() const override { return !m_initialized; }
    NODISCARD uint32_t getCurrentBuffer() const { return m_currentBuffer; }
    NODISCARD uint32_t getNumParticles() const { return m_numParticles; }
    NODISCARD const VBO &getParticleVbo(const uint32_t index) const { return m_vbos[index]; }
};

class NODISCARD ParticleRenderMesh final : public IRenderable
{
private:
    const SharedFunctions m_shared_functions;
    Functions &m_functions;
    const std::shared_ptr<ParticleRenderShader> m_program;
    const ParticleSimulationMesh &m_simulation;

    VAO m_vaos[2];
    float m_intensity = 0.0f;
    bool m_initialized = false;

public:
    explicit ParticleRenderMesh(SharedFunctions shared_functions,
                                std::shared_ptr<ParticleRenderShader> program,
                                const ParticleSimulationMesh &simulation);
    ~ParticleRenderMesh() override;

    void setIntensity(const float intensity) { m_intensity = intensity; }

private:
    void init();
    void virt_clear() override {}
    void virt_reset() override;
    NODISCARD bool virt_isEmpty() const override;
    void virt_render(const GLRenderState &renderState) override;
};

} // namespace Legacy
