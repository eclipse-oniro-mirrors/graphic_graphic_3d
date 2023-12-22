/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef API_RENDER_RENDER_DATA_STORE_RENDER_PODS_H
#define API_RENDER_RENDER_DATA_STORE_RENDER_PODS_H

#include <cstdint>

#include <base/math/vector.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
/** \addtogroup group_render_renderdatastorerenderpods
 *  @{
 */
/** A POD for back buffer configuration */
struct NodeGraphBackBufferConfiguration {
    /** Max back buffer name length */
    static constexpr uint32_t CORE_MAX_BACK_BUFFER_NAME_LENGTH { 128 };

    /** Back buffer type */
    enum class BackBufferType : uint32_t {
        /** Undefined */
        UNDEFINED,
        /** Swapchain */
        SWAPCHAIN,
        /** GPU image */
        GPU_IMAGE,
        /** GPU image buffer copy */
        GPU_IMAGE_BUFFER_COPY,
    };

    /** Name of the GpuImage in render node graph json where rendering happens.
     * Image with this name must have been created by the application (or by the engine). */
    char backBufferName[CORE_MAX_BACK_BUFFER_NAME_LENGTH];

    /** Back buffer type */
    BackBufferType backBufferType = { BackBufferType::UNDEFINED };

    /** Handle to the final target.
     * - If backbufferType is SWAPCHAIN this handle is not used.
     * - If backbufferType is GPU_IMAGE this must point to a valid GpuImage.
     * - If backbufferType is GPU_IMAGE_BUFFER_COPY this must point to a valid (linear) GpuImage. */
    RenderHandleReference backBufferHandle;
    /* - If backbufferType is GPU_IMAGE_BUFFER_COPY this must point to a valid GpuBuffer where image data is copied. */
    RenderHandleReference gpuBufferHandle;

    /** Present */
    bool present { false };

    /** Binary semaphore for signaling end of frame, i.e. when rendered to back buffer */
    uint64_t gpuSemaphoreHandle { 0 };
};

/** Bloom post process configuration. */
struct BloomConfiguration {
    /** Bloom quality type enum */
    enum BloomQualityType {
        /** Low quality */
        QUALITY_TYPE_LOW = 0,
        /** Normal quality */
        QUALITY_TYPE_NORMAL = 1,
        /** High quality */
        QUALITY_TYPE_HIGH = 2,
    };
    /** Bloom type enum */
    enum BloomType {
        /** Normal, smooth to every direction */
        TYPE_NORMAL = 0,
        /** Blurred/Blooms more in horizontal direction */
        TYPE_HORIZONTAL = 1,
        /** Blurred/Blooms more in vertical direction */
        TYPE_VERTICAL = 2,
        /** Bilateral filter, uses depth if available */
        TYPE_BILATERAL = 3,
    };

    /** Bloom type */
    BloomType bloomType { BloomType::TYPE_NORMAL };
    /** Bloom quality type */
    BloomQualityType bloomQualityType { BloomQualityType::QUALITY_TYPE_NORMAL };

    /** Threshold hard (Default: 1.0f) */
    float thresholdHard { 1.0f };
    /** Threshold soft (Default: 2.0f) */
    float thresholdSoft { 2.0f };
    /** Coefficient amount */
    float amountCoefficient { 0.25f };
    /** Dirt mask coefficient */
    float dirtMaskCoefficient { 0.0f };

    /** Optional dirt mask image handle */
    RenderHandleReference dirtMaskImage {};
    /** Use compute dispatches for bloom */
    bool useCompute { false };
};

/** Vignette post process configuration. */
struct VignetteConfiguration {
    /** Coefficient (Default: 0.5f) */
    float coefficient { 0.5f };
    /** Power (Default: 0.4f) */
    float power { 0.4f };
};

/** Color fringe post process configuration. */
struct ColorFringeConfiguration {
    /** Coefficient */
    float coefficient { 1.0f };
    /** Distance coefficient */
    float distanceCoefficient { 2.0f };
};

/** Dither post process configuration. */
struct DitherConfiguration {
    /** Dither type enum */
    enum DitherType {
        /** Interleaved noise */
        INTERLEAVED_NOISE = 0,
        /** Interleaved noise */
        TRIANGLE_NOISE = 0,
        /** Interleaved noise */
        TRIANGLE_NOISE_RGB = 0,
    };

    /* Dither type */
    DitherType ditherType { DitherType::INTERLEAVED_NOISE };

    /* Amount coefficient */
    float amountCoefficient { 0.1f };
};

struct BlurConfiguration {
    /** Blur quality type enum */
    enum BlurQualityType {
        /** Low */
        QUALITY_TYPE_LOW = 0,
        /** Normal */
        QUALITY_TYPE_NORMAL = 1,
        /** Gaussian like, more smooth */
        QUALITY_TYPE_HIGH = 2,
    };
    /** Blur type enum */
    enum BlurType {
        /** Normal, smooth to every direction */
        TYPE_NORMAL = 0,
        /** Blurred more in horizontal direction */
        TYPE_HORIZONTAL = 1,
        /** Blurred more in vertical direction */
        TYPE_VERTICAL = 2,
    };

    /** Blur type */
    BlurType blurType { BlurType::TYPE_NORMAL };
    /** Blur quality type */
    BlurQualityType blurQualityType { BlurQualityType::QUALITY_TYPE_NORMAL };
    /** Blur desired filter size in pixels. Might not be exact. */
    float filterSize { 1.0f };
    /** Blur quality max mip. If mips available. With this one could only blur to first (few) mips */
    uint32_t maxMipLevel { ~0u };
};

