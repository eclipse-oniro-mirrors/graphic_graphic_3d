#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "common/bloom_common.h"

// sets

#include "render/shaders/common/render_post_process_layout_common.h"

layout(set = 1, binding = 0) uniform texture2D uTex;
layout(set = 1, binding = 1) uniform texture2D uInputColor;
layout(set = 1, binding = 2) uniform sampler uSampler;

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

///////////////////////////////////////////////////////////////////////////////
// bloom upscale

void main()
{
    const vec2 uv = inUv;
    vec3 color = bloomUpscale(uv, uPc.viewportSizeInvSize.zw, uTex, uSampler);
    const vec3 baseColor = textureLod(sampler2D(uInputColor, uSampler), uv, 0).xyz;
    const float scatter = uPc.factor.w;
    color = min((baseColor + color * scatter), CORE_BLOOM_CLAMP_MAX_VALUE);

    outColor = vec4(color, 1.0);
}
