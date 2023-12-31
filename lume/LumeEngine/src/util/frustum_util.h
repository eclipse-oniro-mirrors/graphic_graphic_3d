/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#ifndef CORE_UTIL_FRUSTUM_UTIL_H
#define CORE_UTIL_FRUSTUM_UTIL_H
#include <core/util/intf_frustum_util.h>

CORE_BEGIN_NAMESPACE()
class FrustumUtil final : public IFrustumUtil {
public:
    Frustum CreateFrustum(const BASE_NS::Math::Mat4X4& matrix) const override;
    bool SphereFrustumCollision(
        const Frustum& frustum, const BASE_NS::Math::Vec3 pos, const float radius) const override;

    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    void Ref() override;
    void Unref() override;
};
CORE_END_NAMESPACE()
#endif // CORE_UTIL_FRUSTUM_UTIL_H