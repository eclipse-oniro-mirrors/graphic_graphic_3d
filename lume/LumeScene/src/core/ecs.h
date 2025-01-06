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

#ifndef SCENE_SRC_CORE_ECS_H
#define SCENE_SRC_CORE_ECS_H

#include <ComponentTools/component_query.h>
#include <scene/base/types.h>
#include <scene/ext/intf_ecs_context.h>
#include <text_3d/ecs/components/text_component.h>
#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_state_component.h>
#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/layer_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/util/intf_picking.h>
#include <core/ecs/intf_ecs.h>
#include <core/intf_engine.h>
#include <render/intf_render_context.h>

#include "ecs_listener.h"

SCENE_BEGIN_NAMESPACE()

class InternalScene;

class Ecs : public META_NS::IntroduceInterfaces<IEcsContext>, private EcsListener {
public:
    bool Initialize(const BASE_NS::shared_ptr<IInternalScene>& scene);
    void Uninitialize();

    CORE3D_NS::ISceneNode* GetNode(CORE_NS::Entity ent);
    const CORE3D_NS::ISceneNode* GetNode(CORE_NS::Entity ent) const;
    void RemoveNode(CORE_NS::Entity ent);

    const CORE3D_NS::ISceneNode* FindNode(BASE_NS::string_view path) const;
    const CORE3D_NS::ISceneNode* FindNodeParent(BASE_NS::string_view path) const;

    bool SetNodeParentAndName(CORE_NS::Entity ent, BASE_NS::string_view name, const CORE3D_NS::ISceneNode* parent);
    bool SetRenderMode(RenderMode);
    RenderMode GetRenderMode() const;

    BASE_NS::string GetPath(const CORE3D_NS::ISceneNode* node) const;
    bool IsNodeEntity(CORE_NS::Entity ent) const;

public:
    bool CreateUnnamedRootNode() override;

    CORE_NS::IComponentManager* FindComponent(BASE_NS::string_view name) const override;
    CORE_NS::IEcs::Ptr GetNativeEcs() const override
    {
        return ecs;
    }

    void AddDefaultComponents(CORE_NS::Entity ent) const override;
    BASE_NS::string GetPath(CORE_NS::Entity ent) const override;
    IEcsObject::Ptr GetEcsObject(CORE_NS::Entity) override;
    void RemoveEcsObject(const IEcsObject::ConstPtr&) override;
    CORE_NS::Entity GetRootEntity() const override;

public:
    CORE_NS::IEcs::Ptr ecs;

    CORE3D_NS::IAnimationComponentManager* animationComponentManager {};
    CORE3D_NS::IAnimationStateComponentManager* animationStateComponentManager {};
    CORE3D_NS::ICameraComponentManager* cameraComponentManager {};

    CORE3D_NS::IEnvironmentComponentManager* envComponentManager {};
    CORE3D_NS::ILayerComponentManager* layerComponentManager {};
    CORE3D_NS::ILightComponentManager* lightComponentManager {};
    CORE3D_NS::IMaterialComponentManager* materialComponentManager {};
    CORE3D_NS::IMeshComponentManager* meshComponentManager {};
    CORE3D_NS::INameComponentManager* nameComponentManager {};
    CORE3D_NS::INodeComponentManager* nodeComponentManager {};
    CORE3D_NS::IRenderMeshComponentManager* renderMeshComponentManager {};
    CORE3D_NS::IRenderHandleComponentManager* rhComponentManager {};
    CORE3D_NS::ITransformComponentManager* transformComponentManager {};
    CORE3D_NS::IUriComponentManager* uriComponentManager {};
    CORE3D_NS::IRenderConfigurationComponentManager* renderConfigComponentManager {};
    CORE3D_NS::IPostProcessComponentManager* postProcessComponentManager {};
    CORE3D_NS::ILocalMatrixComponentManager* localMatrixComponentManager {};
    CORE3D_NS::IWorldMatrixComponentManager* worldMatrixComponentManager {};

    TEXT3D_NS::ITextComponentManager* textComponentManager {};
    BASE_NS::unique_ptr<CORE_NS::ComponentQuery> meshQuery {};
    BASE_NS::unique_ptr<CORE_NS::ComponentQuery> materialQuery {};
    BASE_NS::unique_ptr<CORE_NS::ComponentQuery> animationQuery {};

    CORE3D_NS::INodeSystem* nodeSystem {};
    CORE3D_NS::IPicking* picking {};

private:
    template<typename Manager>
    Manager* GetCoreManager();

private:
    BASE_NS::weak_ptr<IInternalScene> scene_;
    CORE3D_NS::ISceneNode* rootNode_ {};
    BASE_NS::unordered_map<BASE_NS::string, CORE_NS::IComponentManager*> components_;
};

CORE_NS::EntityReference CopyExternalAsChild(const IEcsObject& parent, const IEcsObject& extChild);
CORE_NS::EntityReference CopyExternalAsChild(const IEcsObject& parent, const IScene& extScene);

SCENE_END_NAMESPACE()

#endif