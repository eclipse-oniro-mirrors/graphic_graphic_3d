/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_input_component.h>
#include <3d/ecs/components/animation_output_component.h>
#include <3d/ecs/components/animation_state_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/dynamic_environment_blender_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/fog_component.h>
#include <3d/ecs/components/graphics_state_component.h>
#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/layer_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/morph_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/physical_camera_component.h>
#include <3d/ecs/components/planar_reflection_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/post_process_configuration_component.h>
#include <3d/ecs/components/reflection_probe_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_batch_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/rsdz_model_id_component.h>
#include <3d/ecs/components/skin_component.h>
#include <3d/ecs/components/skin_ibm_component.h>
#include <3d/ecs/components/skin_joints_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/water_ripple_component.h>
#include <3d/ecs/components/weather_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_morphing_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/ecs/systems/intf_skinning_system.h>
#include <3d/ecs/systems/intf_weather_system.h>
#include <core/log.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/intf_plugin.h>

#include "ecs/components/initial_transform_component.h"
#include "ecs/components/previous_joint_matrices_component.h"
#include "ecs/systems/animation_system.h"
#include "ecs/systems/local_matrix_system.h"
#include "render/datastore/render_data_store_default_camera.h"
#include "render/datastore/render_data_store_default_light.h"
#include "render/datastore/render_data_store_default_material.h"
#include "render/datastore/render_data_store_default_scene.h"
#include "render/datastore/render_data_store_morph.h"
#include "render/datastore/render_data_store_weather.h"
#include "render/node/render_node_camera_single_post_process.h"
#include "render/node/render_node_camera_weather.h"
#include "render/node/render_node_create_default_camera_gpu_images.h"
#include "render/node/render_node_default_camera_controller.h"
#include "render/node/render_node_default_camera_post_process_controller.h"
#include "render/node/render_node_default_cameras.h"
#include "render/node/render_node_default_depth_render_slot.h"
#include "render/node/render_node_default_env.h"
#include "render/node/render_node_default_environment_blender.h"
#include "render/node/render_node_default_lights.h"
#include "render/node/render_node_default_material_deferred_shading.h"
#include "render/node/render_node_default_material_objects.h"
#include "render/node/render_node_default_material_render_slot.h"
#include "render/node/render_node_default_shadow_render_slot.h"
#include "render/node/render_node_default_shadows_blur.h"
#include "render/node/render_node_morph.h"
#include "render/node/render_node_weather_simulation.h"

// Include the declarations directly from engine.
// NOTE: macro defined by cmake as CORE_STATIC_PLUGIN_HEADER="${CORE_ROOT_DIRECTORY}/src/static_plugin_decl.h"
// this is so that the core include directories are not leaked here, but we want this one header in this case.
#ifndef __APPLE__
#include CORE_STATIC_PLUGIN_HEADER
#include "registry_data.cpp"
#endif

#define SYSTEM_FACTORY(type) type##Instance, type##Destroy
#define MANAGER_FACTORY(type) type##Instance, type##Destroy

#define MANAGER(name, type)                                                                               \
    extern IComponentManager* type##Instance(IEcs&);                                                      \
    extern void type##Destroy(IComponentManager*);                                                        \
    namespace {                                                                                           \
    static constexpr auto name = ComponentManagerTypeInfo { { ComponentManagerTypeInfo::UID }, type::UID, \
        CORE_NS::GetName<type>().data(), MANAGER_FACTORY(type) };                                         \
    }

#define SYSTEM(name, type, deps, readOnlyDeps, afterSystem, beforeSystem)                                              \
    extern ISystem* type##Instance(IEcs&);                                                                             \
    extern void type##Destroy(ISystem*);                                                                               \
    namespace {                                                                                                        \
    static constexpr auto name = SystemTypeInfo { { SystemTypeInfo::UID }, type::UID, CORE_NS::GetName<type>().data(), \
        SYSTEM_FACTORY(type), deps, readOnlyDeps, afterSystem, beforeSystem };                                         \
    }

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

