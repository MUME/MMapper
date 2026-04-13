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

#include <array>

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
public:
    static constexpr size_t NUM_BUFFERS = 2;

private:
    struct NODISCARD Buffers final
    {
        VAO vao;
        VBO vbo;

        Buffers() = default;
        ~Buffers() = default;
        DELETE_CTORS_AND_ASSIGN_OPS(Buffers);
    };

private:
    const SharedFunctions m_shared_functions;
    Functions &m_functions;
    const std::shared_ptr<ParticleSimulationShader> m_program;

    TFO m_tfo;
    std::array<Buffers, NUM_BUFFERS> m_buffers{};

    uint32_t m_numParticles = 512;
    uint8_t m_currentBuffer = 0;
    bool m_initialized = false;

public:
    explicit ParticleSimulationMesh(SharedFunctions shared_functions,
                                    std::shared_ptr<ParticleSimulationShader> program);
    ~ParticleSimulationMesh() override;

private:
    void init();
    void virt_clear() final {}
    void virt_reset() final;
    void virt_render(const GLRenderState &renderState) final;
    NODISCARD bool virt_isEmpty() const final { return !m_initialized; }

public:
    NODISCARD uint32_t getCurrentBuffer() const { return m_currentBuffer; }
    NODISCARD uint32_t getNextBuffer() const
    {
        return utils::circular_increment<NUM_BUFFERS>(m_currentBuffer);
    }
    // unused
    NODISCARD uint32_t getPreviousBuffer() const
    {
        return utils::circular_decrement<NUM_BUFFERS>(m_currentBuffer);
    }
    void advanceCurrentBuffer()
    {
        m_currentBuffer = utils::circular_increment<NUM_BUFFERS>(m_currentBuffer);
    }
    NODISCARD uint32_t getNumParticles() const { return m_numParticles; }
    NODISCARD VBO &getParticleVbo(const size_t index) { return m_buffers[index].vbo; }
};

class NODISCARD ParticleRenderMesh final : public IRenderable
{
private:
    const SharedFunctions m_shared_functions;
    Functions &m_functions;
    const std::shared_ptr<ParticleRenderShader> m_program;
    ParticleSimulationMesh &m_simulation;

    // This design is horrifying.
    static constexpr size_t NUM_BUFFERS = ParticleSimulationMesh::NUM_BUFFERS;
    std::array<VAO, NUM_BUFFERS> m_vaos{};

    float m_intensity = 0.0f;
    bool m_initialized = false;

public:
    explicit ParticleRenderMesh(SharedFunctions shared_functions,
                                std::shared_ptr<ParticleRenderShader> program,
                                ParticleSimulationMesh &simulation);
    ~ParticleRenderMesh() override;

    void setIntensity(float intensity);

private:
    void init();
    void virt_clear() final {}
    void virt_reset() final;
    NODISCARD bool virt_isEmpty() const final;
    void virt_render(const GLRenderState &renderState) final;
};

} // namespace Legacy
