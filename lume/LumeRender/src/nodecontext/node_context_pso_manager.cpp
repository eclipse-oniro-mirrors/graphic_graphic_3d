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

#include "node_context_pso_manager.h"

#include <cstdint>

#include <base/containers/vector.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>

#include "device/device.h"
#include "device/gpu_program.h"
#include "device/gpu_program_util.h"
#include "device/gpu_resource_handle_util.h"
#include "device/gpu_resource_manager.h"
#include "device/pipeline_state_object.h"
#include "device/shader_manager.h"
#include "util/log.h"

template<>
uint64_t BASE_NS::hash(const RENDER_NS::ShaderSpecializationConstantDataView& specialization)
{
    uint64_t seed = BASE_NS::CompileTime::FNV_OFFSET_BASIS;
    if ((!specialization.data.empty()) && (!specialization.constants.empty())) {
        const size_t minSize = BASE_NS::Math::min(specialization.constants.size(), specialization.data.size());
        for (size_t idx = 0; idx < minSize; ++idx) {
            const auto& currConstant = specialization.constants[idx];
            uint64_t v = 0;
            const auto constantSize = RENDER_NS::GpuProgramUtil::SpecializationByteSize(currConstant.type);
            if ((currConstant.offset + constantSize) <= specialization.data.size_bytes()) {
                uint8_t const* data = (uint8_t const*)specialization.data.data() + currConstant.offset;
                size_t const bytes = sizeof(v) < constantSize ? sizeof(v) : constantSize;
                seed = BASE_NS::FNV1aHash(data, bytes, seed);
            }
#if (RENDER_VALIDATION_ENABLED == 1)
            else {
                PLUGIN_LOG_E("RENDER_VALIDATION: shader specialization issue with constant and data size mismatch");
            }
#endif
        }
    }
    return seed;
}

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
uint64_t HashComputeShader(
    const RenderHandle shaderHandle, const ShaderSpecializationConstantDataView& shaderSpecialization)
{
    return Hash(shaderHandle.id, shaderSpecialization);
}

uint64_t HashGraphicsShader(const RenderHandle shaderHandle, const RenderHandle graphicsStateHandle,
    const DynamicStateFlags dynamicStateFlags, const ShaderSpecializationConstantDataView& shaderSpecialization,
    const uint64_t customGraphicsStateHash)
{
    return Hash(
        shaderHandle.id, graphicsStateHandle.id, dynamicStateFlags, shaderSpecialization, customGraphicsStateHash);
}

#if (RENDER_VALIDATION_ENABLED == 1)
void validateSSO(
    ShaderManager& shaderMgr, const RenderHandle shaderHandle, const VertexInputDeclarationDataWrapper& vidw)
{
    const GpuShaderProgram* gsp = shaderMgr.GetGpuShaderProgram(shaderHandle);
    if (gsp) {
        const auto& reflection = gsp->GetReflection();
        const bool hasBindings = (reflection.vertexInputDeclarationView.bindingDescriptions.size() > 0);
        const bool vidHasBindings = (vidw.bindingDescriptions.size() > 0);
        if (hasBindings != vidHasBindings) {
            PLUGIN_LOG_E(
                "RENDER_VALIDATION: vertex input declaration pso (bindings: %u) mismatch with shader reflection "
                "(bindings: %u)",
                static_cast<uint32_t>(vidw.bindingDescriptions.size()),
                static_cast<uint32_t>(reflection.vertexInputDeclarationView.bindingDescriptions.size()));
        }
    }
}
#endif
} // namespace

NodeContextPsoManager::NodeContextPsoManager(Device& device, ShaderManager& shaderManager)
    : device_ { device }, shaderMgr_ { shaderManager }
{}