// ECS components
MANAGER(CAMERA_COMPONENT_TYPE_INFO, ICameraComponentManager);
MANAGER(PHYSICAL_CAMERA_COMPONENT_TYPE_INFO, IPhysicalCameraComponentManager);
MANAGER(LIGHT_COMPONENT_TYPE_INFO, ILightComponentManager);
MANAGER(LOCAL_MATRIX_COMPONENT_TYPE_INFO, ILocalMatrixComponentManager);
MANAGER(NODE_COMPONENT_TYPE_INFO, INodeComponentManager);
MANAGER(RENDER_MESH_COMPONENT_TYPE_INFO, IRenderMeshComponentManager);
MANAGER(WORLD_MATRIX_COMPONENT_TYPE_INFO, IWorldMatrixComponentManager);
MANAGER(TRANSFORM_COMPONENT_TYPE_INFO, ITransformComponentManager);
MANAGER(INITIAL_TRANSFORM_COMPONENT_TYPE_INFO, IInitialTransformComponentManager);
MANAGER(RENDER_CONFIGURATION_COMPONENT_TYPE_INFO, IRenderConfigurationComponentManager);
MANAGER(SKIN_COMPONENT_TYPE_INFO, ISkinComponentManager);
MANAGER(SKIN_IBM_COMPONENT_TYPE_INFO, ISkinIbmComponentManager);
MANAGER(SKIN_JOINTS_COMPONENT_TYPE_INFO, ISkinJointsComponentManager);
MANAGER(JOINT_MATRICES_COMPONENT_TYPE_INFO, IJointMatricesComponentManager);
MANAGER(MORPH_COMPONENT_TYPE_INFO, IMorphComponentManager);
MANAGER(PLANAR_REFLECTION_COMPONENT_TYPE_INFO, IPlanarReflectionComponentManager);
MANAGER(RSDZ_MODEL_ID_COMPONENT_TYPE_INFO, IRSDZModelIdComponentManager);
MANAGER(MATERIAL_COMPONENT_TYPE_INFO, IMaterialComponentManager);
MANAGER(NAME_COMPONENT_TYPE_INFO, INameComponentManager);
MANAGER(MESH_COMPONENT_TYPE_INFO, IMeshComponentManager);
MANAGER(URI_COMPONENT_TYPE_INFO, IUriComponentManager);
MANAGER(ANIMATION_COMPONENT_TYPE_INFO, IAnimationComponentManager);
MANAGER(ANIMATION_INPUT_COMPONENT_TYPE_INFO, IAnimationInputComponentManager);
MANAGER(ANIMATION_OUTPUT_COMPONENT_TYPE_INFO, IAnimationOutputComponentManager);
MANAGER(ANIMATION_STATE_COMPONENT_TYPE_INFO, IAnimationStateComponentManager);
MANAGER(ANIMATION_TRACK_COMPONENT_TYPE_INFO, IAnimationTrackComponentManager);
MANAGER(ENVIRONMENT_COMPONENT_TYPE_INFO, IEnvironmentComponentManager);
MANAGER(FOG_COMPONENT_TYPE_INFO, IFogComponentManager);
MANAGER(RENDER_HANDLE_COMPONENT_TYPE_INFO, IRenderHandleComponentManager);
MANAGER(POST_PROCESS_COMPONENT_TYPE_INFO, IPostProcessComponentManager);
MANAGER(POST_PROCESS_CONFIGURATION_COMPONENT_TYPE_INFO, IPostProcessConfigurationComponentManager);
MANAGER(LAYER_COMPONENT_TYPE_INFO, ILayerComponentManager);
MANAGER(RENDER_MESH_BATCH_COMPONENT_TYPE_INFO, IRenderMeshBatchComponentManager);
MANAGER(PREV_JOINT_MATRICES_COMPONENT_TYPE_INFO, IPreviousJointMatricesComponentManager);
MANAGER(REFLECTION_PROBE_COMPONENT_TYPE_INFO, IReflectionProbeComponentManager);
MANAGER(DYNAMIC_ENVIRONMENT_BLENDER_COMPONENT_TYPE_INFO, IDynamicEnvironmentBlenderComponentManager);
MANAGER(GRAPHICS_STATE_COMPONENT_TYPE_INFO, IGraphicsStateComponentManager)
MANAGER(WATER_RIPPLE_COMPONENT_TYPE_INFO, IWaterRippleComponentManager)
MANAGER(WEATHER_COMPONENT_TYPE_INFO, IWeatherComponentManager)

