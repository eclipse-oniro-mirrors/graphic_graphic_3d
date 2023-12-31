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

#include "texture_layer.h"

#include <include/gpu/GrBackendSurface.h>
#ifndef NEW_SKIA
#include <include/gpu/GrContext.h>
#else
#include <include/gpu/GrDirectContext.h>
#endif
#include <include/gpu/gl/GrGLInterface.h>
#include <native_buffer.h>
#include <render_service_base/include/pipeline/rs_recording_canvas.h>
#include <render_service_base/include/property/rs_properties_def.h>
#include <render_service_client/core/pipeline/rs_node_map.h>
#include <render_service_client/core/transaction/rs_interfaces.h>
#include <render_service_client/core/ui/rs_canvas_node.h>
#include <render_service_client/core/ui/rs_root_node.h>
#include <render_service_client/core/ui/rs_surface_node.h>
#include <surface_buffer.h>
#include <surface_utils.h>
#include <window.h>

#include "3d_widget_adapter_log.h"
#include "graphics_manager.h"
#include "offscreen_context_helper.h"
#include "widget_trace.h"

namespace OHOS {
namespace Render3D {

TextureLayer::TextureLayer(int32_t key) : key_(key)
{
    auto& gfxManager = GraphicsManager::GetInstance();
    backend_ = gfxManager.GetRenderBackendType(key);
    if (backend_ == RenderBackend::GLES) {
        gfxManager.GetOrCreateOffScreenContext(EGL_NO_CONTEXT);
    }
}

void TextureLayer::UpdateRenderFinishFuture(std::shared_future<void> &ftr)
{
    std::lock_guard<std::mutex> lk(ftrMut_);
    image_.ftr_ = ftr;
}

void TextureLayer::SetParent(std::shared_ptr<Rosen::RSNode>& parent)
{
    parent_ = parent;
    // delete previous rs node reference
    RemoveChild();

    if (parent_ && rsNode_) {
        parent_->AddChild(rsNode_, 0); // second paramenter is added child at the index of parent's children;
    }
}

void TextureLayer::RemoveChild()
{
    if (parent_ && rsNode_) {
        parent_->RemoveChild(rsNode_);
    }
}

void TextureLayer::AllocGLTexture(uint32_t width, uint32_t height)
{
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, 0);
}

void TextureLayer::AllocEglImage(uint32_t width, uint32_t height)
{
    OH_NativeBuffer_Config config {
        .width = width,
        .height = height,
        .format = GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_MEM_DMA
    };

    nativeBuffer_ = OH_NativeBuffer_Alloc(&config);
    if (!nativeBuffer_) {
        WIDGET_LOGE("native buffer null");
        return;
    }

    surfaceBuffer_ = SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer_);
    if (!surfaceBuffer_) {
        WIDGET_LOGE("surface buffer null");
        FreeNativeBuffer();
        return;
    }

    nativeWindowBuffer_ = CreateNativeWindowBufferFromSurfaceBuffer(&surfaceBuffer_);
    if (!nativeWindowBuffer_) {
        WIDGET_LOGE("create native window buffer fail");
        FreeNativeBuffer();
        return;
    }

    EGLint attrs[] = {
        EGL_IMAGE_PRESERVED, EGL_TRUE, EGL_NONE,
    };
    auto disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglImage_ = eglCreateImageKHR(disp, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_OHOS, nativeWindowBuffer_, attrs);
    if (eglImage_ == EGL_NO_IMAGE_KHR) {
        WIDGET_LOGE("create egl image fail %d", eglGetError());
        FreeNativeWindowBuffer();
        FreeNativeBuffer();
        return;
    }

    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, static_cast<GLeglImageOES>(eglImage_));
}