RenderHandle NodeContextPsoManager::GetComputePsoHandle(const RenderHandle shaderHandle,
    const PipelineLayout& pipelineLayout, const ShaderSpecializationConstantDataView& shaderSpecialization)
{
    // if not matching pso -> deferred creation in render backend
    RenderHandle psoHandle;

    auto& cache = computePipelineStateCache_;

    const uint64_t hash = HashComputeShader(shaderHandle, shaderSpecialization);
    const auto iter = cache.hashToHandle.find(hash);
    const bool needsNewPso = (iter == cache.hashToHandle.cend());
    if (needsNewPso) {
        PLUGIN_ASSERT(cache.psoCreationData.size() == cache.pipelineStateObjects.size());

        // reserve slot for new pso
        const uint32_t index = static_cast<uint32_t>(cache.psoCreationData.size());
        cache.pipelineStateObjects.emplace_back(nullptr);
        // add pipeline layout descriptor set mask to pso handle for fast evaluation
        uint32_t descriptorSetBitmask = 0;
        for (uint32_t idx = 0; idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++idx) {
            if (pipelineLayout.descriptorSetLayouts[idx].set != PipelineLayoutConstants::INVALID_INDEX) {
                descriptorSetBitmask |= (1 << idx);
            }
        }
        psoHandle = RenderHandleUtil::CreateHandle(RenderHandleType::COMPUTE_PSO, index, 0, descriptorSetBitmask);
        cache.hashToHandle[hash] = psoHandle;

#if (RENDER_VALIDATION_ENABLED == 1)
        cache.handleToPipelineLayout[psoHandle] = pipelineLayout;
#endif

        // store needed data for render backend pso creation
        ShaderSpecializationConstantDataWrapper ssw {
            vector<ShaderSpecialization::Constant>(
                shaderSpecialization.constants.begin(), shaderSpecialization.constants.end()),
            vector<uint32_t>(shaderSpecialization.data.begin(), shaderSpecialization.data.end()),
        };
        cache.psoCreationData.push_back({ shaderHandle, pipelineLayout, move(ssw) });
    } else {
        psoHandle = iter->second;
    }

    return psoHandle;
}

RenderHandle NodeContextPsoManager::GetComputePsoHandle(const RenderHandle shaderHandle,
    const RenderHandle pipelineLayoutHandle, const ShaderSpecializationConstantDataView& shaderSpecialization)
{
    RenderHandle psoHandle;
    if (RenderHandleUtil::GetHandleType(pipelineLayoutHandle) == RenderHandleType::PIPELINE_LAYOUT) {
        const PipelineLayout& pl = shaderMgr_.GetPipelineLayoutRef(pipelineLayoutHandle);
        psoHandle = GetComputePsoHandle(shaderHandle, pl, shaderSpecialization);
    } else {
        PLUGIN_LOG_E("NodeContextPsoManager: invalid pipeline layout handle given to GetComputePsoHandle()");
    }
    return psoHandle;
}

RenderHandle NodeContextPsoManager::GetGraphicsPsoHandleImpl(const RenderHandle shader,
    const RenderHandle graphicsState, const PipelineLayout& pipelineLayout,
    const VertexInputDeclarationView& vertexInputDeclarationView,
    const ShaderSpecializationConstantDataView& shaderSpecialization, const DynamicStateFlags dynamicStateFlags,
    const GraphicsState* customGraphicsState)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (RenderHandleUtil::GetHandleType(shader) != RenderHandleType::SHADER_STATE_OBJECT) {
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid shader handle given to graphics pso creation");
    }
    if (RenderHandleUtil::IsValid(graphicsState) &&
        RenderHandleUtil::GetHandleType(graphicsState) != RenderHandleType::GRAPHICS_STATE) {
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid graphics state hadnle given to graphics pso creation");
    }
#endif
    // if not matching pso -> deferred creation in render backend
    RenderHandle psoHandle;

    auto& cache = graphicsPipelineStateCache_;
    uint64_t cGfxHash = 0;
    if ((!RenderHandleUtil::IsValid(graphicsState)) && customGraphicsState) {
        cGfxHash = shaderMgr_.HashGraphicsState(*customGraphicsState);
    }
    const uint64_t hash = HashGraphicsShader(shader, graphicsState, dynamicStateFlags, shaderSpecialization, cGfxHash);
    const auto iter = cache.hashToHandle.find(hash);
    const bool needsNewPso = (iter == cache.hashToHandle.cend());
    if (needsNewPso) {
        const uint32_t index = static_cast<uint32_t>(cache.psoCreationData.size());
        // add pipeline layout descriptor set mask to pso handle for fast evaluation
        uint32_t descriptorSetBitmask = 0;
        for (uint32_t idx = 0; idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++idx) {
            if (pipelineLayout.descriptorSetLayouts[idx].set != PipelineLayoutConstants::INVALID_INDEX) {
                descriptorSetBitmask |= (1 << idx);
            }
        }
        psoHandle = RenderHandleUtil::CreateHandle(RenderHandleType::GRAPHICS_PSO, index, 0, descriptorSetBitmask);
        cache.hashToHandle[hash] = psoHandle;

        // store needed data for render backend pso creation
        ShaderSpecializationConstantDataWrapper ssw {
            { shaderSpecialization.constants.begin(), shaderSpecialization.constants.end() },
            { shaderSpecialization.data.begin(), shaderSpecialization.data.end() },
        };
        VertexInputDeclarationDataWrapper vidw {
            { vertexInputDeclarationView.bindingDescriptions.begin(),
                vertexInputDeclarationView.bindingDescriptions.end() },
            { vertexInputDeclarationView.attributeDescriptions.begin(),
                vertexInputDeclarationView.attributeDescriptions.end() },
        };
#if (RENDER_VALIDATION_ENABLED == 1)
        validateSSO(shaderMgr_, shader, vidw);
        cache.handleToPipelineLayout[psoHandle] = pipelineLayout;
#endif
        // custom graphics state or null
        unique_ptr<GraphicsState> customGraphicsStatePtr =
            customGraphicsState ? make_unique<GraphicsState>(*customGraphicsState) : nullptr;
        cache.psoCreationData.push_back({ shader, graphicsState, pipelineLayout, dynamicStateFlags, move(vidw),
            move(ssw), move(customGraphicsStatePtr) });
    } else {
        psoHandle = iter->second;
    }

    return psoHandle;
}

