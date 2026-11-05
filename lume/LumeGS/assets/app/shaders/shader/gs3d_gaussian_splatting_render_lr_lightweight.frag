#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "common/gs_common.h"

#include "render/shaders/common/render_compatibility_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"

layout(location = 0) in flat uint vColor;
layout(location = 1) in vec2 vPosition;
layout(location = 2) in vec2 vSquarePosition;
layout(location = 3) in vec2 vSrParams;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outAttrs;

void main()
{
    if(dot(vSquarePosition, vSquarePosition) > 1.0f){
        discard;
    }

    const float A = -0.5f * dot(vPosition, vPosition);
    const vec4 color = unpackUnorm4x8(vColor).abgr;

    const float B = exp2(A * 1.44269504f) * color.a;

    if (B < 1.0f / 255.0f) {
        discard;
    }

    const float alpha = B;
    outColor = vec4(alpha * color.rgb, alpha);
    outAttrs = vec4(alpha * vSrParams, alpha);
}

