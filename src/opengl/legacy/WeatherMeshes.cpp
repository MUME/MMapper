// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "WeatherMeshes.h"

#include "../../global/random.h"
#include "../Weather.h"
#include "AttributeLessMeshes.h"
#include "Binders.h"

namespace Legacy {

AtmosphereMesh::AtmosphereMesh(SharedFunctions sharedFunctions,
                               std::shared_ptr<AtmosphereShader> program)
    : FullScreenMesh(std::move(sharedFunctions), std::move(program), GL_TRIANGLE_STRIP, 4)
{}

AtmosphereMesh::~AtmosphereMesh() = default;

TimeOfDayMesh::~TimeOfDayMesh() = default;

WeatherParticleMesh::WeatherParticleMesh(SharedFunctions sharedFunctions,
                                         std::shared_ptr<ParticleSimulationShader> simProgram,
                                         std::shared_ptr<ParticleRenderShader> renderProgram)
    : m_shared_functions(std::move(sharedFunctions))
    , m_functions(deref(m_shared_functions))
    , m_simProgram(std::move(simProgram))
    , m_renderProgram(std::move(renderProgram))
{
    m_tfo.emplace(m_shared_functions);
    for (auto &slot : m_slots) {
        slot.vbo.emplace(m_shared_functions);
        slot.simVao.emplace(m_shared_functions);
        slot.renderVao.emplace(m_shared_functions);
    }

    // Both shader programs are already valid here, so there's no reason to defer setup to the
    // first render call (the old code did, and that lazy-init-during-render was itself one of
    // the things flagged as a problem).
    init();
}

WeatherParticleMesh::~WeatherParticleMesh() = default;

void WeatherParticleMesh::setIntensity(const float intensity)
{
    if (!std::isfinite(intensity)) {
        assert(std::isfinite(intensity));
        m_intensity = 0.f;
        return;
    }

    assert(isClamped(intensity, 0.f, 1.f));
    m_intensity = std::clamp(intensity, 0.f, 1.f);
}

void WeatherParticleMesh::init()
{
    if (m_initialized) {
        return;
    }

    auto get_random_float = []() { return static_cast<float>(getRandom(1000000)) / 1000000.0f; };

    std::vector<WeatherParticleVert> initialData;
    initialData.reserve(m_numParticles);
    for (size_t i = 0; i < m_numParticles; ++i) {
        initialData.emplace_back(glm::vec2(get_random_float() * WeatherConstants::WEATHER_EXTENT
                                               - WeatherConstants::WEATHER_RADIUS,
                                           get_random_float() * WeatherConstants::WEATHER_EXTENT
                                               - WeatherConstants::WEATHER_RADIUS),
                                 get_random_float());
    }

    if constexpr (IS_DEBUG_BUILD) {
        auto &simProg = deref(m_simProgram);
        MAYBE_UNUSED auto simProg_binder = simProg.bind();
        assert(simProg.getAttribLocation("aPos") == 0);
        assert(simProg.getAttribLocation("aLife") == 1);

        auto &renderProg = deref(m_renderProgram);
        MAYBE_UNUSED auto renderProg_binder = renderProg.bind();
        assert(renderProg.getAttribLocation("aParticlePos") == 0);
        assert(renderProg.getAttribLocation("aLife") == 1);
    }

    using Vert = WeatherParticleVert;
    auto &gl = m_functions;
    for (auto &slot : m_slots) {
        std::ignore = slot.vbo.setVbo(GL_ARRAY_BUFFER,
                                      View<Vert>{initialData},
                                      BufferUsageEnum::STREAM_DRAW);

        MAYBE_UNUSED auto vbo_binder = slot.vbo.bind(GL_ARRAY_BUFFER);

        // Transform-feedback input: per-vertex-rate attributes.
        {
            MAYBE_UNUSED auto vao_binder = slot.simVao.bind();
            gl.enableAttrib(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), offsetof(Vert, pos));
            gl.enableAttrib(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vert), offsetof(Vert, life));
        }

