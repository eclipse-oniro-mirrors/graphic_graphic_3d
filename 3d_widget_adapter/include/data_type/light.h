/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_LIGHT_H
#define OHOS_RENDER_3D_LIGHT_H

#include "data_type/constants.h"
#include "data_type/position.h"
#include "data_type/quaternion.h"
#include "data_type/vec3.h"

namespace OHOS::Render3D {
class Light {
public:
    Light(LightType type, const Vec3& color, float intensity, bool shadow, const Position& position,
        const Quaternion& rotation)
        : type_(type), color_(color), intensity_(intensity), shadow_(shadow), position_(position), rotation_(rotation)
    {
    }

    void SetLightType(LightType type)
    {
        type_ = type;
    }

    void SetColor(const OHOS::Render3D::Vec3& color)
    {
        color_ = color;
    }

    void SetIntensity(float intensity)
    {
        intensity_ = intensity;
    }

    void SetLightShadow(bool shadow)
    {
        shadow_ = shadow;
    }

    void SetPosition(const OHOS::Render3D::Position& position)
    {
        position_.SetPosition(position.GetPosition());
        position_.SetDistance(position.GetDistance());
        position_.SetIsAngular(position.GetIsAngular());
    }

    void SetRotation(const OHOS::Render3D::Quaternion& rotation)
    {
        rotation_ = rotation;
    }

    LightType GetLightType() const
    {
        return type_;
    }

    const Vec3& GetLightColor() const
    {
        return color_;
    }

    float GetLightIntensity() const
    {
        return intensity_;
    }

    bool GetLightShadow() const
    {
        return shadow_;
    }

    const Position& GetPosition() const
    {
        return position_;
    }

    const Quaternion& GetRotation() const
    {
        return rotation_;
    }

private:
    LightType type_ = LightType::DIRECTIONAL;
    OHOS::Render3D::Vec3 color_ { 1.0f, 1.0f, 1.0f };
    float intensity_ { 10.0f };
    bool shadow_ = false;
    OHOS::Render3D::Position position_;
    OHOS::Render3D::Quaternion rotation_ { -999999.0f, -999999.0f, -999999.0f, -999999.0f };
};

} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_LIGHT_H
