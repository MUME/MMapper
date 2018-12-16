#pragma once
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

#ifndef MMAPPER_OPENGL_H
#define MMAPPER_OPENGL_H

#include "FontFormatFlags.h"
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <QColor>
#include <QMatrix4x4>
#include <QMessageLogContext>
#include <QOpenGLFunctions_1_0>
#include <QtCore>
#include <QtGlobal>
#include <QtGui/QFontMetrics>
#include <QtGui/QOpenGLFunctions_1_0>
#include <QtGui/qopengl.h>

#include "../global/utils.h"

struct Vec2f
{
    float x = 0.0f, y = 0.0f;
    explicit Vec2f() = default;
    explicit Vec2f(float x, float y)
        : x{x}
        , y{y}
    {}
};

struct Vec3f
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    explicit Vec3f() = default;
    explicit Vec3f(float x, float y, float z)
        : x{x}
        , y{y}
        , z{z}
    {}
};

class OpenGL;

class TexVert
{
private:
    friend class OpenGL;
    Vec2f tex;
    Vec3f vert;

public:
    explicit TexVert(Vec2f tex, Vec3f vert)
        : tex{tex}
        , vert{vert}
    {}
};

enum class DrawType { LINES, LINE_LOOP, LINE_STRIP, POINTS, POLYGON, TRIANGLES, TRIANGLE_STRIP };

class XColor4f
{
private:
    friend class OpenGL;

private:
    GLfloat r, g, b, a;

public:
    explicit XColor4f(const GLfloat r, const GLfloat g, const GLfloat b, const GLfloat a)
        : r{r}
        , g{g}
        , b{b}
        , a{a}
    {
        check();
    }
    explicit XColor4f(const QColor color)
        : r{static_cast<float>(color.redF())}
        , g{static_cast<float>(color.greenF())}
        , b{static_cast<float>(color.blueF())}
        , a{static_cast<float>(color.alphaF())}
    {
        check();
    }

    /**
     * @param color
     * @param a Overrides QColor's alpha
     */
    explicit XColor4f(const QColor color, const float a)
        : r{static_cast<float>(color.redF())}
        , g{static_cast<float>(color.greenF())}
        , b{static_cast<float>(color.blueF())}
        , a{a}
    {
        check();
    }

public:
    GLfloat getR() const { return r; }
    GLfloat getG() const { return g; }
    GLfloat getB() const { return b; }
    GLfloat getA() const { return a; }

    void check() const
    {
#define CHECK(x) \
    if (x < 0.0f || x > 1.0f) \
    qWarning() << "XColor4f" << #x " = " << x
        CHECK(r);
        CHECK(g);
        CHECK(b);
        CHECK(a);
#undef CHECK
    }
};

class XDeviceLineWidth
{
private:
    friend class OpenGL;

private:
    GLfloat width;

public:
    explicit XDeviceLineWidth(const GLfloat width)
        : width{width}
    {}
};

class XDevicePointSize
{
private:
    friend class OpenGL;

private:
    GLfloat size;

public:
    explicit XDevicePointSize(const GLfloat size)
        : size{size}
    {}
};

enum class XOption {
    BLEND,
    DEPTH_TEST,
    LINE_STIPPLE,
    MULTISAMPLE,
    NORMALIZE,
    POLYGON_STIPPLE,
    TEXTURE_2D
};

class XEnable
{
private:
    friend class OpenGL;

private:
    XOption option;

public:
    explicit XEnable(XOption option)
        : option{option}
    {}
};

class XDisable
{
private:
    friend class OpenGL;

private:
    XOption option;

public:
    explicit XDisable(XOption option)
        : option{option}
    {}
};

class XDraw
{
private:
    friend class OpenGL;

private:
    DrawType type;
    std::vector<Vec3f> args;

public:
    XDraw(const DrawType type, std::vector<Vec3f> args)
        : type{type}
        , args{std::move(args)}
    {}
};

class XDrawTextured
{
private:
    friend class OpenGL;

private:
    DrawType type;
    std::vector<TexVert> args;

public:
    explicit XDrawTextured(const DrawType type, std::vector<TexVert> args)
        : type{type}
        , args{std::move(args)}
    {}
};

class XDisplayList final
{
private:
    friend class OpenGL;
    GLuint list = 0u;
    OpenGL *opengl = nullptr;

private:
    explicit XDisplayList(OpenGL *const opengl, const GLuint list)
        : list{list}
        , opengl{opengl}
    {}

public:
    explicit XDisplayList() = default;

public:
    bool isValid() const { return list != 0u; }
    void destroy();
};

enum class LineStippleType { TWO, FOUR };
enum class MatrixType { MODELVIEW, PROJECTION };

struct FontData final
{
public:
    QFont *font = nullptr;
    QFontMetrics *metrics = nullptr;
    QFontMetrics *italicMetrics = nullptr;

public:
    ~FontData();

public:
    void init(QPaintDevice *paintDevice);
    void cleanup();
};