namespace {
// Local matrix system dependencies.
constexpr Uid LOCAL_MATRIX_SYSTEM_RW_DEPS[] = { LOCAL_MATRIX_COMPONENT_TYPE_INFO.uid };
constexpr Uid LOCAL_MATRIX_SYSTEM_R_DEPS[] = { TRANSFORM_COMPONENT_TYPE_INFO.uid };

// Node system dependencies.
constexpr Uid NODE_SYSTEM_RW_DEPS[] = { WORLD_MATRIX_COMPONENT_TYPE_INFO.uid, NODE_COMPONENT_TYPE_INFO.uid };
constexpr Uid NODE_SYSTEM_R_DEPS[] = { NAME_COMPONENT_TYPE_INFO.uid, TRANSFORM_COMPONENT_TYPE_INFO.uid,
    LOCAL_MATRIX_COMPONENT_TYPE_INFO.uid, RSDZ_MODEL_ID_COMPONENT_TYPE_INFO.uid };

// Render preprocessor system dependencies.
constexpr Uid RENDER_PREPROCESSOR_SYSTEM_RW_DEPS[] = { RENDER_HANDLE_COMPONENT_TYPE_INFO.uid };
constexpr Uid RENDER_PREPROCESSOR_SYSTEM_R_DEPS[] = {
    JOINT_MATRICES_COMPONENT_TYPE_INFO.uid,
    LAYER_COMPONENT_TYPE_INFO.uid,
    MATERIAL_COMPONENT_TYPE_INFO.uid,
    MESH_COMPONENT_TYPE_INFO.uid,
    NODE_COMPONENT_TYPE_INFO.uid,
    RENDER_MESH_COMPONENT_TYPE_INFO.uid,
    SKIN_COMPONENT_TYPE_INFO.uid,
    WORLD_MATRIX_COMPONENT_TYPE_INFO.uid,
    GRAPHICS_STATE_COMPONENT_TYPE_INFO.uid,
};

// Render system dependencies.
constexpr Uid RENDER_SYSTEM_RW_DEPS[] = {
    PLANAR_REFLECTION_COMPONENT_TYPE_INFO.uid,
    MATERIAL_COMPONENT_TYPE_INFO.uid,
};
constexpr Uid RENDER_SYSTEM_R_DEPS[] = {
    NODE_COMPONENT_TYPE_INFO.uid,
    RENDER_MESH_COMPONENT_TYPE_INFO.uid,
    WORLD_MATRIX_COMPONENT_TYPE_INFO.uid,
    RENDER_CONFIGURATION_COMPONENT_TYPE_INFO.uid,
    CAMERA_COMPONENT_TYPE_INFO.uid,
    LIGHT_COMPONENT_TYPE_INFO.uid,
    MESH_COMPONENT_TYPE_INFO.uid,
    URI_COMPONENT_TYPE_INFO.uid,
    NAME_COMPONENT_TYPE_INFO.uid,
    ENVIRONMENT_COMPONENT_TYPE_INFO.uid,
    FOG_COMPONENT_TYPE_INFO.uid,
    RENDER_HANDLE_COMPONENT_TYPE_INFO.uid,
    JOINT_MATRICES_COMPONENT_TYPE_INFO.uid,
    POST_PROCESS_COMPONENT_TYPE_INFO.uid,
    POST_PROCESS_CONFIGURATION_COMPONENT_TYPE_INFO.uid,
    LAYER_COMPONENT_TYPE_INFO.uid,
    RENDER_MESH_BATCH_COMPONENT_TYPE_INFO.uid,
    PREV_JOINT_MATRICES_COMPONENT_TYPE_INFO.uid,
    REFLECTION_PROBE_COMPONENT_TYPE_INFO.uid,
    DYNAMIC_ENVIRONMENT_BLENDER_COMPONENT_TYPE_INFO.uid,
};

// Animation system dependencies.
constexpr Uid ANIMATION_SYSTEM_RW_DEPS[] = {
    INITIAL_TRANSFORM_COMPONENT_TYPE_INFO.uid,
    ANIMATION_COMPONENT_TYPE_INFO.uid,
    ANIMATION_STATE_COMPONENT_TYPE_INFO.uid,
};
constexpr Uid ANIMATION_SYSTEM_R_DEPS[] = {
    ANIMATION_INPUT_COMPONENT_TYPE_INFO.uid,
    ANIMATION_OUTPUT_COMPONENT_TYPE_INFO.uid,
    ANIMATION_TRACK_COMPONENT_TYPE_INFO.uid,
    NAME_COMPONENT_TYPE_INFO.uid,
    Uid {},
};

// Skinning system dependencies.
constexpr Uid SKINNING_SYSTEM_RW_DEPS[] = {
    JOINT_MATRICES_COMPONENT_TYPE_INFO.uid,
};
constexpr Uid SKINNING_SYSTEM_R_DEPS[] = {
    SKIN_COMPONENT_TYPE_INFO.uid,
    SKIN_IBM_COMPONENT_TYPE_INFO.uid,
    SKIN_JOINTS_COMPONENT_TYPE_INFO.uid,
    WORLD_MATRIX_COMPONENT_TYPE_INFO.uid,
    NODE_COMPONENT_TYPE_INFO.uid,
    RENDER_MESH_COMPONENT_TYPE_INFO.uid,
    MESH_COMPONENT_TYPE_INFO.uid,
    PREV_JOINT_MATRICES_COMPONENT_TYPE_INFO.uid,
};

// Morph system dependencies.
constexpr Uid MORPHING_SYSTEM_R_DEPS[] = {
    NODE_COMPONENT_TYPE_INFO.uid,
    MESH_COMPONENT_TYPE_INFO.uid,
    MORPH_COMPONENT_TYPE_INFO.uid,
    RENDER_MESH_COMPONENT_TYPE_INFO.uid,
    RENDER_HANDLE_COMPONENT_TYPE_INFO.uid,
};

// Weather system dependencies.
constexpr Uid WEATHER_SYSTEM_RW_DEPS[] = {
    RENDER_HANDLE_COMPONENT_TYPE_INFO.uid,
    MESH_COMPONENT_TYPE_INFO.uid,
    RENDER_MESH_COMPONENT_TYPE_INFO.uid,
    MATERIAL_COMPONENT_TYPE_INFO.uid,
    TRANSFORM_COMPONENT_TYPE_INFO.uid,
    RENDER_CONFIGURATION_COMPONENT_TYPE_INFO.uid,
    CAMERA_COMPONENT_TYPE_INFO.uid,
    ENVIRONMENT_COMPONENT_TYPE_INFO.uid,
    WATER_RIPPLE_COMPONENT_TYPE_INFO.uid,
    WEATHER_COMPONENT_TYPE_INFO.uid,
};

constexpr Uid WEATHER_SYSTEM_R_DEPS[] = {
    WATER_RIPPLE_COMPONENT_TYPE_INFO.uid,
    WEATHER_COMPONENT_TYPE_INFO.uid,
    TRANSFORM_COMPONENT_TYPE_INFO.uid,
};

constexpr ComponentManagerTypeInfo CORE_COMPONENT_TYPE_INFOS[] = { CAMERA_COMPONENT_TYPE_INFO,
    INITIAL_TRANSFORM_COMPONENT_TYPE_INFO, PHYSICAL_CAMERA_COMPONENT_TYPE_INFO, LIGHT_COMPONENT_TYPE_INFO,
    LOCAL_MATRIX_COMPONENT_TYPE_INFO, NODE_COMPONENT_TYPE_INFO, WORLD_MATRIX_COMPONENT_TYPE_INFO,
    RENDER_MESH_COMPONENT_TYPE_INFO, TRANSFORM_COMPONENT_TYPE_INFO, RENDER_CONFIGURATION_COMPONENT_TYPE_INFO,
    SKIN_COMPONENT_TYPE_INFO, SKIN_JOINTS_COMPONENT_TYPE_INFO, JOINT_MATRICES_COMPONENT_TYPE_INFO,
    MORPH_COMPONENT_TYPE_INFO, PLANAR_REFLECTION_COMPONENT_TYPE_INFO, RSDZ_MODEL_ID_COMPONENT_TYPE_INFO,
    MATERIAL_COMPONENT_TYPE_INFO, NAME_COMPONENT_TYPE_INFO, MESH_COMPONENT_TYPE_INFO, URI_COMPONENT_TYPE_INFO,
    SKIN_IBM_COMPONENT_TYPE_INFO, ANIMATION_COMPONENT_TYPE_INFO, ANIMATION_INPUT_COMPONENT_TYPE_INFO,
    ANIMATION_OUTPUT_COMPONENT_TYPE_INFO, ANIMATION_STATE_COMPONENT_TYPE_INFO, ANIMATION_TRACK_COMPONENT_TYPE_INFO,
    ENVIRONMENT_COMPONENT_TYPE_INFO, FOG_COMPONENT_TYPE_INFO, RENDER_HANDLE_COMPONENT_TYPE_INFO,
    POST_PROCESS_COMPONENT_TYPE_INFO, POST_PROCESS_CONFIGURATION_COMPONENT_TYPE_INFO, LAYER_COMPONENT_TYPE_INFO,
    RENDER_MESH_BATCH_COMPONENT_TYPE_INFO, PREV_JOINT_MATRICES_COMPONENT_TYPE_INFO,
    REFLECTION_PROBE_COMPONENT_TYPE_INFO, DYNAMIC_ENVIRONMENT_BLENDER_COMPONENT_TYPE_INFO,
    GRAPHICS_STATE_COMPONENT_TYPE_INFO, WATER_RIPPLE_COMPONENT_TYPE_INFO, WEATHER_COMPONENT_TYPE_INFO };
} // namespace