RenderHandle NodeContextPsoManager::GetGraphicsPsoHandle(const RenderHandle shaderHandle,
    const RenderHandle graphicsState, const RenderHandle pipelineLayoutHandle,
    const RenderHandle vertexInputDeclarationHandle, const ShaderSpecializationConstantDataView& shaderSpecialization,
    const DynamicStateFlags dynamicStateFlags)
{
    RenderHandle psoHandle;
    const PipelineLayout& pl = shaderMgr_.GetPipelineLayout(pipelineLayoutHandle);
    VertexInputDeclarationView vidView = shaderMgr_.GetVertexInputDeclarationView(vertexInputDeclarationHandle);
    const RenderHandle gfxStateHandle =
        (RenderHandleUtil::GetHandleType(graphicsState) == RenderHandleType::GRAPHICS_STATE)
            ? graphicsState
            : shaderMgr_.GetGraphicsStateHandleByShaderHandle(shaderHandle).GetHandle();
    psoHandle = GetGraphicsPsoHandleImpl(
        shaderHandle, gfxStateHandle, pl, vidView, shaderSpecialization, dynamicStateFlags, nullptr);
    return psoHandle;
}

RenderHandle NodeContextPsoManager::GetGraphicsPsoHandle(const RenderHandle shader, const RenderHandle graphicsState,
    const PipelineLayout& pipelineLayout, const VertexInputDeclarationView& vertexInputDeclarationView,
    const ShaderSpecializationConstantDataView& shaderSpecialization, const DynamicStateFlags dynamicStateFlags)
{
    return GetGraphicsPsoHandleImpl(shader, graphicsState, pipelineLayout, vertexInputDeclarationView,
        shaderSpecialization, dynamicStateFlags, nullptr);
}

RenderHandle NodeContextPsoManager::GetGraphicsPsoHandle(const RenderHandle shader, const GraphicsState& graphicsState,
    const PipelineLayout& pipelineLayout, const VertexInputDeclarationView& vertexInputDeclarationView,
    const ShaderSpecializationConstantDataView& shaderSpecialization, const DynamicStateFlags dynamicStateFlags)
{
    return GetGraphicsPsoHandleImpl(shader, RenderHandle {}, pipelineLayout, vertexInputDeclarationView,
        shaderSpecialization, dynamicStateFlags, &graphicsState);
}

#if (RENDER_VALIDATION_ENABLED == 1)
const PipelineLayout& NodeContextPsoManager::GetComputePsoPipelineLayout(const RenderHandle handle) const
{
    auto& handleToPl = computePipelineStateCache_.handleToPipelineLayout;
    if (const auto iter = handleToPl.find(handle); iter != handleToPl.cend()) {
        return iter->second;
    } else {
        static PipelineLayout pl;
        return pl;
    }
}
#endif

#if (RENDER_VALIDATION_ENABLED == 1)
const PipelineLayout& NodeContextPsoManager::GetGraphicsPsoPipelineLayout(const RenderHandle handle) const
{
    auto& handleToPl = graphicsPipelineStateCache_.handleToPipelineLayout;
    if (const auto iter = handleToPl.find(handle); iter != handleToPl.cend()) {
        return iter->second;
    } else {
        static PipelineLayout pl;
        return pl;
    }
}
#endif

