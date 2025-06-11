#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/Array.h"
#include "../global/Color.h"
#include "../global/IndexedVector.h"
#include "../global/TaggedInt.h"
#include "../global/hash.h"
#include "../global/utils.h"
#include "FontFormatFlags.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include <QDebug>
#include <qopengl.h>

struct NODISCARD TexVert final
{
    glm::vec2 tex{};
    glm::vec3 vert{};

    explicit TexVert(const glm::vec2 &tex_, const glm::vec3 &vert_)
        : tex{tex_}
        , vert{vert_}
    {}
};

using TexVertVector = std::vector<TexVert>;

struct NODISCARD ColoredTexVert final
{
    Color color;
    glm::vec2 tex{};
    glm::vec3 vert{};

    explicit ColoredTexVert(const Color &color_, const glm::vec2 &tex_, const glm::vec3 &vert_)
        : color{color_}
        , tex{tex_}
        , vert{vert_}
    {}
};

struct NODISCARD ColorVert final
{
    Color color;
    glm::vec3 vert{};

    explicit ColorVert(const Color &color_, const glm::vec3 &vert_)
        : color{color_}
        , vert{vert_}
    {}
};

// Similar to ColoredTexVert, except it has a base position in world coordinates.
// the font's vertex shader transforms the world position to screen space,
// rounds to integer pixel offset, and then adds the vertex position in screen space.
//
// Rendering with the font shader requires passing uniforms for the world space
// model-view-projection matrix and the output viewport.
struct NODISCARD FontVert3d final
{
    glm::vec3 base{}; // world space
    Color color;
    glm::vec2 tex{};
    glm::vec2 vert{}; // screen space

    explicit FontVert3d(const glm::vec3 &base_,
                        const Color &color_,
                        const glm::vec2 &tex_,
                        const glm::vec2 &vert_)
        : base{base_}
        , color{color_}
        , tex{tex_}
        , vert{vert_}
    {}
};

enum class NODISCARD DrawModeEnum { INVALID = 0, POINTS = 1, LINES = 2, TRIANGLES = 3, QUADS = 4 };

struct NODISCARD LineParams final
{
    float width = 1.f;
    LineParams() = default;

    explicit LineParams(const float width_)
        : width{width_}
    {}
};

#define XFOREACH_DEPTHFUNC(X) \
    X(NEVER) \
    X(LESS) \
    X(EQUAL) \
    X(LEQUAL) \
    X(GREATER) \
    X(NOTEQUAL) \
    X(GEQUAL) \
    X(ALWAYS)

#define X_DECL(X) X = GL_##X,
enum class NODISCARD DepthFunctionEnum { XFOREACH_DEPTHFUNC(X_DECL) DEFAULT = LESS };
#undef X_DECL

enum class NODISCARD BlendModeEnum {
    /* glDisable(GL_BLEND); */
    NONE,
    /* This is the MMapper2 default setting, but not OpenGL default setting
     * glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); */
    TRANSPARENCY,
    /* This mode allows you to multiply by the painted color, in the range [0,1].
     * glEnable(GL_BLEND); glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ZERO, GL_ONE); */
    MODULATE,
};

enum class NODISCARD CullingEnum {

    // Culling is disabled: glDisable(GL_CULL_FACE)
    DISABLED,

    // GL_BACK: back faces are culled (the usual default if GL_CULL_FACE is enabled)
    BACK,

    // GL_FRONT: front faces are culled
    FRONT,

    // GL_FRONT_AND_BACK: both front and back faces are culled
    // (you probably don't ever want this)
    FRONT_AND_BACK

};

namespace tags {
struct NODISCARD MMTextureIdTag final
{};
}; // namespace tags

struct NODISCARD MMTextureId final : TaggedInt<MMTextureId, tags::MMTextureIdTag, int32_t, -1>
{
    using TaggedInt::TaggedInt;
    NODISCARD explicit operator bool() { return value() > DEFAULT_VALUE; }
    NODISCARD explicit operator size_t() const
    {
        if (value() < 0) {
            throw std::runtime_error("negative index");
        }
        return static_cast<size_t>(value());
    }
};
static constexpr const MMTextureId INVALID_MM_TEXTURE_ID{MMTextureId::DEFAULT_VALUE};

template<>
struct std::hash<MMTextureId>
{
    std::size_t operator()(const MMTextureId id) const noexcept { return numeric_hash(id.value()); }
};

class MMTexture;
using SharedMMTexture = std::shared_ptr<MMTexture>;
using TexLookup = IndexedVector<SharedMMTexture, MMTextureId>;
using SharedTexLookup = std::shared_ptr<TexLookup>;

struct NODISCARD GLRenderState final
{
    // glEnable(GL_BLEND)
    BlendModeEnum blend = BlendModeEnum::NONE;

    CullingEnum culling = CullingEnum::DISABLED;

    // glEnable(GL_DEPTH_TEST) + glDepthFunc()
    using OptDepth = std::optional<DepthFunctionEnum>;
    OptDepth depth;

    // glLineWidth() + { glEnable(LINE_STIPPLE) + glLineStipple() }
    LineParams lineParams;

