#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "3d/shaders/common/3d_dm_indirect_lighting_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_common.h"
#include "render/shaders/common/render_tonemap_common.h"

// sets and specializations

#include "3d/shaders/common/3d_dm_frag_layout_common.h"
#define CORE3D_DM_FW_FRAG_INPUT 1
#include "3d/shaders/common/3d_dm_brdf_common.h"
#include "3d/shaders/common/3d_dm_inout_common.h"
#include "3d/shaders/common/3d_dm_inplace_fog_common.h"
#include "3d/shaders/common/3d_dm_inplace_post_process.h"
#include "3d/shaders/common/3d_dm_inplace_sampling_common.h"
#include "3d/shaders/common/3d_dm_lighting_common.h"
#include "3d/shaders/common/3d_dm_target_packing_common.h"

// in / out

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outVelocityNormal;

///////////////////////////////////////////////////////////////////////////////
// "main" functions
#define CORE_PBR_LIGHTING_EPSILON 0.0001

struct ReflectionDataStruct {
    CORE_RELAXEDP float mipCount;
    CORE_RELAXEDP float screenPercentage;
    CORE_RELAXEDP float imageResX;
    CORE_RELAXEDP float imageResY;
};

ReflectionDataStruct GetUnpackReflectionPlaneData()
{
    const uint instanceIdx = 0;
    // clearcoat roughness used as a slot to pass in the factor data
    vec4 factor = uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_CLEARCOAT_ROUGHNESS_IDX];
    ReflectionDataStruct rds;
    rds.mipCount = factor.x;
    rds.screenPercentage = factor.y;
    rds.imageResX = max(1.0, factor.z);
    rds.imageResY = max(1.0, factor.w);
    return rds;
}

float GetLodForReflectionSample(const float roughness, const float mipCount)
{
    return mipCount * roughness;
}

float GetLodForRadianceSample(const float roughness)
{
    return uEnvironmentData.values.x * roughness;
}

vec3 GetDuDvSample(const vec4 inputUv)
{
    return GetEmissiveSample(inputUv) * GetUnpackEmissiveColor(0);
}

vec3 GetReflectionSample(const vec2 inputUv, const float lod)
{
    return textureLod(uSampTextures[CORE_MATERIAL_TEX_CLEARCOAT_ROUGHNESS_IDX], inputUv, lod).xyz;
}

vec3 GetIrradianceSample(const vec3 worldNormal)
{
    const vec3 worldNormalEnv = mat3(uEnvironmentData.envRotation) * worldNormal;
    return unpackIblIrradianceSH(worldNormalEnv, uEnvironmentData.shIndirectCoefficients) *
           uEnvironmentData.indirectDiffuseColorFactor.rgb;
}

vec3 GetRadianceSample(const vec3 worldReflect, const float roughness)
{
    const CORE_RELAXEDP float cubeLod = GetLodForRadianceSample(roughness);
    const vec3 worldReflectEnv = mat3(uEnvironmentData.envRotation) * worldReflect;
    return unpackIblRadiance(textureLod(uSampRadiance, worldReflectEnv, cubeLod)) *
           uEnvironmentData.indirectSpecularColorFactor.rgb;
}

vec3 GetTransmissionRadianceSample(const vec2 fragUv, const vec3 worldReflect, const float roughness)
{
    // NOTE: this makes a pre color selection based on alpha
    // we would generally need an extra flag, the default texture is black with alpha zero
    const CORE_RELAXEDP float lod = GetLodForRadianceSample(roughness);
    vec4 color = textureLod(uSampColorPrePass, fragUv, lod).rgba;
    if (color.a < 0.5f) {
        // sample environment if the default pre pass color was 0.0 alpha
        color.rgb = GetRadianceSample(worldReflect, roughness);
    }
    return color.rgb;
}

float GetRippleHeight(const vec4 rippleUv)
{
    const vec2 uv = GetFinalSamplingUV(rippleUv, CORE_MATERIAL_TEXCOORD_INFO_AO_BIT, CORE_MATERIAL_PACK_TEX_AO_UV_IDX);
    return texture(uSampTextures[CORE_MATERIAL_TEX_AO_IDX], uv).x;
}

