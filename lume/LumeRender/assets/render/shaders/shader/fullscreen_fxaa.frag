#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_common.h"
#include "render/shaders/common/render_tonemap_common.h"

#define G3D_FXAA_PATCHES   1
#define FXAA_PC_CONSOLE    1
#define FXAA_GLSL_130      1
#define FXAA_GREEN_AS_LUMA 1
#include "common/fxaa_reference.h"

// sets

#include "render/shaders/common/render_post_process_layout_common.h"

layout(set = 1, binding = 0) uniform texture2D uTex;
#if (FXAA_GREEN_AS_LUMA == 0)
layout(set = 1, binding = 1) uniform texture2D uTexAlpha;
layout(set = 1, binding = 2) uniform sampler uSampler;
#else
layout(set = 1, binding = 1) uniform sampler uSampler;
#endif

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

void main(void)
{
    const vec2 textureSizeInv = uPc.viewportSizeInvSize.zw;
    const vec2 texelCenter = inUv;

    const vec4 aaPixel = FxaaPixelShader(
        texelCenter,  // FxaaFloat2 pos
        vec4(texelCenter - textureSizeInv.xy * 0.5,
            texelCenter + textureSizeInv.xy * 0.5), // FxaaFloat4 fxaaConsolePosPos
        uTex,             // FxaaTex tex
        uSampler,         // FxaaSampler samp
        vec2(0), // unused FxaaFloat2 fxaaQualityRcpFrame
        vec4(-textureSizeInv.xy, textureSizeInv.xy),     //  FxaaFloat4 fxaaConsoleRcpFrameOpt,
        2 * vec4(-textureSizeInv.xy, textureSizeInv.xy), //  FxaaFloat4 fxaaConsoleRcpFrameOpt2,
        vec4(0), // unused FxaaFloat4 fxaaConsole360RcpFrameOpt2,
        0,       // unused FxaaFloat fxaaQualitySubpix, on the range [0 no subpixel filtering but sharp to 1 maximum and blurry, .75 default]
        0,       // unused FxaaFloat fxaaQualityEdgeThreshold, on the range [0.063 (slow, overkill, good) to 0.333 (fast), 0.125 default]
        0,       // unused FxaaFloat fxaaQualityEdgeThresholdMin, on the range [0.0833 (fast) to 0.0312 (good), with 0.0625 called "high quality"]
        uPc.factor.x, // FxaaFloat fxaaConsoleEdgeSharpness, // 8.0 is sharper, 4.0 is softer, 2.0 is really soft (good for vector graphics inputs)
        uPc.factor.y, // FxaaFloat fxaaConsoleEdgeThreshold, on the range [0.063 (slow, overkill, good) to 0.333 (fast), 0.125 default]
        uPc.factor.z, // FxaaFloat fxaaConsoleEdgeThresholdMin, on the range [0.0833 (fast) to 0.0312 (good), with 0.0625 called "high quality"]
        vec4(0)  // unused FxaaFloat4 fxaaConsole360ConstDir
    );
#if (FXAA_GREEN_AS_LUMA == 0)
    // luma is in uTex alpha, alpha is in separate texture
    const float alpha = texture(sampler2D(uTexAlpha, uSampler), inUv).x;

    outColor = vec4((aaPixel.xyz), alpha);
#else 
    outColor = aaPixel;
#endif
}
