/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef GLES_GPU_IMAGE_GLES_H
#define GLES_GPU_IMAGE_GLES_H

#include <base/math/vector.h>
#include <render/device/gpu_resource_desc.h>
#include <render/gles/intf_device_gles.h>
#include <render/namespace.h>

#include "device/gpu_image.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class DeviceGLES;
struct GpuImageDesc;
struct GpuImagePlatformDataGL final : public GpuImagePlatformData {
    // GL_TEXTURE_2D,GL_TEXTURE_CUBE_MAP,GL_TEXTURE_2D_MULTISAMPLE etc.
    // (ie. viewtype, can be 0. (no view, used for backbuffer "images" and renderbuffers))
    uint32_t type;
    uint32_t image;          // Texture handle (0 for "no view")
    uint32_t format;         // GL_RGB etc
    uint32_t internalFormat; // GL_RGBA16F etc..
    uint32_t dataType;       // GL_FLOAT etc
    uint32_t bytesperpixel;
    struct {
        bool compressed;
        uint8_t blockW;
        uint8_t blockH;
        uint32_t bytesperblock;
    } compression;
    BASE_NS::Math::UVec4 swizzle;
    uint32_t renderBuffer; // For render targets... (can not be sampled or used in compute)
    uintptr_t eglImage;    // For creating image from EGLImage
    uint32_t mipLevel { PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS };
};

class GpuImageGLES final : public GpuImage {
public:
    GpuImageGLES(Device& device, const GpuImageDesc& desc);
    GpuImageGLES(Device& device, const GpuImageDesc& desc, const GpuImagePlatformData& platformData);
    ~GpuImageGLES();

    const GpuImageDesc& GetDesc() const override;
    const GpuImagePlatformData& GetBasePlatformData() const override;
    const GpuImagePlatformDataGL& GetPlatformData() const;

    static GpuImagePlatformDataGL GetPlatformData(const DeviceGLES& device, BASE_NS::Format format);

    AdditionalFlags GetAdditionalFlags() const override;

private:
    DeviceGLES& device_;

    GpuImagePlatformDataGL plat_;
    GpuImageDesc desc_;
    // in normal situations owns all the vulkan resources
    bool ownsResources_ { true };
};
RENDER_END_NAMESPACE()

#endif // GLES_GPU_IMAGE_GLES_H
