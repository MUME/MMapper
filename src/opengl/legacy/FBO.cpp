// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors

#include "FBO.h"

#include "../../global/logging.h"
#include "../OpenGLConfig.h"

namespace {

NODISCARD QOpenGLFramebufferObjectFormat getDefaultFboFormat()
{
    QOpenGLFramebufferObjectFormat fmt;
    fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    fmt.setSamples(0);
    fmt.setTextureTarget(GL_TEXTURE_2D);
    fmt.setInternalTextureFormat(GL_RGB8);
    return fmt;
}

NODISCARD QOpenGLFramebufferObjectFormat getMultiSampleFormat(const int samples)
{
    assert(samples > 0);
    QOpenGLFramebufferObjectFormat fmt;
    fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    fmt.setSamples(samples);
    fmt.setTextureTarget(GL_TEXTURE_2D_MULTISAMPLE);
    fmt.setInternalTextureFormat(GL_RGB8);
    return fmt;
}

} // namespace

namespace Legacy {

static constexpr bool LOG_FBO_ALLOCATIONS = IS_DEBUG_BUILD;

FBO::~FBO()
{
    assert(m_bindState == BindStateEnum::Unbound);
}

void FBO::configure(const Viewport &physicalViewport, const int requestedSamples)
{
    // Unconditionally release old FBOs to ensure a clean slate.
    if (m_bindState != BindStateEnum::Unbound) {
        MMLOG_WARNING() << "FBO was bound during configure.";
        release();
    }
    m_multisamplingFbo.reset();
    m_defaultFbo.reset();

    const QSize physicalSize(physicalViewport.size.x, physicalViewport.size.y);
    if (physicalSize.isEmpty()) {
        if (LOG_FBO_ALLOCATIONS) {
            MMLOG_INFO() << "FBOs destroyed (size empty)";
        }
        return;
    }

    // Always create the default FBO. This is our target for MSAA resolve
    // and the primary render target if MSAA is disabled.
    m_defaultFbo = std::make_unique<QOpenGLFramebufferObject>(physicalSize, getDefaultFboFormat());
    if (!m_defaultFbo->isValid()) {
        m_defaultFbo.reset();
        throw std::runtime_error("Failed to create resolved FBO");
    }

    // Only create the multisampling FBO if requested.
    // Should this be "> 1"? (Does it make sense to have MSSA with 1 sample?)
    if (const int actualSamples = std::min(requestedSamples, OpenGLConfig::getMaxSamples());
        actualSamples > 0) {
        m_multisamplingFbo = std::make_unique<QOpenGLFramebufferObject>(physicalSize,
                                                                        getMultiSampleFormat(
                                                                            actualSamples));
        if (!m_multisamplingFbo->isValid()) {
            m_multisamplingFbo.reset();
            // always log errors
            MMLOG_ERROR() << "Failed to create multisampling FBO. Falling back to no multisampling.";
        } else if (LOG_FBO_ALLOCATIONS) {
            MMLOG_INFO() << "Created multisampling FBO with " << actualSamples << " samples.";
        }
    }
}

QOpenGLFramebufferObject *FBO::getDrawable()
{
    return m_multisamplingFbo ? m_multisamplingFbo.get() : m_defaultFbo.get();
}

void FBO::bind()
{
    assert(m_bindState == BindStateEnum::Unbound);
    if (const auto fboToBind = getDrawable()) {
        if (!fboToBind->bind()) {
            MMLOG_ERROR() << "Failed to bind FBO.";
        } else {
            m_bindState = (fboToBind == m_defaultFbo.get()) ? BindStateEnum::DefaultIsBound
                                                            : BindStateEnum::MultisampleIsBound;
        }
    }
}

void FBO::release()
{
    assert(m_bindState != BindStateEnum::Unbound);
    if (const auto fboToRelease = getDrawable()) {
        switch (m_bindState) {
        case BindStateEnum::Unbound:
            std::abort();
        case BindStateEnum::DefaultIsBound:
            assert(fboToRelease == m_defaultFbo.get());
            break;
        case BindStateEnum::MultisampleIsBound:
            assert(fboToRelease == m_multisamplingFbo.get());
            break;
        }
        assert(fboToRelease->isBound());
        if (!fboToRelease->release()) {
            MMLOG_ERROR() << "Failed to release FBO.";
        }
    }
    m_bindState = BindStateEnum::Unbound;
}

void FBO::resolve()
{
    assert(m_bindState == BindStateEnum::Unbound);
    if (!m_defaultFbo) {
        return; // Nothing to resolve to
    }

    // If we have a valid multisampling FBO, resolve it to the resolved FBO first.
    if (m_multisamplingFbo && m_multisamplingFbo->isValid()) {
        // NOTE: In WebGL2/GLES 3.0 environments, resolving a multisampled framebuffer
        // requires GL_NEAREST.
        QOpenGLFramebufferObject::blitFramebuffer(m_defaultFbo.get(),
                                                  m_multisamplingFbo.get(),
                                                  GL_COLOR_BUFFER_BIT,
                                                  GL_NEAREST);
    }
}

GLuint FBO::resolvedTextureId() const
{
    assert(m_bindState == BindStateEnum::Unbound);
    return m_defaultFbo ? m_defaultFbo->texture() : 0;
}

} // namespace Legacy
