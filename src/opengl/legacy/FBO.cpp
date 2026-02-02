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
    // Unconditionally release old FBOs to ensure a clean slate.
    m_multisamplingFbo.reset();
    m_resolvedFbo.reset();

    const QSize physicalSize(physicalViewport.size.x, physicalViewport.size.y);
    if (physicalSize.isEmpty()) {
        if (LOG_FBO_ALLOCATIONS) {
            MMLOG_INFO() << "FBOs destroyed (size empty)";
        }
        return;
    }

    // Always create the resolved FBO. This is our target for MSAA resolve
    // and the primary render target if MSAA is disabled.
    QOpenGLFramebufferObjectFormat resolvedFormat;
    resolvedFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    resolvedFormat.setSamples(0);
    resolvedFormat.setTextureTarget(GL_TEXTURE_2D);
    resolvedFormat.setInternalTextureFormat(GL_RGBA8);

    m_resolvedFbo = std::make_unique<QOpenGLFramebufferObject>(physicalSize, resolvedFormat);
    if (!m_resolvedFbo->isValid()) {
        m_resolvedFbo.reset();
        throw std::runtime_error("Failed to create resolved FBO");
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

void FBO::bind()
{
    QOpenGLFramebufferObject *fboToBind = m_multisamplingFbo ? m_multisamplingFbo.get()
                                                             : m_resolvedFbo.get();
    if (fboToBind) {
        fboToBind->bind();
    }
}

void FBO::release()
{
    QOpenGLFramebufferObject *fboToRelease = m_multisamplingFbo ? m_multisamplingFbo.get()
                                                                : m_resolvedFbo.get();
    if (fboToRelease) {
        fboToRelease->release();
    }
}

void FBO::blitToTarget(const GLuint targetId, Functions &gl)
{
    if (!m_resolvedFbo) {
        return; // Nothing to blit from
    }

    // If we have a valid multisampling FBO, resolve it to the resolved FBO first.
    if (m_multisamplingFbo && m_multisamplingFbo->isValid()) {
        QOpenGLFramebufferObject::blitFramebuffer(m_resolvedFbo.get(),
                                                  m_multisamplingFbo.get(),
                                                  GL_COLOR_BUFFER_BIT,
                                                  GL_LINEAR);
    }

    // Now blit the (potentially resolved) FBO to the target framebuffer.
    const GLsizei width = m_resolvedFbo->width();
    const GLsizei height = m_resolvedFbo->height();

    // Preserve existing framebuffer bindings so we don't surprise callers that
    // expect their GL_FRAMEBUFFER state to remain unchanged across this call.
    GLint prevReadFbo = 0;
    GLint prevDrawFbo = 0;
    gl.glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFbo);
    gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFbo);

    gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, m_resolvedFbo->handle());
    gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetId);
    gl.glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // Restore previous framebuffer bindings to avoid leaking global GL state.
    gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prevReadFbo));
    gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(prevDrawFbo));
}

} // namespace Legacy
