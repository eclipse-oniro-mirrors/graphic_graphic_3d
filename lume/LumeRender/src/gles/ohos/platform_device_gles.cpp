/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <external_window.h>

#include "gles/device_gles.h"
#include "gles/egl_functions.h"
#include "gles/gl_functions.h"
#include "gles/gpu_image_gles.h"
#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
BASE_NS::unique_ptr<GpuImage> DeviceGLES::CreateGpuImageView(
    const GpuImageDesc& desc, const BackendSpecificImageDesc& platformData)
{
    BASE_NS::unique_ptr<GpuImage> image;
    PLUGIN_ASSERT(IsActive());
    GpuImagePlatformDataGL data {};
#if RENDER_HAS_GLES_BACKEND
    if (backendType_ == DeviceBackendType::OPENGLES) {
        const ImageDescGLES& tmp = (const ImageDescGLES&)platformData;
        data.type = tmp.type;
        data.image = tmp.image;
        data.bytesperpixel = tmp.bytesperpixel;
        data.dataType = tmp.dataType;
        data.format = tmp.format;
        data.internalFormat = tmp.internalFormat;
        data.eglImage = tmp.eglImage;
        data.hwBuffer = tmp.platformHwBuffer;
        data.swizzle = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };

        if (!data.hwBuffer) {
            image = CreateGpuImageView(desc, data);
        } else if (eglCreateImageKHR && eglDestroyImageKHR) {
            auto* nativeBufferPtr = reinterpret_cast<OH_NativeBuffer*>(tmp.platformHwBuffer);
            auto nativeWindowBufferPtr = OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(nativeBufferPtr);

            const auto dsp = static_cast<const DevicePlatformDataGLES&>(eglState_.GetPlatformData()).display;
            static constexpr EGLint attrs[] = { EGL_IMAGE_PRESERVED, EGL_TRUE, EGL_NONE };
            const auto eglImage =
                eglCreateImageKHR(dsp, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_OHOS, nativeWindowBufferPtr, attrs);
            if (!eglImage) {
                PLUGIN_LOG_E("eglCreateImageKHR failed %d", eglGetError());
                return {};
            }
            GpuImageDesc finalDesc = GetImageDescFromHwBufferDesc(tmp.platformHwBuffer);

            data.type = GL_TEXTURE_EXTERNAL_OES;
            data.eglImage = reinterpret_cast<uintptr_t>(eglImage);
            image = CreateGpuImageView(finalDesc, data);

            eglDestroyImageKHR(dsp, eglImage);
        }
    }
#endif
#if RENDER_HAS_GL_BACKEND
    if (backendType_ == DeviceBackendType::OPENGL) {
        const ImageDescGL& tmp = (const ImageDescGL&)platformData;
        data.type = tmp.type;
        data.image = tmp.image;
        data.bytesperpixel = tmp.bytesperpixel;
        data.dataType = tmp.dataType;
        data.format = tmp.format;
        data.internalFormat = tmp.internalFormat;
        data.swizzle = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };
        image = CreateGpuImageView(desc, data);
    }
#endif
    return image;
}
RENDER_END_NAMESPACE()
