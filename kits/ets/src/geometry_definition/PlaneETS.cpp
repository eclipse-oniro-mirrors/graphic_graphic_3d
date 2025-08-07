/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "geometry_definition/PlaneETS.h"

namespace OHOS::Render3D::Geometry {

PlaneETS::PlaneETS(const BASE_NS::Math::Vec2 &size) : GeometryDefinition(), size_(size)
{}

GeometryType PlaneETS::GetType()
{
    return GeometryType::PLANE;
}

SCENE_NS::IMesh::Ptr PlaneETS::CreateMesh(
    const SCENE_NS::ICreateMesh::Ptr &creator, const SCENE_NS::MeshConfig &config) const
{
    return creator->CreatePlane(config, size_.x, size_.y).GetResult();
}

}  // namespace OHOS::Render3D::Geometry