    using Textures = MMapper::Array<MMTextureId, 2>;
    struct NODISCARD Uniforms final
    {
        Color color;
        // glEnable(TEXTURE_2D), or glEnable(TEXTURE_3D)
        Textures textures;
        std::optional<float> pointSize;
    };

    Uniforms uniforms;

    NODISCARD GLRenderState withBlend(const BlendModeEnum new_blend) const
    {
        GLRenderState copy = *this;
        copy.blend = new_blend;
        return copy;
    }

    NODISCARD GLRenderState withColor(const Color &new_color) const
    {
        GLRenderState copy = *this;
        copy.uniforms.color = new_color;
        return copy;
    }

    NODISCARD GLRenderState withCulling(const CullingEnum new_culling) const
    {
        GLRenderState copy = *this;
        copy.culling = new_culling;
        return copy;
    }

    NODISCARD GLRenderState withDepthFunction(const DepthFunctionEnum new_depth) const
    {
        GLRenderState copy = *this;
        copy.depth = new_depth;
        return copy;
    }

    NODISCARD GLRenderState withDepthFunction(std::nullopt_t) const
    {
        GLRenderState copy = *this;
        copy.depth.reset();
        return copy;
    }

    NODISCARD GLRenderState withLineParams(const LineParams &new_lineParams) const
    {
        GLRenderState copy = *this;
        copy.lineParams = new_lineParams;
        return copy;
    }

    NODISCARD GLRenderState withPointSize(const GLfloat size) const
    {
        GLRenderState copy = *this;
        copy.uniforms.pointSize = size;
        return copy;
    }

    NODISCARD GLRenderState withTexture0(const MMTextureId new_texture) const
    {
        GLRenderState copy = *this;
        copy.uniforms.textures = Textures{new_texture, INVALID_MM_TEXTURE_ID};
        return copy;
    }
};

struct NODISCARD IRenderable
{
public:
    IRenderable() = default;
    virtual ~IRenderable();

public:
    DELETE_CTORS_AND_ASSIGN_OPS(IRenderable);

private:
    // Clears the contents of the mesh, but does not give up its GL resources.
    virtual void virt_clear() = 0;
    // Clears the mesh and destroys the GL resources.
    virtual void virt_reset() = 0;
    NODISCARD virtual bool virt_isEmpty() const = 0;

private:
    NODISCARD virtual bool virt_modifiesRenderState() const { return false; }
    NODISCARD virtual GLRenderState virt_modifyRenderState(const GLRenderState &input) const
    {
        assert(false);
        return input;
    }
    virtual void virt_render(const GLRenderState &renderState) = 0;

public:
    // Clears the contents of the mesh, but does not give up its GL resources.
    void clear() { virt_clear(); }
    // Clears the mesh and destroys the GL resources.
    void reset() { virt_reset(); }
    NODISCARD bool isEmpty() const { return virt_isEmpty(); }

public:
    void render(const GLRenderState &renderState)
    {
        if (!virt_modifiesRenderState()) {
            virt_render(renderState);
            return;
        }
        const GLRenderState modifiedState = virt_modifyRenderState(renderState);
        virt_render(modifiedState);
    }
};

struct NODISCARD TexturedRenderable final : public IRenderable
{
private:
    MMTextureId m_texture;
    std::unique_ptr<IRenderable> m_mesh;

public:
    explicit TexturedRenderable(MMTextureId tex, std::unique_ptr<IRenderable> mesh);
    ~TexturedRenderable() final;

public:
    DELETE_CTORS_AND_ASSIGN_OPS(TexturedRenderable);

private:
    void virt_clear() final;
    void virt_reset() final;
    NODISCARD bool virt_isEmpty() const final;

public:
    void virt_render(const GLRenderState &renderState) final;

public:
    NODISCARD MMTextureId replaceTexture(const MMTextureId tex)
    {
        return std::exchange(m_texture, tex);
    }
};

enum class NODISCARD BufferUsageEnum { STATIC_DRAW, DYNAMIC_DRAW };

class NODISCARD UniqueMesh final
{
private:
    std::unique_ptr<IRenderable> m_mesh;

public:
    UniqueMesh() = default;
    explicit UniqueMesh(std::unique_ptr<IRenderable> mesh)
        : m_mesh{std::move(mesh)}
    {
        std::ignore = deref(m_mesh);
    }
    ~UniqueMesh() = default;
    DEFAULT_MOVES_DELETE_COPIES(UniqueMesh);

    void render(const GLRenderState &rs) const { deref(m_mesh).render(rs); }
};

struct NODISCARD UniqueMeshVector final
{
private:
    std::vector<UniqueMesh> m_meshes;

public:
    UniqueMeshVector() = default;
    explicit UniqueMeshVector(std::vector<UniqueMesh> &&meshes)
        : m_meshes{std::move(meshes)}
    {}

    void render(const GLRenderState &rs)
    {
        for (auto &mesh : m_meshes) {
            mesh.render(rs);
        }
    }
};

struct NODISCARD Viewport final
{
    glm::ivec2 offset{};
    glm::ivec2 size{};
};

static constexpr const size_t VERTS_PER_LINE = 2;
static constexpr const size_t VERTS_PER_TRI = 3;
static constexpr const size_t VERTS_PER_QUAD = 4;
