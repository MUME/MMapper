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
    for (int i = 0; i < 2; ++i) {
        m_vbos[i].emplace(m_shared_functions);
        m_vaos[i].emplace(m_shared_functions);
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

    for (int i = 0; i < 2; ++i) {
        if (!m_vbos[i]) {
            m_vbos[i].emplace(m_shared_functions);
        }
        m_functions.glBindBuffer(GL_ARRAY_BUFFER, m_vbos[i].get());
        m_functions.glBufferData(GL_ARRAY_BUFFER,
                                 static_cast<GLsizeiptr>(initialData.size()
                                                         * sizeof(WeatherParticleVert)),
                                 initialData.data(),
                                 GL_STREAM_DRAW);

        if (!m_vaos[i]) {
            m_vaos[i].emplace(m_shared_functions);
        }
        m_functions.glBindVertexArray(m_vaos[i].get());
        m_functions.enableAttrib(0,
                                 2,
                                 GL_FLOAT,
                                 GL_FALSE,
                                 sizeof(WeatherParticleVert),
                                 reinterpret_cast<void *>(offsetof(WeatherParticleVert, pos)));
        m_functions.enableAttrib(1,
                                 1,
                                 GL_FLOAT,
                                 GL_FALSE,
                                 sizeof(WeatherParticleVert),
                                 reinterpret_cast<void *>(offsetof(WeatherParticleVert, life)));
        m_functions.glBindVertexArray(0);
        m_functions.glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    m_initialized = true;
}

void ParticleSimulationMesh::virt_reset()
{
    // Note: TFO is not reset here as it's only created in the constructor
    // and can be reused.
    for (int i = 0; i < 2; ++i) {
        m_vbos[i].reset();
        m_vaos[i].reset();
    }
    m_initialized = false;
}

void ParticleSimulationMesh::virt_render(const GLRenderState &renderState)
{
    init();

    auto binder = m_program->bind();
    const glm::mat4 mvp = renderState.mvp.value_or(m_functions.getProjectionMatrix());
    m_program->setUniforms(mvp, renderState.uniforms);

    const uint32_t bufferOut = 1 - m_currentBuffer;

    m_functions.glBindVertexArray(m_vaos[m_currentBuffer].get());
    m_functions.glEnable(GL_RASTERIZER_DISCARD);
    m_functions.glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tfo.get());
    m_functions.glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_vbos[bufferOut].get());

    {
        m_functions.glBeginTransformFeedback(GL_POINTS);
        m_functions.glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_numParticles));
        m_functions.glEndTransformFeedback();
    }
    m_functions.glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
    m_functions.glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
    m_functions.glDisable(GL_RASTERIZER_DISCARD);
    m_functions.glBindVertexArray(0);

    m_currentBuffer = bufferOut;
}

ParticleRenderMesh::ParticleRenderMesh(SharedFunctions shared_functions,
                                       std::shared_ptr<ParticleRenderShader> program,
                                       const ParticleSimulationMesh &simulation)
    : m_shared_functions(std::move(shared_functions))
    , m_functions(deref(m_shared_functions))
    , m_program(std::move(program))
    , m_simulation(simulation)
{
    for (int i = 0; i < 2; ++i) {
        m_vaos[i].emplace(m_shared_functions);
    }
}

ParticleRenderMesh::~ParticleRenderMesh() = default;

void ParticleRenderMesh::init()
{
    if (m_initialized || m_simulation.virt_isEmpty()) {
        return;
    }

    for (uint32_t i = 0; i < 2; ++i) {
        if (!m_vaos[i]) {
            m_vaos[i].emplace(m_shared_functions);
        }
        m_functions.glBindVertexArray(m_vaos[i].get());

        m_functions.glBindBuffer(GL_ARRAY_BUFFER, m_simulation.getParticleVbo(i).get());
        m_functions.enableAttrib(0,
                                 2,
                                 GL_FLOAT,
                                 GL_FALSE,
                                 sizeof(WeatherParticleVert),
                                 reinterpret_cast<void *>(offsetof(WeatherParticleVert, pos)));
        m_functions.glVertexAttribDivisor(0, 1);
        m_functions.enableAttrib(1,
                                 1,
                                 GL_FLOAT,
                                 GL_FALSE,
                                 sizeof(WeatherParticleVert),
                                 reinterpret_cast<void *>(offsetof(WeatherParticleVert, life)));
        m_functions.glVertexAttribDivisor(1, 1);
    }

    m_functions.glBindVertexArray(0);
    m_functions.glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_initialized = true;
}

void ParticleRenderMesh::virt_reset()
{
    for (int i = 0; i < 2; ++i) {
        m_vaos[i].reset();
    }
    m_initialized = false;
}

bool ParticleRenderMesh::virt_isEmpty() const
{
    return m_simulation.virt_isEmpty();
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
    const GLsizei maxCount = static_cast<GLsizei>(m_simulation.getNumParticles());
    const GLsizei count = std::max<GLsizei>(1,
                                            static_cast<GLsizei>(m_intensity
                                                                 * static_cast<float>(maxCount)));

    auto binder = m_program->bind();
    const glm::mat4 mvp = renderState.mvp.value_or(m_functions.getProjectionMatrix());
    m_program->setUniforms(mvp, renderState.uniforms);

    RenderStateBinder rsBinder(m_functions, m_functions.getTexLookup(), renderState);

    const uint32_t bufferIdx = m_simulation.getCurrentBuffer();
    m_functions.glBindVertexArray(m_vaos[bufferIdx].get());
    m_functions.glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, count);
    m_functions.glBindVertexArray(0);
}

} // namespace Legacy
