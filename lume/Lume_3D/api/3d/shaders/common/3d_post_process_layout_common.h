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

#ifndef SHADERS_COMMON_3D_POST_PROCESS_FRAGMENT_LAYOUT_COMMON_H
#define SHADERS_COMMON_3D_POST_PROCESS_FRAGMENT_LAYOUT_COMMON_H

#include "3d/shaders/common/3d_dm_structures_common.h"
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_post_process_layout_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

#ifdef VULKAN

// sets

// NOTE: maps to default render_post_process_layout_common.h
// 0 0 uGlobalData
// 0 1 uLocalData
//
// Set 0 for general, camera, and lighting data
// UBOs visible in both VS and FS
layout(set = 0, binding = 2, std140) uniform uCameraMatrices
{
    DefaultCameraMatrixStruct uCameras[CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT];
};
layout(set = 0, binding = 3, std140) uniform uGeneralStructData
{
    DefaultMaterialGeneralDataStruct uGeneralData;
};
layout(set = 0, binding = 4, std140) uniform uEnvironmentStructData
{
    DefaultMaterialEnvironmentStruct uEnvironmentData;
};
layout(set = 0, binding = 5, std140) uniform uFogStructData
{
    DefaultMaterialFogStruct uFogData;
};
layout(set = 0, binding = 6, std140) uniform uLightStructData
{
    DefaultMaterialLightStruct uLightData;
};
layout(set = 0, binding = 7, std140) uniform uPostProcessStructData
{
    GlobalPostProcessStruct uPostProcessData;
};
layout(set = 0, binding = 8, std430) buffer uLightClusterIndexData
{
    DefaultMaterialLightClusterData uLightClusterData[];
};
layout(set = 0, binding = 9) uniform CORE_RELAXEDP sampler2D uSampColorPrePass;
layout(set = 0, binding = 10) uniform sampler2D uSampColorShadow;       // VSM or other
layout(set = 0, binding = 11) uniform sampler2DShadow uSampDepthShadow; // PCF
layout(set = 0, binding = 12) uniform samplerCube uSampRadiance;

#else

#endif

#endif // SHADERS_COMMON_3D_POST_PROCESS_FRAGMENT_LAYOUT_COMMON_H
