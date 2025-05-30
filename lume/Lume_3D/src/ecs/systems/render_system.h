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

#ifndef CORE_ECS_RENDERSYSTEM_H
#define CORE_ECS_RENDERSYSTEM_H

#include <ComponentTools/component_query.h>
#include <limits>

#include <3d/ecs/components/layer_defines.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/containers/unordered_map.h>
#include <base/math/vector.h>
#include <core/namespace.h>
#include <core/property_tools/property_api_impl.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "property/property_handle.h"

BASE_BEGIN_NAMESPACE()
namespace Math {
class Mat4X4;
} // namespace Math
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
class IFrustumUtil;
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
class IShaderManager;
class IRenderContext;
struct PostProcessConfiguration;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IDynamicEnvironmentBlenderComponentManager;
class IEnvironmentComponentManager;
class IFogComponentManager;
class IRenderHandleComponentManager;
class INameComponentManager;
class INodeComponentManager;
class IRenderMeshComponentManager;
class IWorldMatrixComponentManager;
class IRenderConfigurationComponentManager;
class ICameraComponentManager;
class ILayerComponentManager;
class ILightComponentManager;
class IJointMatricesComponentManager;
class IMaterialComponentManager;
class IMeshComponentManager;
class ISkinJointsComponentManager;
class IPlanarReflectionComponentManager;
class IPreviousJointMatricesComponentManager;
class IPostProcessComponentManager;
class IPostProcessConfigurationComponentManager;
class IUriComponentManager;
class IRenderDataStoreDefaultCamera;
class IRenderDataStoreDefaultLight;
class IRenderDataStoreDefaultMaterial;
class IRenderDataStoreDefaultScene;

class IRenderPreprocessorSystem;

class IMesh;
class IMaterial;
class IPicking;
class IRenderUtil;
class IGraphicsContext;

struct RenderMeshComponent;
struct JointMatricesComponent;
struct PlanarReflectionComponent;
struct PreviousJointMatricesComponent;
struct MaterialComponent;
struct WorldMatrixComponent;
struct LightComponent;
struct MinAndMax;

class RenderSystem final : public IRenderSystem {
public:
    explicit RenderSystem(CORE_NS::IEcs& ecs);
    ~RenderSystem() override;
    BASE_NS::string_view GetName() const override;
    BASE_NS::Uid GetUid() const override;
    CORE_NS::IPropertyHandle* GetProperties() override;
    const CORE_NS::IPropertyHandle* GetProperties() const override;
    void SetProperties(const CORE_NS::IPropertyHandle&) override;
    bool IsActive() const override;
    void SetActive(bool state) override;

    void Initialize() override;
    bool Update(bool frameRenderingQueued, uint64_t totalTime, uint64_t deltaTime) override;
    void Uninitialize() override;

    const CORE_NS::IEcs& GetECS() const override;

    BASE_NS::array_view<const RENDER_NS::RenderHandleReference> GetRenderNodeGraphs() const override;

    struct BatchData {
        CORE_NS::Entity entity; // node, render mesh component
        CORE_NS::Entity mesh;   // mesh component
        uint64_t layerMask { LayerConstants::DEFAULT_LAYER_MASK };
        uint64_t sceneId { 0U };
        CORE_NS::IComponentManager::ComponentId jointId { CORE_NS::IComponentManager::INVALID_COMPONENT_ID };
        CORE_NS::IComponentManager::ComponentId prevJointId { CORE_NS::IComponentManager::INVALID_COMPONENT_ID };

        BASE_NS::Math::Mat4X4 mtx;       // world matrix
        BASE_NS::Math::Mat4X4 prevWorld; // previous world matrix
    };
    using BatchDataVector = BASE_NS::vector<BatchData>;

    struct DefaultMaterialShaderData {
        struct SingleShaderData {
            CORE_NS::EntityReference shader;
            CORE_NS::EntityReference gfxState;
            CORE_NS::EntityReference gfxStateDoubleSided;
        };
        SingleShaderData opaque;
        SingleShaderData blend;
        SingleShaderData depth;
    };
    struct CameraRenderNodeGraphs {
        // camera
        RENDER_NS::RenderHandleReference rngHandle;
        // camera post process
        RENDER_NS::RenderHandleReference ppRngHandle;
    };
    struct CameraRngsOutput {
        CameraRenderNodeGraphs rngs;
        // multi-view post processes
        RENDER_NS::RenderHandleReference
            multiviewPpHandles[RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT] { {}, {}, {} };
    };

private:
    struct LightProcessData {
        const uint64_t layerMask { 0 };
        uint32_t sceneId { 0U };
        const CORE_NS::Entity& entity;
        const LightComponent& lightComponent;
        const BASE_NS::Math::Mat4X4& world;
        RenderScene& renderScene;
        uint32_t& spotLightIndex;
    };
    struct SkinProcessData {
        const JointMatricesComponent* const jointMatricesComponent { nullptr };
        const PreviousJointMatricesComponent* const prevJointMatricesComponent { nullptr };
    };

