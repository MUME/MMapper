// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "SimpleMesh.h"

#include "../../global/ConfigConsts.h"

void Legacy::drawRoomQuad(Functions &gl, const GLsizei numInstances)
{
    // The shader uses gl_VertexID to generate quad vertices [0..3]
    gl.glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, numInstances);
}
