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

#ifndef API_RENDER_SHADERS_COMMON_CORE_COMPATIBILITY_COMMON_H
#define API_RENDER_SHADERS_COMMON_CORE_COMPATIBILITY_COMMON_H

// Math types compatibility with c/c++
#ifndef VULKAN
#include <cstdint>

#include <base/math/matrix.h>
#include <base/math/vector.h>
#include <base/namespace.h>
using uint = uint32_t;
using ivec4 = int32_t[4]; // Vector4
using vec2 = BASE_NS::Math::Vec2;
using vec3 = BASE_NS::Math::Vec3;
using vec4 = BASE_NS::Math::Vec4;
using uvec2 = BASE_NS::Math::UVec2;
using uvec3 = BASE_NS::Math::UVec3;
using uvec4 = BASE_NS::Math::UVec4;
using mat4 = BASE_NS::Math::Mat4X4;
#endif

// Compatibility macros/constants to handle top-left / bottom-left differences between Vulkan/OpenGL(ES)
#ifdef VULKAN
precision highp float;
precision highp int;
// CORE RELAXED PRECISION
#define CORE_RELAXEDP mediump

#define CORE_BACKEND_TYPE_SPEC_ID 256
// 0 = vulkan  (NDC top-left origin, 0 - 1 depth)
// 1 = gl/gles (bottom-left origin, -1 - 1 depth)
layout(constant_id = CORE_BACKEND_TYPE_SPEC_ID) const uint CORE_BACKEND_TYPE = 0;

#define CORE_BACKEND_TYPE_MASK 0xf
#define CORE_BACKEND_TRANSFORM_MASK 0xf0
#define CORE_BACKEND_TRANSFORM_OFFSET (1 << 4)
#define CORE_BACKEND_TRANSFORM_0 (1 << 4)
#define CORE_BACKEND_TRANSFORM_90 (1 << 5)
#define CORE_BACKEND_TRANSFORM_180 (1 << 6)
#define CORE_BACKEND_TRANSFORM_270 (1 << 7)

// change from top-left NDC origin (vulkan) to bottom-left NDC origin (opengl)
// Our input data (post-projection / NDC coordinates) are in vulkan top-left.
// this constant_id will change to a normal uniform in GL/GLES and will change based on rendertarget.
layout(constant_id = 257) const float CORE_FLIP_NDC = 1.0;

#define CORE_VERTEX_OUT(a)                                                                                           \
    {                                                                                                                \
        gl_Position = (a);                                                                                           \
        if ((CORE_BACKEND_TYPE & CORE_BACKEND_TYPE_MASK) != 0) {                                                     \
            gl_Position = vec4(                                                                                      \
                gl_Position.x, gl_Position.y * CORE_FLIP_NDC, (gl_Position.z * 2.0 - gl_Position.w), gl_Position.w); \
        }                                                                                                            \
    }

vec2 GetFragCoordUv(vec2 fragCoord, vec2 inverseTexelSize)
{
    vec2 uv = fragCoord * inverseTexelSize;
    if (CORE_FLIP_NDC < 0.0) {
        uv = vec2(uv.x, 1.0 - uv.y);
    }
    return uv;
}

// out uv, gl_FragCoord, inverse tex size (i.e. texel size)
#define CORE_GET_FRAGCOORD_UV(uv, fc, its) (uv = GetFragCoordUv(fc, its))

void PreRotateSurface(inout vec2 clipXy)
{
    if ((CORE_BACKEND_TYPE & CORE_BACKEND_TYPE_MASK) == 0) {
        const uint rotate = CORE_BACKEND_TYPE & CORE_BACKEND_TRANSFORM_MASK;
        if (rotate == CORE_BACKEND_TRANSFORM_90) {
            const mat2 m = mat2(0.0, 1.0, -1.0, 0.0);
            clipXy = m * clipXy;
        } else if (rotate == CORE_BACKEND_TRANSFORM_180) {
            const mat2 m = mat2(-1.0, 0.0, 0.0, -1.0);
            clipXy = m * clipXy;
        } else if (rotate == CORE_BACKEND_TRANSFORM_270) {
            const mat2 m = mat2(0.0, -1.0, 1.0, 0.0);
            clipXy = m * clipXy;
        }
    }
}

#define CORE_PRE_ROTATE_SURFACE(clipXy) (PreRotateSurface(clipXy))

#else

static constexpr uint32_t CORE_BACKEND_TYPE_SPEC_ID { 256U };
static constexpr uint32_t CORE_BACKEND_TRANSFORM_OFFSET { 4U };

#endif

#endif // API_RENDER_SHADERS_COMMON_CORE_COMPATIBILITY_COMMON_H
