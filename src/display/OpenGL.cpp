// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "OpenGL.h"

#include <cassert>
#include <QMatrix4x4>
#include <QtGui/QPainter>

#include "../configuration/configuration.h"
#include "../global/utils.h"
#include "FontFormatFlags.h"

#ifdef WIN32
extern "C" {
// Prefer discrete nVidia and AMD GPUs by default on Windows
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

void XDisplayList::destroy()
{
    if (isValid()) {
        if (auto gl = std::exchange(opengl, nullptr)) {
            if ((false))
                qInfo() << "Destroying list:" << list;
            gl->destroyList(std::exchange(list, 0u));
        }
    }
}

FontData::~FontData()
{
    cleanup();
}

void FontData::cleanup()
{
    // Note: We give metrics a pointer to font, so kill metrics first.
    delete std::exchange(italicMetrics, nullptr);
    delete std::exchange(metrics, nullptr);
    delete std::exchange(font, nullptr);
}

void FontData::init(QPaintDevice *const paintDevice)
{
    font = new QFont(QFont(), paintDevice);
    font->setStyleHint(QFont::System, QFont::OpenGLCompatible);

    metrics = new QFontMetrics(*font);
    font->setItalic(true);

    italicMetrics = new QFontMetrics(*font);
    font->setItalic(false);
}

OpenGL::~OpenGL() = default;

void OpenGL::initFont(QPaintDevice *const paintDevice)
{
    assert(m_paintDevice == nullptr);
    deref(paintDevice);
    m_paintDevice = paintDevice;
    m_glFont.init(paintDevice);
}

int OpenGL::getFontWidth(const QString &x, const FontFormatFlags &flags) const
{
    if (flags.contains(FontFormatFlagEnum::ITALICS))
        return deref(m_glFont.italicMetrics).width(x);
    else
        return deref(m_glFont.metrics).width(x);
}

int OpenGL::getFontHeight() const
{
    return deref(m_glFont.metrics).height();
}

// http://stackoverflow.com/questions/28216001/how-to-render-text-with-qopenglwidget/28517897
// They couldn't find a slower way to do this I guess
void OpenGL::renderTextAt(const float x,
                          const float y,
                          const QString &text,
                          const QColor &color,
                          const FontFormatFlags &fontFormatFlags,
                          const float rotationAngle)
{
    deref(m_paintDevice);
    auto &font = deref(m_glFont.font);
    deref(m_glFont.metrics);
    deref(m_glFont.italicMetrics);

    QPainter painter(m_paintDevice);
    painter.translate(QPointF(static_cast<qreal>(x), static_cast<qreal>(y)));
    painter.rotate(static_cast<qreal>(rotationAngle));
    painter.setPen(color);
    if (fontFormatFlags.contains(FontFormatFlagEnum::ITALICS)) {
        font.setItalic(true);
    }
    if (fontFormatFlags.contains(FontFormatFlagEnum::UNDERLINE)) {
        font.setUnderline(true);
    }
    painter.setFont(font);
    if (getConfig().canvas.antialiasingSamples > 0) {
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    }
    painter.drawText(0, 0, text);
    font.setItalic(false);
    font.setUnderline(false);
    painter.end();
}

void OpenGL::setMatrix(const MatrixType type, const QMatrix4x4 &m)
{
    const auto getType = [](const MatrixType type) -> GLenum {
        switch (type) {
        case MatrixType::MODELVIEW:
            return static_cast<GLenum>(GL_MODELVIEW);
        case MatrixType::PROJECTION:
            return static_cast<GLenum>(GL_PROJECTION);
        }
        throw std::invalid_argument("type");
    };

    // static to prevent glLoadMatrixf to fail on certain drivers
    static GLfloat mat[16];
    const float *data = m.constData();
    for (int index = 0; index < 16; ++index) {
        mat[index] = data[index];
    }

    auto &gl = getLegacy();
    gl.glMatrixMode(getType(type));
    gl.glLoadMatrixf(mat);
}
