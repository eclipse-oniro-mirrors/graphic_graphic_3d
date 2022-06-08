/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include <3d/ecs/components/camera_component.h>
#include <core/property/property_types.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"
#include "ecs/components/layer_flag_bits_metadata.h"

#define IMPLEMENT_MANAGER
#include "PropertyTools/property_macros.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;
using BASE_NS::Format;
using BASE_NS::vector;

using CORE3D_NS::CameraComponent;

// Extend propertysystem with the enums
DECLARE_PROPERTY_TYPE(CameraComponent::Projection);
DECLARE_PROPERTY_TYPE(CameraComponent::RenderingPipeline);
DECLARE_PROPERTY_TYPE(CameraComponent::Culling);
DECLARE_PROPERTY_TYPE(CameraComponent::TargetUsage);
DECLARE_PROPERTY_TYPE(vector<CameraComponent::TargetUsage>);
DECLARE_PROPERTY_TYPE(Format);

// Declare their metadata
BEGIN_ENUM(CameraTypeMetaData, CameraComponent::Projection)
DECL_ENUM(CameraComponent::Projection, ORTHOGRAPHIC, "Orthographic")
DECL_ENUM(CameraComponent::Projection, PERSPECTIVE, "Perspective")
DECL_ENUM(CameraComponent::Projection, CUSTOM, "Custom Projection")
END_ENUM(CameraTypeMetaData, CameraComponent::Projection)

BEGIN_ENUM(CameraRenderPipelineTypeMetaData, CameraComponent::RenderingPipeline)
DECL_ENUM(CameraComponent::RenderingPipeline, LIGHT_FORWARD, "Light-Weight Forward")
DECL_ENUM(CameraComponent::RenderingPipeline, FORWARD, "Forward")
DECL_ENUM(CameraComponent::RenderingPipeline, DEFERRED, "Deferred")
DECL_ENUM(CameraComponent::RenderingPipeline, CUSTOM, "Custom")
END_ENUM(CameraRenderPipelineTypeMetaData, CameraComponent::RenderingPipeline)

BEGIN_ENUM(CameraCullTypeMetaData, CameraComponent::Culling)
DECL_ENUM(CameraComponent::Culling, NONE, "None")
DECL_ENUM(CameraComponent::Culling, VIEW_FRUSTUM, "View Frustum")
END_ENUM(CameraCullTypeMetaData, CameraComponent::Culling)

BEGIN_ENUM(CameraSceneFlagBitsMetaData, CameraComponent::SceneFlagBits)
DECL_ENUM(CameraComponent::SceneFlagBits, ACTIVE_RENDER_BIT, "Active Render")
DECL_ENUM(CameraComponent::SceneFlagBits, MAIN_CAMERA_BIT, "Main Camera")
END_ENUM(CameraSceneFlagBitsMetaData, CameraComponent::SceneFlagBits)

BEGIN_ENUM(CameraPipelineFlagBitsMetaData, CameraComponent::PipelineFlagBits)
DECL_ENUM(CameraComponent::PipelineFlagBits, CLEAR_DEPTH_BIT, "Clear Depth")
DECL_ENUM(CameraComponent::PipelineFlagBits, CLEAR_COLOR_BIT, "Clear Color")
DECL_ENUM(CameraComponent::PipelineFlagBits, MSAA_BIT, "MSAA")
DECL_ENUM(CameraComponent::PipelineFlagBits, ALLOW_COLOR_PRE_PASS_BIT, "Allow Color Pre-Pass")
DECL_ENUM(CameraComponent::PipelineFlagBits, FORCE_COLOR_PRE_PASS_BIT, "Force Color Pre-Pass")
DECL_ENUM(CameraComponent::PipelineFlagBits, HISTORY_BIT, "Create and store history image")
DECL_ENUM(CameraComponent::PipelineFlagBits, JITTER_BIT, "Jitter camera, offset within pixel")
DECL_ENUM(CameraComponent::PipelineFlagBits, VELOCITY_OUTPUT_BIT, "Output samplable velocity")
DECL_ENUM(CameraComponent::PipelineFlagBits, DEPTH_OUTPUT_BIT, "Output samplable depth")
END_ENUM(CameraPipelineFlagBitsMetaData, CameraComponent::PipelineFlagBits)

// define some meaningful image formats for camera component usage
BEGIN_ENUM(FormatMetaData, Format)
DECL_ENUM(Format, BASE_FORMAT_UNDEFINED, "BASE_FORMAT_UNDEFINED")
DECL_ENUM(Format, BASE_FORMAT_R8G8B8A8_UNORM, "BASE_FORMAT_R8G8B8A8_UNORM")
DECL_ENUM(Format, BASE_FORMAT_R8G8B8A8_SRGB, "BASE_FORMAT_R8G8B8A8_SRGB")
DECL_ENUM(Format, BASE_FORMAT_A2R10G10B10_UNORM_PACK32, "BASE_FORMAT_A2R10G10B10_UNORM_PACK32")
DECL_ENUM(Format, BASE_FORMAT_R16G16B16A16_UNORM, "BASE_FORMAT_R16G16B16A16_UNORM")
DECL_ENUM(Format, BASE_FORMAT_R16G16B16A16_SFLOAT, "BASE_FORMAT_R16G16B16A16_SFLOAT")
DECL_ENUM(Format, BASE_FORMAT_B10G11R11_UFLOAT_PACK32, "BASE_FORMAT_B10G11R11_UFLOAT_PACK32")
DECL_ENUM(Format, BASE_FORMAT_D16_UNORM, "BASE_FORMAT_D16_UNORM")
DECL_ENUM(Format, BASE_FORMAT_D32_SFLOAT, "BASE_FORMAT_D32_SFLOAT")
DECL_ENUM(Format, BASE_FORMAT_D24_UNORM_S8_UINT, "BASE_FORMAT_D24_UNORM_S8_UINT")
END_ENUM(FormatMetaData, Format)

BEGIN_METADATA(CameraImageFormatsMetaData, CameraComponent::TargetUsage)
DECL_PROPERTY2(CameraComponent::TargetUsage, format, "", 0)
DECL_PROPERTY2(CameraComponent::TargetUsage, usageFlags, "", 0)
END_METADATA(CameraImageFormatsMetaData, CameraComponent::TargetUsage)
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;
using CORE_NS::PropertyFlags;

class CameraComponentManager final : public BaseManager<CameraComponent, ICameraComponentManager> {
    BEGIN_PROPERTY(CameraComponent, ComponentMetadata)
#include <3d/ecs/components/camera_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit CameraComponentManager(IEcs& ecs)
        : BaseManager<CameraComponent, ICameraComponentManager>(ecs, CORE_NS::GetName<CameraComponent>())
    {}

    ~CameraComponentManager() = default;

    size_t PropertyCount() const override
    {
        return componentMetaData_.size();
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < componentMetaData_.size()) {
            return &componentMetaData_[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return componentMetaData_;
    }
};

IComponentManager* ICameraComponentManagerInstance(IEcs& ecs)
{
    return new CameraComponentManager(ecs);
}

void ICameraComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<CameraComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
