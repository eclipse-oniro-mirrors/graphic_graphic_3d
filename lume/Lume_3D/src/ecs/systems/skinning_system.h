/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_ECS_SKINNINGSYSTEM_H
#define CORE_ECS_SKINNINGSYSTEM_H

#include <ComponentTools/component_query.h>
#include <PropertyTools/property_api_impl.h>

#include <3d/ecs/systems/intf_skinning_system.h>
#include <base/math/matrix.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()
class INodeComponentManager;
class ISkinComponentManager;
class ISkinIbmComponentManager;
class ISkinJointsComponentManager;
class IJointMatricesComponentManager;
class IPreviousJointMatricesComponentManager;
class IWorldMatrixComponentManager;
class INodeSystem;
class ICameraComponentManager;
class ILightComponentManager;
class IMeshComponentManager;
class IRenderMeshComponentManager;
class IPicking;

struct JointMatricesComponent;

class SkinningSystem final : public ISkinningSystem {
public:
    explicit SkinningSystem(CORE_NS::IEcs& ecs);
    ~SkinningSystem() override = default;
    BASE_NS::string_view GetName() const override;
    BASE_NS::Uid GetUid() const override;
    CORE_NS::IPropertyHandle* GetProperties() override;
    const CORE_NS::IPropertyHandle* GetProperties() const override;
    void SetProperties(const CORE_NS::IPropertyHandle&) override;

    bool IsActive() const override;
    void SetActive(bool state) override;

    void Initialize() override;
    bool Update(bool frameRenderingQueued, uint64_t time, uint64_t delta) override;
    void Uninitialize() override;

    const CORE_NS::IEcs& GetECS() const override;

    void CreateInstance(CORE_NS::Entity const& skinIbmEntity, BASE_NS::array_view<const CORE_NS::Entity> const& joints,
        CORE_NS::Entity const& entity, CORE_NS::Entity const& skeleton) override;
    void DestroyInstance(CORE_NS::Entity const& entity) override;

private:
    void UpdateSkin(const CORE_NS::ComponentQuery::ResultRow& row);
    void UpdateJointTransformations(bool isEnabled, const BASE_NS::array_view<CORE_NS::Entity const>& jointEntities,
        const BASE_NS::array_view<BASE_NS::Math::Mat4X4 const>& iblMatrices, JointMatricesComponent& jointMatrices,
        const BASE_NS::Math::Mat4X4& skinEntityWorldInverse);

    bool active_;
    CORE_NS::IEcs& ecs_;

    IPicking& picking_;

    ISkinComponentManager& skinManager_;
    ISkinIbmComponentManager& skinIbmManager_;
    ISkinJointsComponentManager& skinJointsManager_;
    IJointMatricesComponentManager& jointMatricesManager_;
    IPreviousJointMatricesComponentManager& previousJointMatricesManager_;
    IWorldMatrixComponentManager& worldMatrixManager_;
    INodeComponentManager& nodeManager_;
    IRenderMeshComponentManager& renderMeshManager_;
    IMeshComponentManager& meshManager_;
    INodeSystem* nodeSystem_ = nullptr;

    CORE_NS::ComponentQuery componentQuery_;

    uint32_t worldMatrixGeneration_ { 0 };
    uint32_t jointMatricesGeneration_ { 0 };
    CORE_NS::PropertyApiImpl<void> SKINNING_SYSTEM_PROPERTIES;
};
CORE3D_END_NAMESPACE()

#endif // CORE_ECS_SKINNINGSYSTEM_H
