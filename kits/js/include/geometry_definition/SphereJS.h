/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef GEOMETRY_DEFINITION_SPHERE_JS_H
#define GEOMETRY_DEFINITION_SPHERE_JS_H

#include <napi_api.h>
#include <scene/interface/intf_create_mesh.h>

#include "geometry_definition/GeometryDefinition.h"

namespace GeometryDefinition {

class SphereJS : public GeometryDefinition {
public:
    ~SphereJS() override = default;
    static GeometryDefinition* FromJs(NapiApi::Object& jsDefinition);
    virtual SCENE_NS::IMesh::Ptr CreateMesh(
        const SCENE_NS::ICreateMesh::Ptr& creator, const SCENE_NS::MeshConfig& config) const override;

    static void Init(napi_env env, napi_value exports);

private:
    explicit SphereJS(float radius, uint32_t segmentCount);
    float radius_;
    uint32_t segmentCount_;
};

} // namespace GeometryDefinition

#endif