SYSTEM(ANIMATION_SYSTEM_TYPE_INFO, IAnimationSystem, ANIMATION_SYSTEM_RW_DEPS, ANIMATION_SYSTEM_R_DEPS, {},
    LocalMatrixSystem::UID);
SYSTEM(LOCAL_MATRIX_SYSTEM_TYPE_INFO, LocalMatrixSystem, LOCAL_MATRIX_SYSTEM_RW_DEPS, LOCAL_MATRIX_SYSTEM_R_DEPS,
    IAnimationSystem::UID, INodeSystem::UID);
SYSTEM(NODE_SYSTEM_TYPE_INFO, INodeSystem, NODE_SYSTEM_RW_DEPS, NODE_SYSTEM_R_DEPS, LocalMatrixSystem::UID,
    IRenderPreprocessorSystem::UID);
SYSTEM(SKINNING_SYSTEM_TYPE_INFO, ISkinningSystem, SKINNING_SYSTEM_RW_DEPS, SKINNING_SYSTEM_R_DEPS, INodeSystem::UID,
    IRenderPreprocessorSystem::UID);
SYSTEM(RENDER_PREPROCESSOR_SYSTEM_TYPE_INFO, IRenderPreprocessorSystem, RENDER_PREPROCESSOR_SYSTEM_RW_DEPS,
    RENDER_PREPROCESSOR_SYSTEM_R_DEPS, INodeSystem::UID, {});
