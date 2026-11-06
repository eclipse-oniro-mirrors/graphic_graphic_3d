#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// include

#include "3d/shaders/common/3d_dm_structures_common.h"

#include "common/gs_struct_common.h"
#include "common/gs_common.h"

// in / out

layout(location = 0) in uvec4 gsInputData;

layout(location = 0) out flat uint vColor;
layout(location = 1) out CORE_RELAXEDP vec2 vPosition;
layout(location = 2) out vec2 vSquarePosition;
// Per-gaussian lightweight SR attributes (3dgsSS "sr_params")
layout(location = 3) out vec3 vSrParams;

layout(set = 0, binding = 0, std140) readonly uniform uCameraMatrices
{
    GsCameraUboStruct cameraUbo;
};

layout(set = 0, binding = 1, std140) readonly uniform uGSDynamicDataBuffer
{
    GsDynamicDataStruct gsDynamicDatas[GS_MAX_NUM_GS_OBJECTS];
};

layout(set = 0, binding = 2, std430) readonly restrict buffer uSrParams
{
    float srParams[];
};

layout(set = 0, binding = 3, std430) readonly restrict buffer inSortedIndices
{
    uint sortedIndices[];
};

layout(push_constant, std430) uniform uPushConstant
{
    GsRenderPushConstantStruct uPc;
};

// Mirror GetGsObjectIndex in gs3d_precompute_projection.comp
int GetGsObjectIndex(const uint elementId) {
    for (int ii = 0; ii < int(uPc.numGsObjects); ii++) {
        if (elementId >= gsDynamicDatas[ii].startOffset && elementId < gsDynamicDatas[ii].endOffset) {
            return ii;
        }
    }
    return -1;
}

const CORE_RELAXEDP vec2 VERTEX_POSITION[4] = vec2[] (
    vec2(-1.0f, -1.0f);
    vec2(1.0f, -1.0f);
    vec2(1.0f, 1.0f);
    vec2(-1.0f, 1.0f);
);

#define GS_SCALER(x) (x <= 0.5f ? (x * 0.5f + 0.75f) : (x <= 0.7f ? (x + 0.5f) : (x * 0.16666667f + 1.0833333f)))

void main(void)
{
    const uvec4 dataPacked = gsInputData;
    if (dataPacked.x == 0u) {
        CORE_VERTEX_OUT(vec4(0, 0, 2, 1));
        return;
    }

    const vec2 screenPosition = unpackHalf2x16(dataPacked.x);
    uint colorPacked = dataPacked.y;
    const vec2 majorAxis = unpackHalf2x16(dataPacked.z);
    const vec2 minorAxis = unpackHalf2x16(dataPacked.w);

    const vec2 position = VERTEX_POSITION[gl_VertexIndex];
    
    const float gaussianOpacity = float(colorPacked & 0xFF) * (1.0f / 255.0f);
    
    const vec4 posOut = vec4(screenPosition + position.x * majorAxis + position.y * minorAxis, 0.0, 1.0f);
    vColor = colorPacked;

    {
        const uint gaussianId = sortedIndices[uint(gl_InstanceIndex)];
        const int gsIndex = GetGsObjectIndex(gaussianId);
        if (gsIndex < 0) {
            vSrParams = vec3(0.0f);
        } else {
            const uint srRow = 3u * (gaussianId - gsDynamicDatas[gsIndex].startOffset);
            vSrParams = vec3(srParams[srRow], srParams[srRow + 1u], srParams[srRow + 2u]);
        }
    }

    const float opacityScaler = GS_SCALER(gaussianOpacity);
    vPosition = position * GS_COVARIANCE_STDDEV * opacityScaler;
    vSquarePosition = position;

    CORE_VERTEX_OUT(posOut);
}