class OpenGL final
{
private:
    QOpenGLFunctions_1_0 m_opengl;
    QPaintDevice *m_paintDevice = nullptr;
    FontData m_glFont;

private:
    float devicePixelRatio_ = 1.0f;
    float devicePixelRatio() { return devicePixelRatio_; }

public:
    ~OpenGL();

public:
    void setDevicePixelRatio(const float ratio) { devicePixelRatio_ = ratio; }

public:
#define GL_PROXY(f) \
    template<typename... Args> \
    auto f(Args... args) \
    { \
        return m_opengl.f(args...); \
    }

public:
    // init
    GL_PROXY(initializeOpenGLFunctions)
    GL_PROXY(glGetString)      // version, renderer, vendor
    GL_PROXY(glPolygonStipple) // halftone
    GL_PROXY(glShadeModel)     // FLAT
    GL_PROXY(glBlendFunc)      // only ever called with (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

public:
    // per frame
    GL_PROXY(glClear)
    GL_PROXY(glClearColor)
    GL_PROXY(glViewport)

public:
    // matrix
    GL_PROXY(glPopMatrix)
    GL_PROXY(glPushMatrix)
    GL_PROXY(glRotatef)
    GL_PROXY(glTranslatef)

public:
#undef GL_PROXY

public:
    void setMatrix(MatrixType type, const QMatrix4x4 &m);

public:
    void callList(XDisplayList list) { m_opengl.glCallList(list.list); }

private:
    static GLuint getGLType(const DrawType type)
    {
#define CASE(x) \
    case DrawType::x: \
        return GL_##x
        switch (type) {
            CASE(LINE_LOOP);
            CASE(LINE_STRIP);
            CASE(LINES);
            CASE(POINTS);
            CASE(POLYGON);
            CASE(TRIANGLES);
            CASE(TRIANGLE_STRIP);
        }
        throw std::invalid_argument("type");
#undef CASE
    }

    static GLenum getOpt(const XOption option)
    {
#define CASE(x) \
    case XOption::x: \
        return GL_##x
        switch (option) {
            CASE(BLEND);
            CASE(DEPTH_TEST);
            CASE(LINE_STIPPLE);
            CASE(TEXTURE_2D);

            // init
            CASE(MULTISAMPLE);
            CASE(NORMALIZE);
            CASE(POLYGON_STIPPLE);
        }
        throw std::invalid_argument("option");
#undef CASE
    }

public:
    void draw(DrawType type, const std::vector<Vec3f> &args)
    {
        m_opengl.glBegin(getGLType(type));
        for (auto v : args) {
            m_opengl.glVertex3f(v.x, v.y, v.z);
        }
        m_opengl.glEnd();
    }

private:
    void draw(DrawType type, const std::vector<TexVert> &args)
    {
        m_opengl.glBegin(getGLType(type));
        for (auto v : args) {
            m_opengl.glTexCoord2f(v.tex.x, v.tex.y);
            m_opengl.glVertex3f(v.vert.x, v.vert.y, v.vert.z);
        }
        m_opengl.glEnd();
    }

public:
    void draw(const XDraw &commands) { draw(commands.type, commands.args); }
    void draw(const XDrawTextured &commands) { draw(commands.type, commands.args); }

private:
    void apply() {}

public:
    template<typename... Args>
    void apply(const XDraw &commands, const Args &... args)
    {
        draw(commands);
        apply(args...);
    }

    template<typename... Args>
    void apply(const XDrawTextured &commands, const Args &... args)
    {
        draw(commands);
        apply(args...);
    }

    template<typename... Args>
    void apply(const XColor4f &color, const Args &... args)
    {
        color.check();
        m_opengl.glColor4f(color.getR(), color.getG(), color.getB(), color.getA());
        apply(args...);
    }

    template<typename... Args>
    void apply(const XDeviceLineWidth &width, const Args &... args)
    {
        m_opengl.glLineWidth(devicePixelRatio() * static_cast<float>(width.width));
        apply(args...);
    }
    template<typename... Args>
    void apply(const XDevicePointSize &size, const Args &... args)
    {
        m_opengl.glPointSize(devicePixelRatio() * static_cast<float>(size.size));
        apply(args...);
    }

    template<typename... Args>
    void apply(const XEnable &enable, const Args &... args)
    {
        m_opengl.glEnable(getOpt(enable.option));
        if (enable.option == XOption::BLEND)
            m_opengl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        apply(args...);
    }

    template<typename... Args>
    void apply(const XDisable &disable, const Args &... args)
    {
        m_opengl.glDisable(getOpt(disable.option));
        apply(args...);
    }

    template<typename... Args>
    void apply(const LineStippleType stipple, const Args &... args)
    {
        const auto getFactor = [](const LineStippleType stipple) -> GLint {
            switch (stipple) {
            case LineStippleType::TWO:
                return 2;
            case LineStippleType::FOUR:
                return 4;
            }
            return 1.0;
        };
        m_opengl.glLineStipple(getFactor(stipple), static_cast<GLushort>(0xAAAAu));
        apply(args...);
    }

    template<typename... Args>
    void apply(const XDisplayList &displayList, const Args &... args)
    {
        this->callList(displayList);
        apply(args...);
    }

public:
    template<typename... Args>
    XDisplayList compile(const Args &... args)
    {
        const auto list = m_opengl.glGenLists(1);
        m_opengl.glNewList(list, GL_COMPILE);
        apply(args...);
        m_opengl.glEndList();
        return XDisplayList{this, list};
    }

    void destroyList(GLuint list)
    {
        if (list != 0u)
            m_opengl.glDeleteLists(list, 1);
    }

public:
    /* REVISIT: font stuff technically isn't part of OpenGL,
     * so this isn't the best place to put font functions. */
    void initFont(QPaintDevice *paintDevice);

    template<typename T>
    int getFontWidth(T x, FontFormatFlags flags) const
    {
        switch (flags) {
        case FontFormatFlags::ITALICS:
            return deref(m_glFont.italicMetrics).width(x);

        case FontFormatFlags::NONE:
        case FontFormatFlags::UNDERLINE:
        default:
            return deref(m_glFont.metrics).width(x);
        }
    }

    int getFontHeight() const { return deref(m_glFont.metrics).height(); }

    void renderTextAt(const float x,
                      const float y,
                      const QString &text,
                      const QColor &color,
                      const FontFormatFlags fontFormatFlag,
                      const float rotationAngle);
};

#endif // MMAPPER_OPENGL_H