SYSTEM(MORPHING_SYSTEM_TYPE_INFO, IMorphingSystem, {}, MORPHING_SYSTEM_R_DEPS, INodeSystem::UID, IRenderSystem::UID);
SYSTEM(WEATHER_SYSTEM_TYPE_INFO, IWeatherSystem, WEATHER_SYSTEM_RW_DEPS, WEATHER_SYSTEM_R_DEPS, INodeSystem::UID,
    IRenderSystem::UID);
SYSTEM(RENDER_SYSTEM_TYPE_INFO, IRenderSystem, RENDER_SYSTEM_RW_DEPS, RENDER_SYSTEM_R_DEPS,
    IRenderPreprocessorSystem::UID, {});

namespace {
constexpr SystemTypeInfo CORE_SYSTEM_TYPE_INFOS[] = {
    LOCAL_MATRIX_SYSTEM_TYPE_INFO,
    NODE_SYSTEM_TYPE_INFO,
    RENDER_PREPROCESSOR_SYSTEM_TYPE_INFO,
    RENDER_SYSTEM_TYPE_INFO,
    ANIMATION_SYSTEM_TYPE_INFO,
    SKINNING_SYSTEM_TYPE_INFO,
    MORPHING_SYSTEM_TYPE_INFO,
    WEATHER_SYSTEM_TYPE_INFO,
};

template<typename RenderType>
constexpr auto FillRenderDataStoreTypeInfo()
{
    return RenderDataStoreTypeInfo { { RenderDataStoreTypeInfo::UID }, RenderType::UID, RenderType::TYPE_NAME,
        RenderType::Create };
}

template<typename RenderType>
constexpr auto FillRenderNodeTypeInfo()
{
    return RenderNodeTypeInfo { { RenderNodeTypeInfo::UID }, RenderType::UID, RenderType::TYPE_NAME, RenderType::Create,
        RenderType::Destroy, IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT,
        IRenderNode::ClassType::CLASS_TYPE_NODE, {}, {} };
}

constexpr RenderDataStoreTypeInfo CORE_RENDER_DATA_STORE_INFOS[] = {
    FillRenderDataStoreTypeInfo<RenderDataStoreDefaultCamera>(),
    FillRenderDataStoreTypeInfo<RenderDataStoreDefaultLight>(),
    FillRenderDataStoreTypeInfo<RenderDataStoreDefaultMaterial>(),
    FillRenderDataStoreTypeInfo<RenderDataStoreDefaultScene>(),
    FillRenderDataStoreTypeInfo<RenderDataStoreMorph>(),
    FillRenderDataStoreTypeInfo<RenderDataStoreWeather>(),
};

constexpr RenderNodeTypeInfo CORE_RENDER_NODE_TYPE_INFOS[] = {
    FillRenderNodeTypeInfo<RenderNodeCreateDefaultCameraGpuImages>(),
    FillRenderNodeTypeInfo<RenderNodeDefaultCameraController>(),
    FillRenderNodeTypeInfo<RenderNodeDefaultCameraPostProcessController>(),
    FillRenderNodeTypeInfo<RenderNodeDefaultCameras>(),
    FillRenderNodeTypeInfo<RenderNodeDefaultDepthRenderSlot>(),
    FillRenderNodeTypeInfo<RenderNodeDefaultEnv>(),
    FillRenderNodeTypeInfo<RenderNodeDefaultLights>(),
    FillRenderNodeTypeInfo<RenderNodeDefaultMaterialDeferredShading>(),
    FillRenderNodeTypeInfo<RenderNodeDefaultMaterialObjects>(),
    FillRenderNodeTypeInfo<RenderNodeDefaultMaterialRenderSlot>(),
    FillRenderNodeTypeInfo<RenderNodeDefaultShadowRenderSlot>(),
    FillRenderNodeTypeInfo<RenderNodeDefaultShadowsBlur>(),
    FillRenderNodeTypeInfo<RenderNodeMorph>(),
    FillRenderNodeTypeInfo<RenderNodeCameraSinglePostProcess>(),
    FillRenderNodeTypeInfo<RenderNodeDefaultEnvironmentBlender>(),
    FillRenderNodeTypeInfo<RenderNodeCameraWeather>(),
    FillRenderNodeTypeInfo<RenderNodeWeatherSimulation>(),
};
} // namespace

