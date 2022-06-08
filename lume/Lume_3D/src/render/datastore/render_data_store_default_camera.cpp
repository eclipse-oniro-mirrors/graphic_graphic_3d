/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "render_data_store_default_camera.h"

#include <cinttypes>
#include <cstddef>

#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;

RenderDataStoreDefaultCamera::RenderDataStoreDefaultCamera(const string_view name) : name_(name) {}

void RenderDataStoreDefaultCamera::PostRender()
{
    Clear();
}

void RenderDataStoreDefaultCamera::Clear()
{
    cameras_.clear();
    nextId = 0;
}

void RenderDataStoreDefaultCamera::AddCamera(const RenderCamera& camera)
{
    if (const uint32_t arrIdx = static_cast<uint32_t>(cameras_.size());
        arrIdx < DefaultMaterialCameraConstants::MAX_CAMERA_COUNT) {
#if (CORE3D_VALIDATION_ENABLED == 1)
        if ((camera.id != 0xFFFFFFFFffff) || (!camera.name.empty())) {
            for (const auto& cam : cameras_) {
                if ((camera.id == cam.id) || ((!camera.name.empty()) && (camera.name == cam.name))) {
                    CORE_LOG_ONCE_W(to_string(camera.id) + camera.name,
                        "CORE_VALIDATION: non unique camera id: %" PRIu64 " or name: %s", camera.id,
                        camera.name.c_str());
                }
            }
        }
#endif
        cameras_.emplace_back(camera);
    } else {
#if (CORE3D_VALIDATION_ENABLED == 1)
        CORE_LOG_ONCE_W("drop_camera_count_full", "CORE_VALIDATION: camera dropped (max count: %u)",
            DefaultMaterialCameraConstants::MAX_CAMERA_COUNT);
#endif
    }
}

array_view<const RenderCamera> RenderDataStoreDefaultCamera::GetCameras() const
{
    return array_view<const RenderCamera>(cameras_.data(), cameras_.size());
}

RenderCamera RenderDataStoreDefaultCamera::GetCamera(const string_view name) const
{
    for (const auto& cam : cameras_) {
        if (cam.name == name) {
            return cam;
        }
    }
    return {};
}

RenderCamera RenderDataStoreDefaultCamera::GetCamera(const uint64_t id) const
{
    for (const auto& cam : cameras_) {
        if (cam.id == id) {
            return cam;
        }
    }
    return {};
}

uint32_t RenderDataStoreDefaultCamera::GetCameraIndex(const string_view name) const
{
    for (uint32_t idx = 0; idx < static_cast<uint32_t>(cameras_.size()); ++idx) {
        if (cameras_[idx].name == name) {
            return idx;
        }
    }
    return ~0u;
}

uint32_t RenderDataStoreDefaultCamera::GetCameraIndex(const uint64_t id) const
{
    for (uint32_t idx = 0; idx < static_cast<uint32_t>(cameras_.size()); ++idx) {
        if (cameras_[idx].id == id) {
            return idx;
        }
    }
    return ~0u;
}

uint32_t RenderDataStoreDefaultCamera::GetCameraCount() const
{
    return static_cast<uint32_t>(cameras_.size());
}

RENDER_NS::IRenderDataStore* RenderDataStoreDefaultCamera::Create(RENDER_NS::IRenderContext&, char const* name)
{
    // device not used
    return new RenderDataStoreDefaultCamera(name);
}

void RenderDataStoreDefaultCamera::Destroy(IRenderDataStore* instance)
{
    delete static_cast<RenderDataStoreDefaultCamera*>(instance);
}
CORE3D_END_NAMESPACE()
