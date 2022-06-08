/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_ECS_LOCALMATRIXSYSTEM_H
#define CORE_ECS_LOCALMATRIXSYSTEM_H

#include <ComponentTools/component_query.h>

#include <3d/namespace.h>
#include <core/ecs/intf_system.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class ILocalMatrixComponentManager;
class ITransformComponentManager;

class LocalMatrixSystem final : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID { "4de00235-c9cd-44d0-94ef-2ef9bbffa088" };

    explicit LocalMatrixSystem(CORE_NS::IEcs& ecs);
    ~LocalMatrixSystem() override = default;

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

private:
    bool active_;
    CORE_NS::IEcs& ecs_;

    ILocalMatrixComponentManager* localMatrixManager_ { nullptr };
    ITransformComponentManager* transformManager_ { nullptr };

    uint32_t transformGeneration_ = 0;
    BASE_NS::vector<uint32_t> transformComponentGenerations_;

    CORE_NS::ComponentQuery componentQuery_;
};

inline constexpr const BASE_NS::string_view GetName(const LocalMatrixSystem*)
{
    return "LocalMatrixSystem";
}
CORE3D_END_NAMESPACE()

#endif // CORE_ECS_LOCALMATRIXSYSTEM_H
