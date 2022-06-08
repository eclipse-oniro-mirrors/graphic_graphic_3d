/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_ECS_NODESYSTEM_H
#define CORE_ECS_NODESYSTEM_H

#include <ComponentTools/component_query.h>

#include <3d/ecs/systems/intf_node_system.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/math/matrix.h>
#include <core/namespace.h>

#include "property/property_handle.h"

CORE3D_BEGIN_NAMESPACE()
class INameComponentManager;
class INodeComponentManager;
class ILocalMatrixComponentManager;
class IPreviousWorldMatrixComponentManager;
class IWorldMatrixComponentManager;
class ITransformComponentManager;

class NodeSystem final : public INodeSystem {
public:
    explicit NodeSystem(CORE_NS::IEcs& ecs);
    ~NodeSystem() override = default;

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

    // INodeSystem.
    ISceneNode& GetRootNode() const override;

    ISceneNode* GetNode(CORE_NS::Entity entity) const override;

    ISceneNode* CreateNode() override;

    ISceneNode* CloneNode(const ISceneNode& node, bool recursive) override;

    void DestroyNode(ISceneNode& rootNode) override;

private:
    class NodeAccess;
    class SceneNode;
    class NodeCache;

    void CollectChangedNodes(ISceneNode& node, BASE_NS::vector<ISceneNode*>& result);
    void UpdateTransformationRecursive(ISceneNode& node, BASE_NS::Math::Mat4X4 const& matrix, bool enabled);
    void GatherNodeEntities(const ISceneNode& node, BASE_NS::vector<CORE_NS::Entity>& entities) const;
    bool UpdatePreviousWorldMatrices();

    CORE_NS::IEcs& ecs_;
    bool active_ = true;

    INameComponentManager& nameManager_;
    INodeComponentManager& nodeManager_;
    ITransformComponentManager& transformManager_;
    ILocalMatrixComponentManager& localMatrixManager_;
    IWorldMatrixComponentManager& worldMatrixManager_;
    IPreviousWorldMatrixComponentManager& prevWorldMatrixManager_;

    BASE_NS::unique_ptr<NodeCache> cache_;

    CORE_NS::ComponentQuery nodeQuery_;

    uint32_t localMatrixGeneration_ = 0;
    uint32_t worldMatrixGeneration_ = 0;
    uint32_t nodeGeneration_ = 0;
};
CORE3D_END_NAMESPACE()

#endif // CORE_ECS_NODESYSTEM_H
