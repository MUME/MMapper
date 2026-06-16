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

// Rain/snow particles drifting in a toroidal region around the player
// (WeatherConstants::WEATHER_RADIUS / WEATHER_EXTENT), simulated entirely on the GPU via
// transform feedback: ParticleSimulationShader advances each particle's position/life every
// frame and writes the result back via TF, with no CPU readback.
//
// Transform feedback cannot read and write the same buffer in one draw call, so simulation
// ping-pongs between NUM_BUFFERS VBO "slots": each frame reads one slot's settled data and
// writes into a different slot. Each slot also needs two VAOs over the same VBO: the
// transform-feedback input needs per-vertex-rate attributes, while the later instanced draw
// needs per-instance-rate attributes (divisor=1). That second VAO is a real OpenGL requirement,
// not an indexing bug.
//
// TODO: NUM_BUFFERS=2 was kept because m_numParticles is only 512 -- a transform-feedback write
// of 512 points plus an instanced draw of 512 quads is a tiny GPU workload, so any pipeline
// stall from the write-then-read dependency this ping-pong still has *within* a single
// virt_render() call (simulate, then draw from the *other*, already-settled slot -- see
// virt_render()'s comments) is likely immeasurable. If weather rendering is ever profiled and
// shown to stall:
//   1. Bump NUM_BUFFERS to 3 here (everything below is already generic over N).
//   2. Change virt_render()'s draw step to read from the slot that was current *two* frames
//      ago instead of *m_currentInput* (last frame's), so the GPU has a full extra frame of
//      slack between the TF write and the dependent draw landing in the command stream.
//   3. Measure with an external GPU profiler (RenderDoc / Xcode GPU capture / Nsight) -- there
//      is no in-repo GPU timer-query plumbing (no QueryObject/glGenQueries), and GL_TIME_ELAPSED
//      isn't core in the ES3.0 backend this project also targets, so don't add one just for
//      this; eyeball frame time and stutter instead.
//   4. This adds a second frame of staleness to rendered particle positions on top of the one
//      this class already has. Confirm that's still visually fine for rain/snow before keeping it.
class NODISCARD WeatherParticleMesh final : public IRenderable
{
public:
    // Bump to 3 to A/B test cross-frame write-after-read slack vs. the default ping-pong.
    static constexpr size_t NUM_BUFFERS = 2;

private:
    struct NODISCARD Slot final
    {
        VBO vbo;       // particle data {pos, life}, ping-ponged
        VAO simVao;    // vertex-rate attribs over vbo; transform-feedback input
        VAO renderVao; // instance-rate attribs (divisor=1) over vbo; instanced-draw input

        Slot() = default;
        ~Slot() = default;
        DELETE_CTORS_AND_ASSIGN_OPS(Slot);
    };

private:
    const SharedFunctions m_shared_functions;
    Functions &m_functions;
    const std::shared_ptr<ParticleSimulationShader> m_simProgram;
    const std::shared_ptr<ParticleRenderShader> m_renderProgram;

    TFO m_tfo;
    std::array<Slot, NUM_BUFFERS> m_slots{};

    uint32_t m_numParticles = 512;
    uint8_t m_currentInput = 0; // slot holding this frame's fully-simulated, safe-to-render data
    float m_intensity = 0.0f;
    bool m_initialized = false;

public:
    explicit WeatherParticleMesh(SharedFunctions sharedFunctions,
                                 std::shared_ptr<ParticleSimulationShader> simProgram,
                                 std::shared_ptr<ParticleRenderShader> renderProgram);
    ~WeatherParticleMesh() override;

    void setIntensity(float intensity);

private:
    void init();
    void virt_clear() final {}
    void virt_reset() final;
    void virt_render(const GLRenderState &renderState) final;
    NODISCARD bool virt_isEmpty() const final { return !m_initialized; }
    NODISCARD uint32_t getOutputIndex() const
    {
        return utils::circular_increment<NUM_BUFFERS>(m_currentInput);
    }
};

} // namespace Legacy
