/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_MORPH_H
#define CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_MORPH_H

#include <cstdint>

#include <3d/render/intf_render_data_store_morph.h>
#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
/**
RenderDataStoreMorph implementation.
*/
class RenderDataStoreMorph final : public IRenderDataStoreMorph {
public:
    explicit RenderDataStoreMorph(const BASE_NS::string_view name);
    ~RenderDataStoreMorph() override = default;

    void Init(const IRenderDataStoreMorph::ReserveSize& reserveSize);

    void PreRender() override {};
    void PreRenderBackend() override {};
    // Reset and start indexing from the beginning. i.e. frame boundary reset.
    void PostRender() override;
    void Clear() override;

    // Add submeshes safely.
    void AddSubmesh(const RenderDataMorph::Submesh& submesh) override;

    BASE_NS::array_view<const RenderDataMorph::Submesh> GetSubmeshes() const override;

    // for plugin / factory interface
    static constexpr char const* const TYPE_NAME = "RenderDataStoreMorph";
    static IRenderDataStore* Create(RENDER_NS::IRenderContext& renderContext, char const* name);
    static void Destroy(IRenderDataStore* instance);

    BASE_NS::string_view GetTypeName() const override
    {
        return TYPE_NAME;
    }

    BASE_NS::string_view GetName() const override
    {
        return name_;
    }

    const BASE_NS::Uid& GetUid() const override
    {
        return UID;
    }

private:
    const BASE_NS::string name_;

    BASE_NS::vector<RenderDataMorph::Submesh> submeshes_;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_MORPH_H