void RegisterTypes(IPluginRegister& pluginRegistry)
{
    for (const auto& info : CORE_RENDER_DATA_STORE_INFOS) {
        pluginRegistry.RegisterTypeInfo(info);
    }
    for (const auto& info : CORE_RENDER_NODE_TYPE_INFOS) {
        pluginRegistry.RegisterTypeInfo(info);
    }
    for (const auto& info : CORE_COMPONENT_TYPE_INFOS) {
        pluginRegistry.RegisterTypeInfo(info);
    }
    for (const auto& info : CORE_SYSTEM_TYPE_INFOS) {
        pluginRegistry.RegisterTypeInfo(info);
    }
}

void UnregisterTypes(IPluginRegister& pluginRegistry)
{
    for (const auto& info : CORE_SYSTEM_TYPE_INFOS) {
        pluginRegistry.UnregisterTypeInfo(info);
    }
    for (const auto& info : CORE_COMPONENT_TYPE_INFOS) {
        pluginRegistry.UnregisterTypeInfo(info);
    }
    for (const auto& info : CORE_RENDER_NODE_TYPE_INFOS) {
        pluginRegistry.UnregisterTypeInfo(info);
    }
    for (const auto& info : CORE_RENDER_DATA_STORE_INFOS) {
        pluginRegistry.UnregisterTypeInfo(info);
    }
}
CORE3D_END_NAMESPACE()
