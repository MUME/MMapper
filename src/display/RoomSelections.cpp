// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "../expandoracommon/coordinate.h"
#include "../expandoracommon/room.h"
#include "../global/EnumIndexedArray.h"
#include "../mapdata/roomselection.h"
#include "../opengl/OpenGL.h"
#include "../opengl/OpenGLTypes.h"
#include "Characters.h"
#include "MapCanvasData.h"
#include "Textures.h"
#include "mapcanvas.h"

class NODISCARD RoomSelFakeGL final
{
public:
    enum class SelTypeEnum { Near, Distant, MoveBad, MoveGood };
    static constexpr size_t NUM_SEL_TYPES = 4;

private:
    glm::mat4 m_modelView = glm::mat4(1);
    EnumIndexedArray<std::vector<TexVert>, SelTypeEnum, NUM_SEL_TYPES> m_arrays;

public:
    void resetMatrix() { m_modelView = glm::mat4(1); }
    void glRotatef(float degrees, float x, float y, float z)
    {
        auto &m = m_modelView;
        m = glm::rotate(m, glm::radians(degrees), glm::vec3(x, y, z));
    }
    void glTranslatef(int x, int y, int z)
    {
        glTranslatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    }
    void glTranslatef(float x, float y, float z)
    {
        auto &m = m_modelView;
        m = glm::translate(m, glm::vec3(x, y, z));
    }

    void drawColoredQuad(const SelTypeEnum type)
    {
#define DECL(name, a, b) \
    const TexVert name \
    { \
        glm::vec2{(a), (b)}, glm::vec3 { (a), (b), 0 } \
    }
        DECL(A, 0, 0);
        DECL(B, 1, 0);
        DECL(C, 1, 1);
        DECL(D, 0, 1);
        auto &m = m_modelView;
        const auto transform = [&m](const TexVert &in_vert) -> TexVert {
            auto tmp = m * glm::vec4{in_vert.vert, 1.f};
            return TexVert{in_vert.tex, glm::vec3{tmp / tmp.w}};
        };

        m_arrays[type].emplace_back(transform(A));
        m_arrays[type].emplace_back(transform(B));
        m_arrays[type].emplace_back(transform(C));
        m_arrays[type].emplace_back(transform(D));
#undef DECL
    }

    void draw(OpenGL &gl, const MapCanvasTextures &textures)
    {
        static const MMapper::Array<SelTypeEnum, NUM_SEL_TYPES> ALL_SEL_TYPES
            = {SelTypeEnum::Near, SelTypeEnum::Distant, SelTypeEnum::MoveBad, SelTypeEnum::MoveGood};

        const auto rs
            = GLRenderState().withBlend(BlendModeEnum::TRANSPARENCY).withDepthFunction(std::nullopt);

        for (const SelTypeEnum type : ALL_SEL_TYPES) {
            const std::vector<TexVert> &arr = m_arrays[type];
            if (arr.empty())
                continue;

            const SharedMMTexture &texture = [&textures, type]() -> const SharedMMTexture & {
                switch (type) {
                case SelTypeEnum::Near:
                    return textures.room_sel;
                case SelTypeEnum::Distant:
                    return textures.room_sel_distant;
                case SelTypeEnum::MoveBad:
                    return textures.room_sel_move_bad;
                case SelTypeEnum::MoveGood:
                    return textures.room_sel_move_good;
                default:
                    break;
                }
                std::abort();
            }();

            gl.renderTexturedQuads(arr, rs.withTexture0(texture));
        }
    }
};

void MapCanvas::paintSelectedRoom(RoomSelFakeGL &gl, const Room &room)
{
    const Coordinate &roomPos = room.getPosition();
    const int x = roomPos.x;
    const int y = roomPos.y;
    const int z = roomPos.z;

    // This fake GL uses resetMatrix() before this function.
    gl.resetMatrix();

    const float marginPixels = MapScreen::DEFAULT_MARGIN_PIXELS;
    const bool isMoving = hasRoomSelectionMove();

    if (!isMoving && !m_mapScreen.isRoomVisible(roomPos, marginPixels / 2.f)) {
        const glm::vec3 roomCenter = roomPos.to_vec3() + glm::vec3{0.5f, 0.5f, 0.f};
        const auto dot = DistantObjectTransform::construct(roomCenter, m_mapScreen, marginPixels);
        gl.glTranslatef(dot.offset.x, dot.offset.y, dot.offset.z);
        gl.glRotatef(dot.rotationDegrees, 0.f, 0.f, 1.f);
        const glm::vec2 iconCenter{0.5f, 0.5f};
        gl.glTranslatef(-iconCenter.x, -iconCenter.y, 0.f);
        gl.drawColoredQuad(RoomSelFakeGL::SelTypeEnum::Distant);
    } else {
        // Room is close
        gl.glTranslatef(x, y, z);
        gl.drawColoredQuad(RoomSelFakeGL::SelTypeEnum::Near);
        gl.resetMatrix();
    }

    if (isMoving) {
        gl.resetMatrix();
        const auto &relativeOffset = m_roomSelectionMove->pos;
        gl.glTranslatef(x + relativeOffset.x, y + relativeOffset.y, z);
        gl.drawColoredQuad(m_roomSelectionMove->wrongPlace ? RoomSelFakeGL::SelTypeEnum::MoveBad
                                                           : RoomSelFakeGL::SelTypeEnum::MoveGood);
    }
}

void MapCanvas::paintSelectedRooms()
{
    if (!m_roomSelection || m_roomSelection->empty())
        return;

    RoomSelFakeGL gl;

    for (const auto &[rid, room] : *m_roomSelection) {
        if (room != nullptr) {
            gl.resetMatrix();
            paintSelectedRoom(gl, *room);
        }
    }

    gl.draw(getOpenGL(), m_textures);
}