/** Tonemap configuration. */
struct TonemapConfiguration {
    /** Tonemap type */
    enum TonemapType {
        /** Aces */
        TONEMAP_ACES = 0,
        /** Aces 2020 */
        TONEMAP_ACES_2020 = 1,
        /** Filmic */
        TONEMAP_FILMIC = 2,
    };

    /** Tonemap type */
    TonemapType tonemapType { TonemapType::TONEMAP_ACES };
    /** Exposure */
    float exposure { 0.7f };
};

/** Opto-electronic conversion functions. */
struct ColorConversionConfiguration {
    /** Tonemap type */
    enum ConversionFunctionType {
        /** Linear -> no conversion in normal situation. */
        CONVERSION_LINEAR = 0,
        /** Linear to sRGB conversion */
        CONVERSION_LINEAR_TO_SRGB = 1,
    };

    /** Conversion function type */
    ConversionFunctionType conversionFunctionType { ConversionFunctionType::CONVERSION_LINEAR };
};

/** Fxaa Antialiasing configuration. */
struct FxaaConfiguration {
    enum class Sharpness { SOFT = 0, MEDIUM = 1, SHARP = 2 };
    enum class Quality { LOW = 0, MEDIUM = 1, HIGH = 2 };

    /** Edge sharpness. */
    Sharpness sharpness { Sharpness::SHARP };
    /** Overall edge quality. */
    Quality quality { Quality::MEDIUM };
};

/** Taa Antialiasing configuration. */
struct TaaConfiguration {
    enum class Sharpness { SOFT = 0, MEDIUM = 1, SHARP = 2 };
    enum class Quality { LOW = 0, MEDIUM = 1, HIGH = 2 };

    /** Edge sharpness. */
    Sharpness sharpness { Sharpness::SHARP };
    /** Overall edge quality. */
    Quality quality { Quality::MEDIUM };
};

/** Post process configuration POD. */
struct PostProcessConfiguration {
    /** Post process enable flags. Used in shader as well, must match render_post_process_structs_common.h */
    enum PostProcessEnableFlagbits : uint32_t {
        /** Enable tonemap */
        ENABLE_TONEMAP_BIT = (1 << 0),
        /** Enable vignette */
        ENABLE_VIGNETTE_BIT = (1 << 1),
        /** Enable dither */
        ENABLE_DITHER_BIT = (1 << 2),
        /** Enable color conversion */
        ENABLE_COLOR_CONVERSION_BIT = (1 << 3),
        /** Enable fringe */
        ENABLE_COLOR_FRINGE_BIT = (1 << 4),

        /** Enable blur */
        ENABLE_BLUR_BIT = (1 << 8),
        /** Enable bloom */
        ENABLE_BLOOM_BIT = (1 << 9),
        /** Enable FXAA */
        ENABLE_FXAA_BIT = (1 << 10),
        /** Enable TAA */
        ENABLE_TAA_BIT = (1 << 11),
    };
    using PostProcessEnableFlags = uint32_t;

    /** Factor indices. */
    enum PostProcessFactorIndices : uint32_t {
        INDEX_TONEMAP = 0,
        INDEX_VIGNETTE = 1,
        INDEX_DITHER = 2,
        INDEX_COLOR_CONVERSION = 3,
        INDEX_COLOR_FRINGE = 4,

        INDEX_BLUR = 8,
        INDEX_BLOOM = 9,
        INDEX_FXAA = 10,
        INDEX_TAA = 11,

        INDEX_FACTOR_COUNT = 14,
    };
    /** Enabled flags */
    PostProcessEnableFlags enableFlags { 0u };

    /** Tonemap configuration */
    TonemapConfiguration tonemapConfiguration;
    /** Vignette configuration */
    VignetteConfiguration vignetteConfiguration;
    /** Dither configuration */
    DitherConfiguration ditherConfiguration;
    /** Color conversion configuration */
    ColorConversionConfiguration colorConversionConfiguration;

    /** Color fringe configuration */
    ColorFringeConfiguration colorFringeConfiguration;

    /** Bloom configuration */
    BloomConfiguration bloomConfiguration;
    /** Blur configuration */
    BlurConfiguration blurConfiguration;

    /** Fxaa antialiasing configuration */
    FxaaConfiguration fxaaConfiguration;
    /** Taa antialiasing configuration */
    TaaConfiguration taaConfiguration;
};

/** Render post process configuration (must match render_post_process_structs_common.h */
struct RenderPostProcessConfiguration {
    /* Single uvec4 for flags .x has post process specialization flags */
    BASE_NS::Math::UVec4 flags { 0, 0, 0, 0 };
    // .x = delta time (ms), .y = tick delta time (ms), .z = tick total time (s), .w = frame index (asuint)
    BASE_NS::Math::Vec4 renderTimings { 0.0f, 0.0f, 0.0f, 0.0f };
    /* All post process factors */
    BASE_NS::Math::Vec4 factors[PostProcessConfiguration::INDEX_FACTOR_COUNT];
};

/** Default render pod store for shader specialization */
struct ShaderSpecializationRenderPod {
    /* Max supported specialization constants */
    static constexpr uint32_t MAX_SPECIALIZATION_CONSTANT_COUNT { 8u };
    /* Constant flags are mapped in order, use constant_id 0, 1.. in shaders.
     * Do not leave unused constant ids.
     */
    struct ConstantFlags {
        /** Value */
        uint32_t value { 0u };
    };

    /** Count of constants */
    uint32_t specializationConstantCount { 0u };
    /** Specialization data */
    ConstantFlags specializationFlags[MAX_SPECIALIZATION_CONSTANT_COUNT] {};
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_RENDER_DATA_STORE_RENDER_PODS_H
