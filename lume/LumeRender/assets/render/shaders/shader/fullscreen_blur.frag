#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_blur_common.h"

// sets

#include "render/shaders/common/render_post_process_layout_common.h"

layout(set = 1, binding = 0) uniform sampler uSampler;
layout(set = 1, binding = 1) uniform texture2D uTex;

layout(constant_id = 0) const uint CORE_BLUR_COLOR_TYPE = 0;

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

void main(void)
{
    if (CORE_BLUR_COLOR_TYPE == CORE_BLUR_TYPE_R) {
        outColor.r =
            GaussianBlurR(uTex, uSampler, gl_FragCoord.xy, inUv.xy, uPc.factor.xy, uPc.viewportSizeInvSize.zw).r;
    } else if (CORE_BLUR_COLOR_TYPE == CORE_BLUR_TYPE_RG) {
        outColor.rg =
            GaussianBlurRG(uTex, uSampler, gl_FragCoord.xy, inUv.xy, uPc.factor.xy, uPc.viewportSizeInvSize.zw).rg;
    } else if (CORE_BLUR_COLOR_TYPE == CORE_BLUR_TYPE_RGB) {
        outColor.rgb =
            GaussianBlurRGB(uTex, uSampler, gl_FragCoord.xy, inUv.xy, uPc.factor.xy, uPc.viewportSizeInvSize.zw).rgb;
    } else if (CORE_BLUR_COLOR_TYPE == CORE_BLUR_TYPE_A) {
        outColor.r =
            GaussianBlurA(uTex, uSampler, gl_FragCoord.xy, inUv.xy, uPc.factor.xy, uPc.viewportSizeInvSize.zw).r;
    } else if (CORE_BLUR_COLOR_TYPE == CORE_BLUR_TYPE_SOFT_DOWNSCALE_RGB) {
        // dir not used
        outColor.rgb =
            SoftDownscaleRGB(uTex, uSampler, gl_FragCoord.xy, inUv.xy, uPc.factor.xy, uPc.viewportSizeInvSize.zw).rgb;
    } else if (CORE_BLUR_COLOR_TYPE == CORE_BLUR_TYPE_DOWNSCALE_RGBA) {
        // dir not used
        outColor.rgba =
            DownscaleRGBA(uTex, uSampler, gl_FragCoord.xy, inUv.xy, uPc.factor.xy, uPc.viewportSizeInvSize.zw).rgba;
    } else { // (CORE_BLUR_COLOR_TYPE == CORE_BLUR_TYPE_RGBA)
        outColor.rgba =
            GaussianBlurRGBA(uTex, uSampler, gl_FragCoord.xy, inUv.xy, uPc.factor.xy, uPc.viewportSizeInvSize.zw).rgba;
    }
}
