/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "widget_adapter.h"

#include <window.h>

#include "3d_widget_adapter_log.h"
#include "graphics_manager.h"
#include "widget_trace.h"

namespace OHOS::Render3D {
#ifdef CHECK_NULL_PTR
#undef CHECK_NULL_PTR
#endif
#define CHECK_NULL_PTR(ptr) \
do { \
    if (!(ptr)) { \
        WIDGET_LOGE("%s engine not ready", __func__); \
        return false; \
    } \
} while (false)

WidgetAdapter::WidgetAdapter(uint32_t key) : key_(key)
{
    WIDGET_LOGD("WidgetAdapter create key %d", key);
}

WidgetAdapter::~WidgetAdapter()
{
    WIDGET_LOGD("WidgetAdapter destroy key %d", key_);
}

bool WidgetAdapter::Initialize(std::unique_ptr<IEngine> engine)
{
    WIDGET_SCOPED_TRACE("WidgetAdapter::Initialze");
    engine_ = std::move(engine);
    CHECK_NULL_PTR(engine_);
    engine_->InitializeScene(key_);
    return true;
}

bool WidgetAdapter::SetupCameraTransform(
    const OHOS::Render3D::Position& position, const OHOS::Render3D::Vec3& lookAt,
    const OHOS::Render3D::Vec3& up, const OHOS::Render3D::Quaternion& rotation)
{
    CHECK_NULL_PTR(engine_);
    engine_->SetupCameraTransform(position, lookAt, up, rotation);
    return true;
}

bool WidgetAdapter::SetupCameraViewProjection(float zNear, float zFar, float fovDegrees)
{
    CHECK_NULL_PTR(engine_);
    engine_->SetupCameraViewProjection(zNear, zFar, fovDegrees);
    return true;
}

bool WidgetAdapter::UpdateLights(const std::vector<std::shared_ptr<OHOS::Render3D::Light>>& lights)
{
    CHECK_NULL_PTR(engine_);
    engine_->UpdateLights(lights);
    return true;
}

bool WidgetAdapter::OnTouchEvent(const PointerEvent& event)
{
    CHECK_NULL_PTR(engine_);
    engine_->OnTouchEvent(event);
    return true;
}

bool WidgetAdapter::DrawFrame()
{
    WIDGET_SCOPED_TRACE("WidgetAdpater::DrawFrame");
    CHECK_NULL_PTR(engine_);
#if MULTI_ECS_UPDATE_AT_ONCE
    engine_->DeferDraw();
#else
    engine_->DrawFrame();
#endif
    return true;
}

bool WidgetAdapter::UpdateGeometries(const std::vector<std::shared_ptr<Geometry>>& shapes)
{
    CHECK_NULL_PTR(engine_);
    engine_->UpdateGeometries(shapes);
    return true;
}

bool WidgetAdapter::UpdateGLTFAnimations(const std::vector<std::shared_ptr<GLTFAnimation>>&
    animations)
{
    CHECK_NULL_PTR(engine_);
    engine_->UpdateGLTFAnimations(animations);
    return true;
}


bool WidgetAdapter::UpdateCustomRender(const std::shared_ptr<CustomRenderDescriptor>&
    customRenders)
{
    CHECK_NULL_PTR(engine_);
    engine_->UpdateCustomRender(customRenders);
    return true;
}

bool WidgetAdapter::UpdateShaderPath(const std::string& shaderPath)
{
    CHECK_NULL_PTR(engine_);
    engine_->UpdateShaderPath(shaderPath);
    return true;
}

bool WidgetAdapter::UpdateImageTexturePaths(const std::vector<std::string>& imageTextures)
{
    CHECK_NULL_PTR(engine_);
    engine_->UpdateImageTexturePaths(imageTextures);
    return true;
}

bool WidgetAdapter::UpdateShaderInputBuffer(const std::shared_ptr<OHOS::Render3D::ShaderInputBuffer>&
    shaderInputBuffer)
{
    CHECK_NULL_PTR(engine_);
    engine_->UpdateShaderInputBuffer(shaderInputBuffer);
    return true;
}

void WidgetAdapter::DeInitEngine()
{
    WIDGET_SCOPED_TRACE("WidgetAdapter::DeInitEngine");
    if (engine_ == nullptr) {
        return;
    }
    engine_->DeInitEngine();
    engine_.reset();
}

bool WidgetAdapter::SetupCameraViewport(uint32_t width, uint32_t height)
{
    CHECK_NULL_PTR(engine_);
    engine_->SetupCameraViewPort(width, height);
    return true;
}

bool WidgetAdapter::OnWindowChange(const TextureInfo& textureInfo)
{
    CHECK_NULL_PTR(engine_);
    engine_->OnWindowChange(textureInfo);
    return true;
}

bool WidgetAdapter::LoadSceneModel(const std::string& scene)
{
    CHECK_NULL_PTR(engine_);
    engine_->LoadSceneModel(scene);
    return true;
}

bool WidgetAdapter::LoadEnvModel(const std::string& enviroment, BackgroundType type)
{
    CHECK_NULL_PTR(engine_);
    engine_->LoadEnvModel(enviroment, type);
    return true;
}

bool WidgetAdapter::UnloadSceneModel()
{
    CHECK_NULL_PTR(engine_);
    engine_->UnloadSceneModel();
    return true;
}

bool WidgetAdapter::UnloadEnvModel()
{
    CHECK_NULL_PTR(engine_);
    engine_->UnloadSceneModel();
    return true;
}

bool WidgetAdapter::NeedsRepaint()
{
    CHECK_NULL_PTR(engine_);
    return engine_->NeedsRepaint();
}
} // namespace OHOS::Render3D