vec4 PlaneReflector(const vec2 fragUv)
{
    const uint instanceIdx = 0;

    CORE_RELAXEDP vec4 baseColor = GetBaseColorSample(inUv) * GetUnpackBaseColor(instanceIdx) * inColor;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) ==
        CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) {
        if (baseColor.a < GetUnpackAlphaCutoff(instanceIdx)) {
            discard;
        }
    }
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_OPAQUE_BIT) == CORE_MATERIAL_OPAQUE_BIT) {
        baseColor.a = 1.0;
    } else {
        baseColor = Unpremultiply(baseColor);
    }

    DefaultMaterialUnpackedSceneTimingStruct sceneTiming = GetUnpackSceneTiming(uGeneralData);
    const vec2 baseUv = inUv.xy;
    const vec3 dudvColor = GetDuDvSample(inUv);
    const vec3 normNormal = normalize(inNormal.xyz);
    vec3 N = normNormal;
    const mat3 tbn = CalcTbnMatrix(normNormal, inTangentW);
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_NORMAL_MAP_BIT) == CORE_MATERIAL_NORMAL_MAP_BIT) {
        N = GetNormalSample(inUv, instanceIdx);
        const float normalScale = GetUnpackNormalScale(instanceIdx);
        N = CalcFinalNormal(tbn, N, normalScale);
    }

    const uint cameraIdx = GetUnpackFlatIndicesCameraIdx(inIndices);
    const vec3 camPos = uCameras[cameraIdx].viewInv[3].xyz;
    const vec3 V = normalize(camPos.xyz - inPos);
    const float surfaceViewAngle = 1.0 - clamp(abs(dot(N, V)), 0.0, 1.0);
    const float samplingDelta = mix(1.0, 6.0, surfaceViewAngle) / float(textureSize(uSampTextures[CORE_MATERIAL_TEX_AO_IDX], 0).x);
    
    // Reduce effect near screen edges
    const vec2 edgeFade = smoothstep(0.0, 0.1, fragUv) * smoothstep(1.0, 0.9, fragUv);

    // Compute ripple height and preturb surface normal.
    const float rippleHeightCenter = GetRippleHeight(inUv);
    const float heightR = GetRippleHeight(inUv + vec4(samplingDelta, 0.0, 0.0, 0.0));
    const float heightU = GetRippleHeight(inUv + vec4(0.0, samplingDelta, 0.0, 0.0));

    const float rippleTime = sceneTiming.tickTotalTime.x;

    // Add noise for more realistic water (i.e. slight waves)
    const float noiseFactor1 = sin(inUv.x * 20.0 + rippleTime) * 0.1;
    const float noiseFactor2 = sin(inUv.y * 25.0 + rippleTime * 1.2) * 0.05;
    const float noiseFactor3 = cos((inUv.x + inUv.y) * 35.0 + rippleTime * 0.8) * 0.05;

    const float combinedNoiseFactor = noiseFactor1 + noiseFactor2 + noiseFactor3;

    const float heightL = GetRippleHeight(inUv + vec4(-samplingDelta, 0.0, 0.0, 0.0));
    const float heightD = GetRippleHeight(inUv + vec4(0.0, -samplingDelta, 0.0, 0.0));
    const float rippleHeight = ((rippleHeightCenter + heightL + heightR + heightD + heightU) / 5.0 + combinedNoiseFactor) * edgeFade.x * edgeFade.y;
    const float dHdX = (heightR - heightL) / (2.0 * samplingDelta);
    const float dHdY = (heightU - heightD) / (2.0 * samplingDelta);
    
    const vec3 rippleNormal = normalize(vec3(-dHdX, -dHdY, 1.0));
    N = normalize(tbn * rippleNormal);

    // if no backface culling we flip automatically
    N = gl_FrontFacing ? N : -N;
    const float NoV = clamp(dot(N, V), CORE_PBR_LIGHTING_EPSILON, 1.0);

    const CORE_RELAXEDP vec4 material = GetMaterialSample(inUv) * GetUnpackMaterial(instanceIdx);
    InputBrdfData brdfData = CalcBRDFMetallicRoughness(baseColor, normNormal, material);

    const float f0 = 0.04;
    const float fresnel = clamp(mix(f0, 1.0, pow(1.0 - NoV, 5.0)), 0.7, 1.3);
    
    CORE_RELAXEDP float transmission;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_TRANSMISSION_BIT) == CORE_MATERIAL_TRANSMISSION_BIT) {
        transmission = GetTransmissionSample(inUv) * GetUnpackTransmission(instanceIdx);
        brdfData.diffuseColor *= (1.0 - transmission);
    }

    ShadingData shadingData;
    shadingData.pos = inPos.xyz;
    shadingData.N = N;
    shadingData.NoV = NoV;
    shadingData.V = V;
    shadingData.f0 = brdfData.f0;
    shadingData.alpha2 = brdfData.alpha2;
    shadingData.diffuseColor = brdfData.diffuseColor;
    CORE_RELAXEDP const float roughness = max(brdfData.roughness, 0.02);

    vec3 color = brdfData.diffuseColor;

    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT) ==
        CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT) {
        color = CalculateLighting(shadingData, CORE_MATERIAL_FLAGS);
    }

    const vec2 dudvDistortion = (dudvColor.xy * 2.0 - 1.0) * 0.05;
    const vec2 movingFragUv = fragUv.xy + dudvDistortion;

    // reflection
    const ReflectionDataStruct rds = GetUnpackReflectionPlaneData();
    const CORE_RELAXEDP float reflLod = GetLodForReflectionSample(roughness, rds.mipCount);
    vec2 scaledFragUv;
    CORE_GET_FRAGCOORD_UV(
        scaledFragUv, gl_FragCoord.xy * rds.screenPercentage, 1.0 / vec2(rds.imageResX, rds.imageResY));
    scaledFragUv += rippleNormal.xy * rippleHeight * 10.0;
    const CORE_RELAXEDP vec3 radianceSample = GetReflectionSample(scaledFragUv.xy, reflLod);

    CORE_RELAXEDP vec3 radiance = radianceSample * vec3(fresnel);
    CORE_RELAXEDP vec3 irradiance = vec3(0.0);
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT) ==
        CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT) {
        const CORE_RELAXEDP float ao = clamp(GetAOSample(inUv, instanceIdx) * GetUnpackAO(instanceIdx), 0.0, 1.0);
        // lambert baked into irradianceSample (SH)
        irradiance = GetIrradianceSample(N) * shadingData.diffuseColor * ao;

        const CORE_RELAXEDP vec3 fIndirect = EnvBRDFApprox(shadingData.f0.xyz, roughness, NoV);
        radiance *= fIndirect;

        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_TRANSMISSION_BIT) == CORE_MATERIAL_TRANSMISSION_BIT) {
            // NOTE: ATM use direct refract (no sphere mapping)
            const vec3 rr = -V; // refract(-V, N, 1.0 / ior);
            const vec3 Ft = GetTransmissionRadianceSample(movingFragUv, rr, roughness) * baseColor.rgb;
            irradiance *= (1.0 - transmission);
            irradiance = mix(irradiance, Ft, transmission);
        }
    }
    color += irradiance + radiance;

    InplaceFogBlock(CORE_CAMERA_FLAGS, inPos.xyz, camPos.xyz, vec4(color, baseColor.a), color);

    outVelocityNormal = GetPackVelocityAndNormal(GetFinalCalculatedVelocity(inPos.xyz, inPrevPosI.xyz, cameraIdx), N);

    return vec4(color * baseColor.a, baseColor.a);
}

/**
 * fragment shader for plane reflector
 */
void main(void)
{
    vec2 fragUv;
    CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, GetUnpackViewport(uGeneralData).zw);

    outColor = PlaneReflector(fragUv);

    if (CORE_POST_PROCESS_FLAGS > 0) {
        InplacePostProcess(fragUv, outColor);
    }
}
