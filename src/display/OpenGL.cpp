/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com>
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "OpenGL.h"

#include <cassert>
#include <QMatrix4x4>
#include <QtGui/QPainter>

#include "../configuration/configuration.h"
#include "../global/utils.h"
#include "FontFormatFlags.h"

OpenGL::~OpenGL()
{
    // Note: We give metrics a pointer to font, so kill metrics first.
    delete std::exchange(m_glFont.metrics, nullptr);
    delete std::exchange(m_glFont.italicMetrics, nullptr);
    delete std::exchange(m_glFont.font, nullptr);
}

void OpenGL::initFont(QPaintDevice *const paintDevice)
{
    assert(m_paintDevice == nullptr);
    deref(paintDevice);
    m_paintDevice = paintDevice;
    m_glFont.font = new QFont(QFont(), paintDevice);
    m_glFont.font->setStyleHint(QFont::System, QFont::OpenGLCompatible);
    m_glFont.metrics = new QFontMetrics(*m_glFont.font);
    m_glFont.font->setItalic(true);
    m_glFont.italicMetrics = new QFontMetrics(*m_glFont.font);
    m_glFont.font->setItalic(false);
}

// http://stackoverflow.com/questions/28216001/how-to-render-text-with-qopenglwidget/28517897
// They couldn't find a slower way to do this I guess
void OpenGL::renderTextAt(const double x,
                          const double y,
                          const QString &text,
                          const QColor &color,
                          const FontFormatFlags fontFormatFlag,
                          const double rotationAngle)
{
    deref(m_paintDevice);
    deref(m_glFont.font);
    deref(m_glFont.metrics);
    deref(m_glFont.italicMetrics);

    QPainter painter(m_paintDevice);
    painter.translate(x, y);
    painter.rotate(rotationAngle);
    painter.setPen(color);
    if (IS_SET(fontFormatFlag, FontFormatFlags::ITALICS)) {
        m_glFont.font->setItalic(true);
    }
    if (IS_SET(fontFormatFlag, FontFormatFlags::UNDERLINE)) {
        m_glFont.font->setUnderline(true);
    }
    painter.setFont(*m_glFont.font);
    if (getConfig().canvas.antialiasingSamples > 0) {
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    }
    painter.drawText(0, 0, text);
    m_glFont.font->setItalic(false);
    m_glFont.font->setUnderline(false);
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

    m_opengl.glMatrixMode(getType(type));
    m_opengl.glLoadMatrixf(mat);
}
