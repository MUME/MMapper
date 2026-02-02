// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "FBO.h"

#include "../../global/logging.h"
#include "../OpenGLConfig.h"
#include "Legacy.h"

namespace Legacy {

bool LOG_FBO_ALLOCATIONS = true;

void FBO::configure(const Viewport &physicalViewport, int requestedSamples)
{
    // Unconditionally release old FBO to ensure a clean slate.
    m_multisamplingFbo.reset();

    const QSize physicalSize(physicalViewport.size.x, physicalViewport.size.y);
    if (physicalSize.isEmpty()) {
        if (LOG_FBO_ALLOCATIONS) {
            MMLOG_INFO() << "FBO destroyed (size empty)";
        }
        return;
    }

    // Only create the multisampling FBO if requested.
    if (requestedSamples > 0) {
        int actualSamples = std::min(requestedSamples, OpenGLConfig::getMaxSamples());

        if (actualSamples > 0) {
            QOpenGLFramebufferObjectFormat msFormat;
            msFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
            msFormat.setSamples(actualSamples);
            msFormat.setTextureTarget(GL_TEXTURE_2D_MULTISAMPLE);
            msFormat.setInternalTextureFormat(GL_RGBA8);

            m_multisamplingFbo = std::make_unique<QOpenGLFramebufferObject>(physicalSize, msFormat);
            if (!m_multisamplingFbo->isValid()) {
                if (LOG_FBO_ALLOCATIONS) {
                    MMLOG_ERROR()
                        << "Failed to create multisampling FBO. Falling back to no multisampling.";
                }
                m_multisamplingFbo.reset();
            } else if (LOG_FBO_ALLOCATIONS) {
                MMLOG_INFO() << "Created multisampling FBO with " << actualSamples << " samples.";
            }
        }
    }
}

void FBO::bind(const GLuint targetId, Functions &gl)
{
    if (m_multisamplingFbo) {
        m_multisamplingFbo->bind();
    } else {
        gl.glBindFramebuffer(GL_FRAMEBUFFER, targetId);
    }
}

void FBO::release()
{
    if (m_multisamplingFbo) {
        m_multisamplingFbo->release();
    }
}

void FBO::blitToTarget(const GLuint targetId, Functions &gl)
{
    if (!m_multisamplingFbo || !m_multisamplingFbo->isValid()) {
        return;
    }

    const GLsizei width = m_multisamplingFbo->width();
    const GLsizei height = m_multisamplingFbo->height();

    // Now resolve the multisampling FBO directly to the target framebuffer.
    gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, m_multisamplingFbo->handle());
    gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetId);
    gl.glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // Leave the target FBO bound.
    gl.glBindFramebuffer(GL_FRAMEBUFFER, targetId);
}

} // namespace Legacy
