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

ParticleSimulationMesh::ParticleSimulationMesh(SharedFunctions shared_functions,
                                               std::shared_ptr<ParticleSimulationShader> program)
    : m_shared_functions(std::move(shared_functions))
    , m_functions(deref(m_shared_functions))
    , m_program(std::move(program))
{
    m_tfo.emplace(m_shared_functions);
    for (auto &bufs : m_buffers) {
        bufs.vao.emplace(m_shared_functions);
        bufs.vbo.emplace(m_shared_functions);
    }
}

ParticleSimulationMesh::~ParticleSimulationMesh() = default;

void ParticleSimulationMesh::init()
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
        auto &prog = deref(m_program);
        MAYBE_UNUSED auto prog_binder = prog.bind();
        assert(prog.getAttribLocation("aPos") == 0);
        assert(prog.getAttribLocation("aLife") == 1);
    }

    for (auto &bufs : m_buffers) {
        auto &vao = bufs.vao;
        auto &vbo = bufs.vbo;
        using Vert = WeatherParticleVert;
        std::ignore = vbo.setVbo(GL_ARRAY_BUFFER,
                                 View<Vert>{initialData},
                                 BufferUsageEnum::STREAM_DRAW);

        MAYBE_UNUSED auto vbo_binder = vbo.bind(GL_ARRAY_BUFFER);
        MAYBE_UNUSED auto vao_binder = vao.bind();
        m_functions.enableAttrib(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), offsetof(Vert, pos));
        m_functions.enableAttrib(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vert), offsetof(Vert, life));
    }

    m_initialized = true;
}

void ParticleSimulationMesh::virt_reset()
{
    // Note: TFO is not reset here as it's only created in the constructor
    // and can be reused.
    for (auto &bufs : m_buffers) {
        bufs.vao.reset();
        bufs.vbo.reset();
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

void ParticleSimulationMesh::virt_render(const GLRenderState &renderState)
{
    if (!m_initialized) {
        // REVISIT: Why is this function allowed to be called before it's initialized?
        // Initializing and rendering in the same frame is usually really bad for performance.
        init();
        assert(m_initialized);
    }

    // REVISIT: please explain why this uses value_or().
    const glm::mat4 mvp = renderState.mvp.value_or(m_functions.getProjectionMatrix());
    auto &prog = deref(m_program);

    MAYBE_UNUSED auto binder = prog.bind();
    prog.setUniforms(mvp, renderState.uniforms);

    // REVISIT: Why are the VAO and VBO out of sync? Arrrrrrrrgh.
    auto &vao = m_buffers[getCurrentBuffer()].vao;
    auto &vbo = m_buffers[getNextBuffer()].vbo;
    invokeTransformFeedback(m_functions, m_tfo, vao, vbo, m_numParticles);
    advanceCurrentBuffer();
}

ParticleRenderMesh::ParticleRenderMesh(SharedFunctions shared_functions,
                                       std::shared_ptr<ParticleRenderShader> program,
                                       ParticleSimulationMesh &simulation)
    : m_shared_functions(std::move(shared_functions))
    , m_functions(deref(m_shared_functions))
    , m_program(std::move(program))
    , m_simulation(simulation)
{
    for (auto &vao : m_vaos) {
        vao.emplace(m_shared_functions);
    }
}

ParticleRenderMesh::~ParticleRenderMesh() = default;

void ParticleRenderMesh::setIntensity(const float intensity)
{
    if (!std::isfinite(intensity)) {
        assert(std::isfinite(intensity));
        m_intensity = 0.f;
        return;
    }

    assert(isClamped(intensity, 0.f, 1.f));
    m_intensity = std::clamp(intensity, 0.f, 1.f);
}

void ParticleRenderMesh::init()
{
    if (m_initialized || m_simulation.isEmpty()) {
        return;
    }

    if constexpr (IS_DEBUG_BUILD) {
        auto &prog = deref(m_program);
        MAYBE_UNUSED auto prog_binder = prog.bind();
        assert(prog.getAttribLocation("aParticlePos") == 0);
        assert(prog.getAttribLocation("aLife") == 1);
    }

    auto &gl = m_functions;
    for (size_t i = 0; i < NUM_BUFFERS; ++i) {
        using Vert = WeatherParticleVert;
        auto &vao = m_vaos[i];
        MAYBE_UNUSED auto vao_binder = vao.bind();
        MAYBE_UNUSED auto vbo_binder = m_simulation.getParticleVbo(i).bind(GL_ARRAY_BUFFER);
        gl.enableAttrib(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vert), offsetof(Vert, pos));
        gl.glVertexAttribDivisor(0, 1);
        gl.enableAttrib(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vert), offsetof(Vert, life));
        gl.glVertexAttribDivisor(1, 1);
    }

    m_initialized = true;
}

void ParticleRenderMesh::virt_reset()
{
    for (auto &vao : m_vaos) {
        vao.reset();
    }
    m_initialized = false;
}

bool ParticleRenderMesh::virt_isEmpty() const
{
    return m_simulation.isEmpty();
}

void ParticleRenderMesh::virt_render(const GLRenderState &renderState)
{
    init();
    if (!m_initialized) {
        return;
    }

    // Thinning: use precipitation intensity to drive instance count
    if (m_intensity <= 0.0f) {
        return;
    }
    const auto maxCount = static_cast<GLsizei>(m_simulation.getNumParticles());
    const auto count = std::max<GLsizei>(1,
                                         static_cast<GLsizei>(m_intensity
                                                              * static_cast<float>(maxCount)));

    // REVISIT: please explain why this uses value_or().
    const glm::mat4 mvp = renderState.mvp.value_or(m_functions.getProjectionMatrix());

    auto &prog = deref(m_program);
    MAYBE_UNUSED auto binder = prog.bind();
    prog.setUniforms(mvp, renderState.uniforms);

    MAYBE_UNUSED auto rsBinder = RenderStateBinder{m_functions,
                                                   m_functions.getTexLookup(),
                                                   renderState};

    const uint32_t bufferIdx = m_simulation.getCurrentBuffer();
    MAYBE_UNUSED auto vao_binder = m_vaos[bufferIdx].bind();
    m_functions.glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, count);
}

} // namespace Legacy