void* TextureLayer::CreateNativeWindow(uint32_t width, uint32_t height)
{
    struct Rosen::RSSurfaceNodeConfig surfaceNodeConfig = { .SurfaceNodeName = std::string("SceneViewer Model") +
        std::to_string(key_)};
    rsNode_ = Rosen::RSSurfaceNode::Create(surfaceNodeConfig, false);
    if (!rsNode_) {
        WIDGET_LOGE("Create rs node fail");
        return nullptr;
    }

    auto surfaceNode = OHOS::Rosen::RSBaseNode::ReinterpretCast<OHOS::Rosen::RSSurfaceNode>(rsNode_);
    surfaceNode->SetFrameGravity(Rosen::Gravity::RESIZE);
    if (surface_ == SurfaceType::SURFACE_WINDOW) {
        surfaceNode->SetHardwareEnabled(true);
    }
    if (surface_ == SurfaceType::SURFACE_TEXTURE) {
        surfaceNode->SetHardwareEnabled(false);
    }

    producerSurface_ = surfaceNode->GetSurface();
    if (!producerSurface_) {
        WIDGET_LOGE("Get producer surface fail");
        return nullptr;
    }
    auto ret = SurfaceUtils::GetInstance()->Add(producerSurface_->GetUniqueId(), producerSurface_);
    if (ret != SurfaceError::SURFACE_ERROR_OK) {
        WIDGET_LOGE("add surface error");
        return nullptr;
    }

    producerSurface_->SetQueueSize(3); // 3 seems ok
    producerSurface_->SetUserData("SURFACE_STRIDE_ALIGNMENT", "8");
    producerSurface_->SetUserData("SURFACE_FORMAT", std::to_string(GRAPHIC_PIXEL_FMT_RGBA_8888));
    producerSurface_->SetUserData("SURFACE_WIDTH", std::to_string(width));
    producerSurface_->SetUserData("SURFACE_HEIGHT", std::to_string(height));
    auto window = CreateNativeWindowFromSurface(&producerSurface_);

    return reinterpret_cast<void *>(window);
}

void TextureLayer::ConfigWindow(float offsetX, float offsetY, float width, float height, float scale,
    bool recreateWindow)
{
    float widthScale = image_.textureInfo_.widthScale_;
    float heightScale = image_.textureInfo_.heightScale_;
    if (surface_ == SurfaceType::SURFACE_WINDOW || surface_ == SurfaceType::SURFACE_TEXTURE) {
        image_.textureInfo_.recreateWindow_ = recreateWindow;

        if (!image_.textureInfo_.nativeWindow_) {
            image_.textureInfo_.nativeWindow_ = reinterpret_cast<void *>(CreateNativeWindow(
                static_cast<uint32_t>(width * widthScale), static_cast<uint32_t>(height * heightScale)));
        }
        // need check recreate window flag
        NativeWindowHandleOpt(reinterpret_cast<OHNativeWindow *>(image_.textureInfo_.nativeWindow_),
            SET_BUFFER_GEOMETRY, static_cast<uint32_t>(width * scale * widthScale),
            static_cast<uint32_t>(height * scale * heightScale));
        rsNode_->SetBounds(offsetX, offsetY, width, height);
    }
}

