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

#ifndef SCENE_SRC_RESOURCE_TYPES_GLTF_SCENE_TYPE_H
#define SCENE_SRC_RESOURCE_TYPES_GLTF_SCENE_TYPE_H

#include <scene/interface/intf_render_context.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/resource/types.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class GltfSceneResourceType : public META_NS::IntroduceInterfaces<META_NS::MetaObject, CORE_NS::IResourceType> {
    META_OBJECT(GltfSceneResourceType, ClassId::GltfSceneResource, IntroduceInterfaces)
public:
    bool Build(const META_NS::IMetadata::Ptr&) override;

    BASE_NS::string GetResourceName() const override;
    BASE_NS::Uid GetResourceType() const override;
    CORE_NS::IResource::Ptr LoadResource(const StorageInfo&) const override;
    bool SaveResource(const CORE_NS::IResource::ConstPtr&, const StorageInfo&) const override;
    bool ReloadResource(const StorageInfo&, const CORE_NS::IResource::Ptr&) const override;

private:
    IRenderContext::WeakPtr context_;
};

SCENE_END_NAMESPACE()

#endif