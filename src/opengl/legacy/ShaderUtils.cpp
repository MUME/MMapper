// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "ShaderUtils.h"

#include "../../global/ConfigConsts.h"
#include "../../global/Consts.h"
#include "../../global/PrintUtils.h"
#include "../../global/TextUtils.h"

#include <array>
#include <cassert>
#include <cstdio>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string_view>
#include <vector>

#include <QDebug>
#include <QMessageBox>

namespace ShaderUtils {

static const bool VERBOSE_SHADER_DEBUG = []() -> bool {
    if (auto opt = utils::getEnvBool("MMAPPER_VERBOSE_SHADER_DEBUG")) {
        return opt.value();
    }
    return false;
}();
static constexpr const auto npos = std::string_view::npos;

template<typename Callback>
static void foreach_line(const Source &source, Callback &&callback)
{
    std::string_view sv{source.source};
    while (!sv.empty()) {
        const auto it = sv.find(char_consts::C_NEWLINE);
        if (it == npos) {
            callback(sv, false);
            break;
        }
        callback(sv.substr(0, it), true);
        sv = sv.substr(it + 1);
    }
}

static void print(std::ostream &os, const Source &source)
{
    const int width = [&source]() -> int {
        int line = 0;
        bool was_newline = true;
        foreach_line(source, [&line, &was_newline](auto /*sv*/, bool has_newline) {
            if (was_newline) {
                ++line;
            }
            was_newline = has_newline;
        });
        return static_cast<int>(std::to_string(line).size());
    }();

    int line = 0;
    bool was_newline = true;
    foreach_line(source, [&os, width, &line, &was_newline](std::string_view sv, bool has_newline) {
        if (was_newline) {
            ++line;
            os << " " << std::setw(width) << line << ": ";
        }
        os << sv;
        if (has_newline) {
            os << std::endl;
        }
        was_newline = has_newline;
    });

    if (!was_newline) {
        os << std::endl << "WARNING: Missing newline at end of file." << std::endl;
    }
}

NODISCARD static const char *shaderTypeName(const GLenum type)
{
#define CASE(x) \
    do { \
    case GL_##x##_SHADER: \
        return #x; \
    } while (0)
    switch (type) {
        CASE(VERTEX);
        CASE(FRAGMENT);
    default:
        break;
#undef CASE
    }
    return "*ERROR*";
}

#define LOG_AND_POPUP(msg) logAndPopup(__FILE__, __LINE__, __FUNCTION__, (msg))

static void logAndPopup(const char *const file,
                        const int line,
                        const char *const func,
                        const std::string &str)
{
    // NOTE: braces ensure side effects take place before the QMessageBox is displayed.
    {
        // NOTE: this is like calling qWarning();
        auto &&warn = QMessageLogger(file, line, func).warning();
        warn.noquote();
        warn.nospace();
        warn << str.c_str();
    }

    QMessageBox box;
    box.setWindowTitle("Message from OpenGL");
    box.setText(str.c_str());
    box.exec();
}

static void checkProgramInfo(Functions &gl, const GLuint programID)
{
    GLint result = GL_FALSE;
    gl.glGetProgramiv(programID, GL_LINK_STATUS, &result);
    if (result != GL_TRUE) {
        qWarning() << "ERROR: Failed to link program";
    }

    {
        int infoLogLength = 0;
        gl.glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            std::vector<char> programErrorMessage(static_cast<size_t>(infoLogLength + 1));
            gl.glGetProgramInfoLog(programID, infoLogLength, nullptr, &programErrorMessage[0]);

            // Some drivers return strings with null characters or multiple newlines
            QByteArray ba(&programErrorMessage[0]);
            ba = ba.trimmed();
            if (!ba.isEmpty()) {
                std::ostringstream os;
                os << "Program info log:" << std::endl;
                os << &programErrorMessage[0] << std::endl;
                LOG_AND_POPUP(os.str());
            }
        }
    }

    if (result != GL_TRUE) {
        std::abort();
    }
}

static void checkShaderInfo(Functions &gl, const GLuint shaderId)
{
    // REVISIT: Technically you can retrieve the source with glGetShaderiv(),
    // so we could choose to only print the source code if there's a problem.

    GLint result = GL_FALSE;
    gl.glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
    if (result != GL_TRUE) {
        qWarning() << "ERROR: Failed to compile shader.";
    }

    {
        GLint infoLogLength = 0;
        gl.glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            std::vector<char> shaderErrorMessage(static_cast<size_t>(infoLogLength + 1));
            gl.glGetShaderInfoLog(shaderId, infoLogLength, nullptr, &shaderErrorMessage[0]);

            // Some drivers return strings with null characters or multiple newlines
            QByteArray ba(&shaderErrorMessage[0]);
            ba = ba.trimmed();
            if (!ba.isEmpty()) {
                std::ostringstream os;
                os << "Shader info log:" << std::endl;
                os << &shaderErrorMessage[0] << std::endl;
                LOG_AND_POPUP(os.str());
            }
        }
    }

    if (result != GL_TRUE) {
        std::abort();
    }
}

NODISCARD static GLuint compileShader(Functions &gl, const GLenum type, const Source &source)
{
    if (!source) {
        return 0;
    }

    const GLuint shaderId = gl.glCreateShader(type);
    if constexpr ((IS_DEBUG_BUILD)) {
        std::ostringstream os;
        os << "Compiling " << shaderTypeName(type) << " shader " << QuotedString(source.filename)
           << "...";

        if (VERBOSE_SHADER_DEBUG) {
            os << std::endl;
            print(os, source);
        }

        qDebug() << os.str().c_str();
    }

    // NOTE: GLES 2.0 required `const char**` instead of `const char*const*`,
    // so Qt uses the least common denominator without the middle const;
    // that's the reason the `ptrs` array below is not `const`.
    std::array<const char *, 3> ptrs = {Functions::getShaderVersion(),
                                        "#line 1\n",
                                        source.source.c_str()};
    gl.glShaderSource(shaderId, static_cast<GLsizei>(ptrs.size()), ptrs.data(), nullptr);
    gl.glCompileShader(shaderId);
    checkShaderInfo(gl, shaderId);

    return shaderId;
}

Program loadShaders(Functions &gl, const Source &vert, const Source &frag)
{
    std::vector<GLuint> shaders{compileShader(gl, GL_VERTEX_SHADER, vert),
                                compileShader(gl, GL_FRAGMENT_SHADER, frag)};

    const auto is_valid = [](GLuint s) -> bool { return s != 0; };
    const auto numShaders = std::count_if(shaders.begin(), shaders.end(), is_valid);
    if (numShaders == 0) {
        std::abort(); // there won't be anything to link
    }

    if constexpr ((IS_DEBUG_BUILD)) {
        std::ostringstream os;
        os << "Linking " << numShaders << " shader " << ((numShaders == 1) ? "stage" : "stages");
        qDebug() << os.str().c_str();
    }

    Program result_prog;
    result_prog.emplace(gl.shared_from_this());
    const GLuint prog = result_prog.get();

    for (const GLuint s : shaders) {
        if (is_valid(s)) {
            gl.glAttachShader(prog, s);
        }
    }

    gl.glLinkProgram(prog);
    checkProgramInfo(gl, prog);

    for (const GLuint s : shaders) {
        if (is_valid(s)) {
            gl.glDetachShader(prog, s);
            gl.glDeleteShader(s);
        }
    }

    return result_prog;
}

} // namespace ShaderUtils