void TextureLayer::ConfigTexture(float width, float height)
{
    if ((surface_ == SurfaceType::SURFACE_BUFFER) && !image_.textureInfo_.textureId_) {
        AutoRestore scope;

        GraphicsManager::GetInstance().BindOffScreenContext();
        glGenTextures(1, &image_.textureInfo_.textureId_);

        glBindTexture(GL_TEXTURE_2D, image_.textureInfo_.textureId_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        AllocGLTexture(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

#if defined(UNIFY_RENDER) && (UNIFY_RENDER == 1)
        AllocEglImage(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
#endif
    }
}

TextureInfo TextureLayer::OnWindowChange(float offsetX, float offsetY, float width, float height, float scale,
    bool recreateWindow, SurfaceType surfaceType)
{
    DestroyRenderTarget();
    surface_ = surfaceType;
    offsetX_ = offsetX;
    offsetY_ = offsetY;
    if (image_.textureInfo_.width_ != static_cast<uint32_t>(width) ||
        image_.textureInfo_.height_ != static_cast<uint32_t>(height)) {
        needsRecreateSkImage_ = true;
    }
    image_.textureInfo_.width_ = static_cast<uint32_t>(width);
    image_.textureInfo_.height_ = static_cast<uint32_t>(height);

    ConfigWindow(offsetX, offsetY, width, height, scale, recreateWindow);
    ConfigTexture(width, height);

    WIDGET_LOGD("TextureLayer OnWindowChange offsetX %f, offsetY %f, width %d, height %d, float scale %f,"
        "recreateWindow %d window empty %d", offsetX, offsetY, image_.textureInfo_.width_, image_.textureInfo_.height_,
        scale, recreateWindow, image_.textureInfo_.nativeWindow_ == nullptr);
    return image_.textureInfo_;
}

TextureInfo TextureLayer::OnWindowChange(const WindowChangeInfo& windowChangeInfo)
{
    DestroyRenderTarget();
    surface_ = windowChangeInfo.surfaceType;
    offsetX_ = (int)windowChangeInfo.offsetX;
    offsetY_ = (int)windowChangeInfo.offsetY;
    if (image_.textureInfo_.width_ != static_cast<uint32_t>(windowChangeInfo.width) ||
        image_.textureInfo_.height_ != static_cast<uint32_t>(windowChangeInfo.height)) {
        needsRecreateSkImage_ = true;
    }
    image_.textureInfo_.width_ = static_cast<uint32_t>(windowChangeInfo.width);
    image_.textureInfo_.height_ = static_cast<uint32_t>(windowChangeInfo.height);

    image_.textureInfo_.widthScale_ = static_cast<float>(windowChangeInfo.widthScale);
    image_.textureInfo_.heightScale_ = static_cast<float>(windowChangeInfo.heightScale);

    ConfigWindow(windowChangeInfo.offsetX, windowChangeInfo.offsetY, windowChangeInfo.width,
        windowChangeInfo.height, windowChangeInfo.scale, windowChangeInfo.recreateWindow);
    ConfigTexture(windowChangeInfo.width, windowChangeInfo.height);

    WIDGET_LOGD("TextureLayer OnWindowChange-1 offsetX %d, offsetY %d, width %d, height %d, scale %f,widthScale %f,"
        "heightScale %f, recreateWindow %d window empty %d", offsetX_, offsetY_, image_.textureInfo_.width_,
        image_.textureInfo_.height_, windowChangeInfo.scale, image_.textureInfo_.widthScale_,
        image_.textureInfo_.heightScale_, windowChangeInfo.recreateWindow,
        image_.textureInfo_.nativeWindow_ == nullptr);
    return image_.textureInfo_;
}

void TextureLayer::FreeNativeBuffer()
{
    if (!nativeBuffer_) {
        return;
    }
    OH_NativeBuffer_Unreference(nativeBuffer_);
    nativeBuffer_ = nullptr;
}

void TextureLayer::FreeNativeWindowBuffer()
{
    if (!nativeWindowBuffer_) {
        return;
    }
    DestroyNativeWindowBuffer(nativeWindowBuffer_);
    nativeWindowBuffer_ = nullptr;
}

void TextureLayer::DestroyProducerSurface()
{
    if (!producerSurface_) {
        return;
    }
    auto surfaceUtils = SurfaceUtils::GetInstance();
    surfaceUtils->Remove(producerSurface_->GetUniqueId());
}

void TextureLayer::DestroyNativeWindow()
{
    WIDGET_LOGD("TextureLayer::DestroyNativeWindow");
    if (!image_.textureInfo_.nativeWindow_) {
        WIDGET_LOGW("TextureLayer::DestroyNativeWindow invalid window");
        return;
    }
    DestoryNativeWindow(reinterpret_cast<OHNativeWindow *>(image_.textureInfo_.nativeWindow_));
    image_.textureInfo_.nativeWindow_ = nullptr;
}

void TextureLayer::DestroyRenderTarget()
{
    // surface buffer release
    if (eglImage_ != EGL_NO_IMAGE_KHR) {
        auto disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglDestroyImageKHR(disp, eglImage_);
        eglImage_ = EGL_NO_IMAGE_KHR;
    }

    FreeNativeWindowBuffer();
    FreeNativeBuffer();

    if (image_.textureInfo_.textureId_ != 0U) {
        AutoRestore scope;
        GraphicsManager::GetInstance().BindOffScreenContext();
        glDeleteTextures(1, &image_.textureInfo_.textureId_);
    }

#if defined(DBG_DRAW_PIXEL) && (DBG_DRAW_PIXEL == 1)
    if (fbo_ != 0U) {
        AutoRestore scope;
        GraphicsManager::GetInstance().BindOffScreenContext();
        glDeleteFramebuffers(1, fbo_);
        fbo_ = 0U;
    }

    if (data_ != nullptr) {
        delete[] data_;
        data_ = nullptr;
    }
#endif

    // window release
    DestroyNativeWindow();
    DestroyProducerSurface();
    RemoveChild();
    rsNode_ = nullptr;
    parent_ = nullptr;
    image_.textureInfo_ = {};
}

TextureLayer::~TextureLayer()
{
    // explicit release resource before destructor
}

void TextureLayer::OnDraw(SkCanvas* canvas)
{
    if (surface_ == SurfaceType::SURFACE_WINDOW || surface_ == SurfaceType::SURFACE_TEXTURE) {
        return;
    }

    if (canvas == nullptr) {
        WIDGET_LOGE("TextureLayer::OnDraw canvas invalid")
        return;
    };

    #if defined(UNIFY_RENDER) && (UNIFY_RENDER == 1)
        onDraw(canvas);
    #else
        canvas->drawDrawable(this);
    #endif
}

void TextureLayer::onDraw(SkCanvas* canvas)
{
    WIDGET_SCOPED_TRACE("TextureLayer::onDraw");
    if (canvas == nullptr) {
        WIDGET_LOGE("TextureLayer::OnDraw canvas invalid")
        return;
    };
    std::lock_guard<std::mutex> lk(ftrMut_);
    if (image_.ftr_.valid()) {
        image_.ftr_.get();
    }
    #if defined(UNIFY_RENDER) && (UNIFY_RENDER == 1)
        DrawTextureUnifyRender(canvas);
    #else
        DrawTexture(canvas);
    #endif
}

#if defined(UNIFY_RENDER) && (UNIFY_RENDER == 1)
void TextureLayer::DrawTextureUnifyRender(SkCanvas* canvas)
{
#if defined(DBG_DRAW_PIXEL) && (DBG_DRAW_PIXEL == 1)
    // DEBUG copy texture to bitmap, no need ipc surfacebuffer
    ReadPixel();
    canvas->drawImage(MakePixelImage(), offsetX_, offsetY_);
    return;
#endif
#ifndef USE_ROSEN_DRAWING
    Rosen::RSSurfaceBufferInfo info {
        surfaceBuffer_, offsetX_, offsetY_, image_.textureInfo_.width_, image_.textureInfo_.height_
    };

    auto recordingCanvas = static_cast<Rosen::RSRecordingCanvas*>(canvas);
    recordingCanvas->DrawSurfaceBuffer(info);
#endif
}
#endif

void TextureLayer::DrawTexture(SkCanvas* canvas)
{
    WIDGET_SCOPED_TRACE("TextureLayer::DrawTexture");
    if (image_.textureInfo_.textureId_ == 0U) {
        WIDGET_LOGE("%s invalid texture %d", __func__, __LINE__);
        return;
    }

    if (image_.skImage_ == nullptr || needsRecreateSkImage_) {
        needsRecreateSkImage_ = false;
        GrGLTextureInfo textureInfo = { GL_TEXTURE_2D, image_.textureInfo_.textureId_, GL_RGBA8_OES };
        GrBackendTexture backendTexture(image_.textureInfo_.width_, image_.textureInfo_.height_,
            GrMipMapped::kNo, textureInfo);
#ifdef NEW_SKIA
        image_.skImage_ = SkImage::MakeFromTexture(canvas->recordingContext(), backendTexture, kTopLeft_GrSurfaceOrigin,
            kRGBA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB());
#else
        image_.skImage_ = SkImage::MakeFromTexture(canvas->getGrContext(), backendTexture, kTopLeft_GrSurfaceOrigin,
            kRGBA_8888_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGB());
#endif
        WIDGET_LOGW("%s Create SkImage %d", __func__, __LINE__);
    }

    canvas->drawImage(image_.skImage_, offsetX_, offsetY_);
}

#if defined(DBG_DRAW_PIXEL) && (DBG_DRAW_PIXEL == 1)
auto TextureLayer::MakePixelImage()
{
    SkColorType ct = SkColorType::kRGBA_8888_SkColorType;
    SkAlphaType at = SkAlphaType::kOpaque_SkAlphaType;
    sk_sp<SkColorSpace> cs = SkColorSpace::MakeSRGB();
    auto info = SkImageInfo::Make(image_.textureInfo_.width_, image_.textureInfo_.height_, ct, at, cs);

    SkPixmap imagePixmap(info, reinterpret_cast<const void*>(data_), image_.textureInfo_.width_ * 4);

    return SkImage::MakeFromRaster(imagePixmap, nullptr, nullptr);
}

void TextureLayer::ReadPixel()
{
    AutoRestore scope;
    GraphicsManager::GetInstance().BindOffScreenContext();

    if (fbo_ == 0) {
        CreateReadFbo();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    if (data_ == nullptr) {
        data_ = new uint8_t[image_.textureInfo_.width_ * image_.textureInfo_.height_ * 4];
    }

    glReadPixels(0, 0, image_.textureInfo_.width_, image_.textureInfo_.height_, GL_RGBA, GL_UNSIGNED_BYTE, data_);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TextureLayer::CreateReadFbo()
{
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glBindTexture(GL_TEXTURE_2D, image_.textureInfo_.textureId_);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, image_.textureInfo_.textureId_, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        WIDGET_LOGE("framebuffer not complete, status %d, fbo_ %d, textureId_ %d",
            status, fbo_, image_.textureInfo_.textureId_);
        return;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
#endif

SkRect TextureLayer::onGetBounds()
{
    return SkRect::MakeWH(image_.textureInfo_.width_, image_.textureInfo_.height_);
}

TextureInfo TextureLayer::GetTextureInfo()
{
    return image_.textureInfo_;
}

} // Render3D
} // OHOS