    void SetDataStorePointers(RENDER_NS::IRenderDataStoreManager& manager);
    // returns the instance's valid scene component
    RenderConfigurationComponent GetRenderConfigurationComponent();
    CORE_NS::Entity ProcessScene(const RenderConfigurationComponent& sc);
    void ProcessMesh(const RenderMeshData rmd, const SkinProcessData& spd);
    void ProcessMesh(
        BASE_NS::array_view<const RenderMeshData> rmd, const RenderMeshBatchData rmbd, const SkinProcessData& spd);
    void ProcessRenderMeshComponentBatch(
        uint32_t sceneId, const CORE_NS::Entity renderMeshBatch, const CORE_NS::ComponentQuery::ResultRow* row);
    void ProcessRenderMeshAutomaticBatch(
        uint32_t sceneId, BASE_NS::array_view<const CORE_NS::Entity> renderMeshComponents);
    void ProcessSingleRenderMesh(uint32_t sceneId, CORE_NS::Entity renderMeshComponent);
    void ProcessRenderables();
    void ProcessRenderMeshBatchComponentRenderables();
    void ProcessEnvironments(const RenderConfigurationComponent& sceneComponent);
    void ProcessCameras(const RenderConfigurationComponent& sceneComponent, const CORE_NS::Entity& mainCameraEntity,
        RenderScene& renderScene);
    void ProcessLight(const LightProcessData& lightProcessData);
    void ProcessLights(RenderScene& renderScene);
    void ProcessShadowCamera(const LightProcessData lightProcessData, RenderLight& light);
    void ProcessReflection(const CORE_NS::ComponentQuery::ResultRow& row,
        const PlanarReflectionComponent& reflComponent, const RenderCamera& camera, BASE_NS::Math::UVec2 targetRes);
    void ProcessReflections(const RenderScene& renderScene);
    void ProcessPostProcesses();
    void RecalculatePostProcesses(BASE_NS::string_view name, RENDER_NS::PostProcessConfiguration& ppConfig);
    void FetchFullScene();
    void EvaluateFrameObjectFlags();
    // calculates min max from all submeshes and does min max for inout
    struct BatchIndices {
        uint32_t submeshIndex { ~0u };
        uint32_t batchStartIndex { ~0u };
        uint32_t batchEndIndex { ~0u };
    };
    // with submeshIndex == ~0u processes all submeshes
    void CombineBatchWorldMinAndMax(const BatchDataVector& batchData, const BatchIndices& batchIndices,
        const MeshComponent& mesh, MinAndMax& mam) const;

    void ProcessRenderNodeGraphs(const RenderConfigurationComponent& renderConfig, const RenderScene& renderScene);
    void DestroyRenderDataStores();
    CameraRngsOutput GetCameraRenderNodeGraphs(const RenderScene& renderScene, const RenderCamera& renderCamera);
    RENDER_NS::RenderHandleReference GetSceneRenderNodeGraph(const RenderScene& renderScene);

    struct CameraData;
    CameraData UpdateAndGetPreviousFrameCameraData(
        const CORE_NS::Entity& entity, const BASE_NS::Math::Mat4X4& view, const BASE_NS::Math::Mat4X4& proj);
    BASE_NS::vector<RenderCamera> GetMultiviewCameras(const RenderCamera& renderCamera);

    bool active_ = true;
    CORE_NS::IEcs& ecs_;

    IRenderSystem::Properties properties_;

    BASE_NS::refcnt_ptr<IRenderDataStoreDefaultCamera> dsCamera_;
    BASE_NS::refcnt_ptr<IRenderDataStoreDefaultLight> dsLight_;
    BASE_NS::refcnt_ptr<IRenderDataStoreDefaultMaterial> dsMaterial_;
    BASE_NS::refcnt_ptr<IRenderDataStoreDefaultScene> dsScene_;
    RENDER_NS::IShaderManager* shaderMgr_ = nullptr;
    RENDER_NS::IGpuResourceManager* gpuResourceMgr_ = nullptr;
    CORE_NS::IFrustumUtil* frustumUtil_ = nullptr;

    INodeComponentManager* nodeMgr_ = nullptr;
    IRenderMeshComponentManager* renderMeshMgr_ = nullptr;
    IWorldMatrixComponentManager* worldMatrixMgr_ = nullptr;
    IRenderConfigurationComponentManager* renderConfigMgr_ = nullptr;
    ICameraComponentManager* cameraMgr_ = nullptr;
    ILightComponentManager* lightMgr_ = nullptr;
    IPlanarReflectionComponentManager* planarReflectionMgr_ = nullptr;