        // Instanced-draw input: same buffer, but per-instance-rate attributes.
        {
            MAYBE_UNUSED auto vao_binder = slot.renderVao.bind();
            gl.enableAttrib(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), offsetof(Vert, pos));
            gl.glVertexAttribDivisor(0, 1);
            gl.enableAttrib(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vert), offsetof(Vert, life));
            gl.glVertexAttribDivisor(1, 1);
        }
    }

    m_initialized = true;
}

void WeatherParticleMesh::virt_reset()
{
    // Note: TFO is not reset here as it's only created in the constructor
    // and can be reused.
    for (auto &slot : m_slots) {
        slot.vbo.reset();
        slot.simVao.reset();
        slot.renderVao.reset();
    }
    m_initialized = false;
}

// low priority: if this code gets duplicated elsewhere, then we'd want to consider moving
// this function into the Functions class.
static void invokeTransformFeedback(Functions &gl, TFO &tfo, VAO &vao, VBO &vbo, const size_t count)
{
    GLuint index = 0;
    GLint first = 0;
    MAYBE_UNUSED auto vao_binder = vao.bind();
    MAYBE_UNUSED auto tfo_binder = tfo.bind();
    MAYBE_UNUSED auto vbo_binder = vbo.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, index);
    {
        // low priority: should we consider using RAII enable/diable?
        gl.glEnable(GL_RASTERIZER_DISCARD);
        {
            // low priority: should we consider separate transform feedback draw function?
            gl.glBeginTransformFeedback(GL_POINTS);
            gl.glDrawArrays(GL_POINTS, first, static_cast<GLsizei>(count));
            gl.glEndTransformFeedback();
        }
        gl.glDisable(GL_RASTERIZER_DISCARD);
    }
}

void WeatherParticleMesh::virt_render(const GLRenderState &renderState)
{
    assert(m_initialized && "init() must be called by GLWeather::initMeshes() before rendering");

    // 1. Simulate: read this frame's settled data from m_currentInput, write the newly
    // simulated particles into the other slot via transform feedback.
    {
        // ParticleSimulationShader::virt_setUniforms() ignores mvp entirely; the transform
        // feedback pass needs no projection (gl_Position is written but never rasterized,
        // since GL_RASTERIZER_DISCARD is enabled inside invokeTransformFeedback()). The
        // fallback value below is computed only to satisfy setUniforms()'s signature.
        const glm::mat4 mvp = renderState.mvp.value_or(m_functions.getProjectionMatrix());
        auto &prog = deref(m_simProgram);
        MAYBE_UNUSED auto binder = prog.bind();
        prog.setUniforms(mvp, renderState.uniforms);

        auto &vao = m_slots[m_currentInput].simVao;
        auto &outputVbo = m_slots[getOutputIndex()].vbo;
        invokeTransformFeedback(m_functions, m_tfo, vao, outputVbo, m_numParticles);
    }

    // 2. Draw: render from m_currentInput, the slot simulated *last* frame. This is the
    // already-settled data, never the buffer transform feedback just wrote this frame, so the
    // draw never has to wait on a same-frame write-then-read GPU hazard.
    if (m_intensity > 0.0f) {
        // Thinning: use precipitation intensity to drive instance count.
        const auto maxCount = static_cast<GLsizei>(m_numParticles);
        const auto count = std::max<GLsizei>(1,
                                             static_cast<GLsizei>(m_intensity
                                                                  * static_cast<float>(maxCount)));

        const glm::mat4 mvp = renderState.mvp.value_or(m_functions.getProjectionMatrix());
        auto &prog = deref(m_renderProgram);
        MAYBE_UNUSED auto binder = prog.bind();
        prog.setUniforms(mvp, renderState.uniforms);

        MAYBE_UNUSED auto rsBinder = RenderStateBinder{m_functions,
                                                       m_functions.getTexLookup(),
                                                       renderState};

        MAYBE_UNUSED auto vao_binder = m_slots[m_currentInput].renderVao.bind();
        m_functions.glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, count);
    }

    // 3. Advance: the slot we just finished simulating into becomes next frame's input.
    m_currentInput = static_cast<uint8_t>(getOutputIndex());
}

} // namespace Legacy