const ComputePipelineStateObject* NodeContextPsoManager::GetComputePso(
    const RenderHandle handle, const LowLevelPipelineLayoutData* pipelineLayoutData)
{
    PLUGIN_ASSERT(RenderHandleUtil::GetHandleType(handle) == RenderHandleType::COMPUTE_PSO);
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    PLUGIN_ASSERT_MSG(index < (uint32_t)computePipelineStateCache_.psoCreationData.size(),
        "Check that IRenderNode::InitNode clears cached handles.");

    auto& cache = computePipelineStateCache_;
    if (cache.pipelineStateObjects[index] == nullptr) { // pso needs to be created
        PLUGIN_ASSERT(index < static_cast<uint32_t>(cache.psoCreationData.size()));
        const auto& psoDataRef = cache.psoCreationData[index];
        const GpuComputeProgram* gcp = shaderMgr_.GetGpuComputeProgram(psoDataRef.shaderHandle);
        PLUGIN_ASSERT(gcp);
        if (gcp) {
            const ShaderSpecializationConstantDataView sscdv {
                psoDataRef.shaderSpecialization.constants,
                psoDataRef.shaderSpecialization.data,
            };
            cache.pipelineStateObjects[index] =
                device_.CreateComputePipelineStateObject(*gcp, psoDataRef.pipelineLayout, sscdv, pipelineLayoutData);
        }
    }
    return cache.pipelineStateObjects[index].get();
}

const GraphicsPipelineStateObject* NodeContextPsoManager::GetGraphicsPso(const RenderHandle handle,
    const RenderPassDesc& renderPassDesc, const array_view<const RenderPassSubpassDesc> renderPassSubpassDescs,
    const uint32_t subpassIndex, const uint64_t psoStateHash, const LowLevelRenderPassData* renderPassData,
    const LowLevelPipelineLayoutData* pipelineLayoutData)
{
    PLUGIN_ASSERT(RenderHandleUtil::GetHandleType(handle) == RenderHandleType::GRAPHICS_PSO);
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    PLUGIN_ASSERT_MSG(index < (uint32_t)graphicsPipelineStateCache_.psoCreationData.size(),
        "Check that IRenderNode::InitNode clears cached handles.");

    auto& cache = graphicsPipelineStateCache_;
    const uint64_t hash =
        (device_.GetBackendType() == DeviceBackendType::VULKAN) ? Hash(handle.id, psoStateHash) : handle.id;
    if (const auto iter = cache.pipelineStateObjects.find(hash); iter != cache.pipelineStateObjects.cend()) {
        return iter->second.get();
    } else {
        PLUGIN_ASSERT(index < static_cast<uint32_t>(cache.psoCreationData.size()));
        const auto& psoDataRef = cache.psoCreationData[index];
        const GpuShaderProgram* gsp = shaderMgr_.GetGpuShaderProgram(psoDataRef.shaderHandle);
        PLUGIN_ASSERT(gsp);
        if (gsp) {
            const GraphicsState* customGraphicsState = psoDataRef.customGraphicsState.get();
            const GraphicsState& graphicsState = (customGraphicsState)
                                                     ? *customGraphicsState
                                                     : shaderMgr_.GetGraphicsStateRef(psoDataRef.graphicsStateHandle);
#if (RENDER_VALIDATION_ENABLED == 1)
            if (graphicsState.colorBlendState.colorAttachmentCount !=
                renderPassSubpassDescs[subpassIndex].colorAttachmentCount) {
                PLUGIN_LOG_ONCE_I("node_context_pso_output_info",
                    "RENDER_VALIDATION: graphics state color attachment count (%u) does not match "
                    "render pass subpass color attachment count (%u). (Output not consumed info)",
                    graphicsState.colorBlendState.colorAttachmentCount,
                    renderPassSubpassDescs[subpassIndex].colorAttachmentCount);
            }
#endif
            const DynamicStateFlags combinedDynamicStateFlags =
                (graphicsState.dynamicStateFlags | psoDataRef.dynamicStateFlags);

            const auto& vertexInput = psoDataRef.vertexInputDeclaration;
            const VertexInputDeclarationView vidv { vertexInput.bindingDescriptions,
                vertexInput.attributeDescriptions };

            const auto& shaderSpec = psoDataRef.shaderSpecialization;
            const ShaderSpecializationConstantDataView sscdv { shaderSpec.constants, shaderSpec.data };

            auto& newPsoRef = cache.pipelineStateObjects[hash];
            newPsoRef = device_.CreateGraphicsPipelineStateObject(*gsp, graphicsState, psoDataRef.pipelineLayout, vidv,
                sscdv, combinedDynamicStateFlags, renderPassDesc, renderPassSubpassDescs, subpassIndex, renderPassData,
                pipelineLayoutData);
            return newPsoRef.get();
        }
    }
    return nullptr;
}
RENDER_END_NAMESPACE()
