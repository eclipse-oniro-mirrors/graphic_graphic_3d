/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__RENDER__NODE__RENDER_NODE_CREATE_DEFAULT_CAMERA_GPU_IMAGES_H
#define CORE__RENDER__NODE__RENDER_NODE_CREATE_DEFAULT_CAMERA_GPU_IMAGES_H

#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/device/gpu_resource_desc.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

CORE3D_BEGIN_NAMESPACE()
class RenderNodeCreateDefaultCameraGpuImages final : public RENDER_NS::IRenderNode {
public:
    RenderNodeCreateDefaultCameraGpuImages() = default;
    ~RenderNodeCreateDefaultCameraGpuImages() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "c694a5bd-c6f6-4167-9b61-cf481632c171" };
    static constexpr char const* TYPE_NAME = "RenderNodeCreateDefaultCameraGpuImages";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void ParseRenderNodeInputs();

    struct JsonInputs {
        BASE_NS::vector<RENDER_NS::RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc> gpuImageDescs;
    };
    JsonInputs jsonInputs_;

    SceneRenderDataStores stores_;

    struct Names {
        BASE_NS::string globalName;
        BASE_NS::string shareName;
    };
    BASE_NS::vector<Names> imageNames_;
    BASE_NS::vector<RENDER_NS::GpuImageDesc> descs_;
    BASE_NS::vector<RENDER_NS::RenderHandleReference> resourceHandles_;

    BASE_NS::string customCameraName_;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_CREATE_DEFAULT_CAMERA_GPU_IMAGES_H
