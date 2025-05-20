/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef RENDER_RENDER_POST_PROCESS_RENDER_POST_PROCESS_BLUR_NODE_H
#define RENDER_RENDER_POST_PROCESS_RENDER_POST_PROCESS_BLUR_NODE_H

#include <base/containers/unique_ptr.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_interface_helper.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.h>
#include <core/property_tools/property_macros.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_shader_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_post_process.h>
#include <render/nodecontext/intf_render_post_process_node.h>
#include <render/property/property_types.h>
#include <render/render_data_structures.h>
#include <render/shaders/common/render_blur_common.h>

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;

class RenderPostProcessBlurNode final : public CORE_NS::IInterfaceHelper<IRenderPostProcessNode> {
public:
    static constexpr auto UID = BASE_NS::Uid("65ec929c-c38c-4d4e-b0a0-62fffb55d338");

    using Ptr = BASE_NS::refcnt_ptr<RenderPostProcessBlurNode>;

    // needs to match the shader
    enum BlurShaderType {
        BLUR_SHADER_TYPE_RGBA = 0,
        BLUR_SHADER_TYPE_R = 1,
        BLUR_SHADER_TYPE_RG = 2,
        BLUR_SHADER_TYPE_RGB = 3,
        BLUR_SHADER_TYPE_A = 4,
        BLUR_SHADER_TYPE_SOFT_DOWNSCALE_RGB = 5,
        BLUR_SHADER_TYPE_DOWNSCALE_RGBA = 6,
        BLUR_SHADER_TYPE_DOWNSCALE_RGBA_ALPHA_WEIGHT = 7,
        BLUR_SHADER_TYPE_RGBA_ALPHA_WEIGHT = 8,
    };
    struct EffectProperties {
        bool enabled { false };
        BlurConfiguration blurConfiguration;
        uint32_t blurShaderType { BlurShaderType::BLUR_SHADER_TYPE_RGBA };
        uint32_t blurShaderScaleType { BlurShaderType::BLUR_SHADER_TYPE_DOWNSCALE_RGBA };
    };

    RenderPostProcessBlurNode();
    ~RenderPostProcessBlurNode() = default;

    CORE_NS::IPropertyHandle* GetRenderInputProperties() override;
    CORE_NS::IPropertyHandle* GetRenderOutputProperties() override;
    DescriptorCounts GetRenderDescriptorCounts() const override;
    void SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest) override;

    IRenderNode::ExecuteFlags GetExecuteFlags() const override;
    void Init(const IRenderPostProcess::Ptr& postProcess, IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecute() override;
    void Execute(IRenderCommandList& cmdList) override;

    struct NodeInputs {
        BindableImage input;
    };
    struct NodeOutputs {
        BindableImage output;
    };

    NodeInputs nodeInputsData;
    NodeOutputs nodeOutputsData;

private:
    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };
    IRenderPostProcess::Ptr postProcess_;

    struct ImageData {
        BASE_NS::Math::UVec2 size { 0U, 0U };
        uint32_t mipCount { 0U };
        RenderHandleReference handle;
        RenderHandle rawHandle;
        RenderHandleReference handleExt;
        RenderHandle rawHandleExt;
    };
    ImageData tmpImgTargets_;
    ImageData inputImgData_;

    void EvaluateTemporaryTargets();
    bool EvaluateInOut();
    void CheckDescriptorSetNeed();
    void RenderData(IRenderCommandList& cmdList, const RenderPass& renderPassBase);
    void RenderGaussian(IRenderCommandList& cmdList, const RenderPass& renderPassBase);
    void RenderDirectionalBlur(IRenderCommandList& cmdList, const RenderPass& renderPassBase);

    struct PushConstantStruct {
        BASE_NS::Math::Vec4 texSizeInvTexSize;
    };

    RenderAreaRequest renderAreaRequest_;
    bool useRequestedRenderArea_ { false };

    struct PipelineData {
        IShaderManager::GraphicsShaderData gsd;
        RenderHandle psoScale;
        RenderHandle psoBlur;
    };
    PipelineData pipelineData_;

    CORE_NS::PropertyApiImpl<NodeInputs> inputProperties_;
    CORE_NS::PropertyApiImpl<NodeOutputs> outputProperties_;

    BindableImage defaultInput_;

    DescriptorCounts descriptorCounts_;
    bool valid_ { false };

    BASE_NS::vector<IDescriptorSetBinder::Ptr> binders_;

    EffectProperties effectProperties_;
    bool mipsBlur { true };
};
RENDER_END_NAMESPACE()

#endif // RENDER_RENDER_POST_PROCESS_RENDER_POST_PROCESS_BLUR_NODE_H
