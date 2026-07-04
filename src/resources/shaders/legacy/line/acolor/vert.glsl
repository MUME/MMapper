uniform mat4 uMVP;
uniform float uLineWidth;
uniform ivec4 uViewport; // x, y, width, height

layout(location = 0) in vec4 aColor;
layout(location = 1) in vec3 aVert1;
layout(location = 2) in vec3 aVert2;

out vec4 vColor;

void main()
{
    vColor = aColor;

    vec4 clip1 = uMVP * vec4(aVert1, 1.0);
    vec4 clip2 = uMVP * vec4(aVert2, 1.0);

    vec2 ndc1 = clip1.xy / clip1.w;
    vec2 ndc2 = clip2.xy / clip2.w;

    vec2 screen1 = (ndc1 + 1.0) * 0.5 * vec2(uViewport.zw);
    vec2 screen2 = (ndc2 + 1.0) * 0.5 * vec2(uViewport.zw);

    vec2 dir = screen2 - screen1;
    float len = length(dir);
    if (len < 0.0001) {
        dir = vec2(1.0, 0.0);
    } else {
        dir /= len;
    }
    vec2 normal = vec2(-dir.y, dir.x);

    vec2 offset = normal * uLineWidth * 0.5;

    vec2 pos;
    float z;
    if (gl_VertexID == 0) { pos = screen1 + offset; z = clip1.z / clip1.w; }
    else if (gl_VertexID == 1) { pos = screen1 - offset; z = clip1.z / clip1.w; }
    else if (gl_VertexID == 2) { pos = screen2 - offset; z = clip2.z / clip2.w; }
    else { pos = screen2 + offset; z = clip2.z / clip2.w; }

    vec2 final_ndc = (pos / vec2(uViewport.zw)) * 2.0 - 1.0;
    gl_Position = vec4(final_ndc, z, 1.0);
}