    IMaterialComponentManager* materialMgr_ = nullptr;
    IMeshComponentManager* meshMgr_ = nullptr;
    IUriComponentManager* uriMgr_ = nullptr;
    INameComponentManager* nameMgr_ = nullptr;
    IEnvironmentComponentManager* environmentMgr_ = nullptr;
    IFogComponentManager* fogMgr_ = nullptr;
    IRenderHandleComponentManager* gpuHandleMgr_ = nullptr;
    ILayerComponentManager* layerMgr_ = nullptr;
    IDynamicEnvironmentBlenderComponentManager* dynamicEnvBlendMgr_ = nullptr;

    IJointMatricesComponentManager* jointMatricesMgr_ = nullptr;
    IPreviousJointMatricesComponentManager* prevJointMatricesMgr_ = nullptr;

    IPostProcessComponentManager* postProcessMgr_ = nullptr;
    IPostProcessConfigurationComponentManager* postProcessConfigMgr_ = nullptr;

    uint32_t renderConfigurationGeneration_ = 0;
    uint32_t cameraGeneration_ = 0;
    uint32_t lightGeneration_ = 0;
    uint32_t planarReflectionGeneration_ = 0;
    uint32_t environmentGeneration_ = 0;
    uint32_t fogGeneration_ = 0;
    uint32_t postprocessGeneration_ = 0;
    uint32_t postprocessConfigurationGeneration_ = 0;

    IPicking* picking_ = nullptr;

    IGraphicsContext* graphicsContext_ = nullptr;
    IRenderUtil* renderUtil_ = nullptr;
    RENDER_NS::IRenderContext* renderContext_ = nullptr;

    IRenderPreprocessorSystem* renderPreprocessorSystem_ = nullptr;

    CORE_NS::ComponentQuery lightQuery_;
    CORE_NS::ComponentQuery renderableQuery_;
    CORE_NS::ComponentQuery reflectionsQuery_;
    uint32_t reflectionMaxMipBlur_ { 0U };

    BASE_NS::Math::Vec3 sceneBoundingSpherePosition_ { 0.0f, 0.0f, 0.0f };
    float sceneBoundingSphereRadius_ { 0.0f };

    CORE_NS::PropertyApiImpl<IRenderSystem::Properties> RENDER_SYSTEM_PROPERTIES;

    uint64_t totalTime_ { 0u };
    uint64_t deltaTime_ { 0u };
    uint64_t frameIndex_ { 0u };

    // additionally these could be stored to somewhere else
    // though this is ECS render system and the render node graphs are owned by ECS (RS)
    // these do not add overhead if the property bits are not set (only clear per frame)
    struct RenderProcessing {
        struct AdditionalCameraContainer {
            CameraRenderNodeGraphs rngs;
            RenderCamera::Flags flags { 0 };
            RenderCamera::RenderPipelineType renderPipelineType { RenderCamera::RenderPipelineType::FORWARD };
            uint64_t lastFrameIndex { 0 }; // frame when used
            BASE_NS::fixed_string<RENDER_NS::RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> postProcessName;
            BASE_NS::string customRngFile;
            BASE_NS::string customPostProcessRngFile;
            uint32_t multiViewCameraCount { 0U };
            uint64_t multiViewCameraHash { 0U };
        };
        struct SceneRngContainer {
            BASE_NS::string customRngFile;
            BASE_NS::string customPostSceneRngFile;

            RENDER_NS::RenderHandleReference rng;
            RENDER_NS::RenderHandleReference customRng;
            RENDER_NS::RenderHandleReference customPostRng;
        };

        // all render node graphs (scene rng is always the first and this needs to be in order)
        BASE_NS::vector<RENDER_NS::RenderHandleReference> orderedRenderNodeGraphs;
        // camera component id with generation hash
        BASE_NS::unordered_map<uint64_t, AdditionalCameraContainer> camIdToRng;

        SceneRngContainer sceneRngs;
        uint64_t sceneMainCamId { 0 };

        // reset every frame (flags for rendering hints)
        uint32_t frameFlags { 0u };

        // store created pods
        BASE_NS::vector<BASE_NS::string> postProcessPods;
        // store created post process data stores
        BASE_NS::vector<BASE_NS::string> postProcessConfigs;

        bool frameProcessed { false };
    };
    RenderProcessing renderProcessing_;

    struct CameraData {
        BASE_NS::Math::Mat4X4 view;
        BASE_NS::Math::Mat4X4 proj;
        uint64_t lastFrameIndex { 0 }; // frame when used
    };
    // store previous frame matrices
    BASE_NS::unordered_map<CORE_NS::Entity, CameraData> cameraData_;

    BASE_NS::unordered_map<CORE_NS::Entity, BatchDataVector> batches_;
    // used for render mesh batch processing
    BASE_NS::vector<RenderMeshData> renderMeshData_;

    // store default shader data for default materials in this ECS
    DefaultMaterialShaderData dmShaderData_;
};
CORE3D_END_NAMESPACE()

#endif // CORE_ECS_RENDERSYSTEM_H
