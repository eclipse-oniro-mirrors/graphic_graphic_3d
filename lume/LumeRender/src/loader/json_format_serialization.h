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

#ifndef LOADER_JSON_FORMAT_SERIALIZATION_H
#define LOADER_JSON_FORMAT_SERIALIZATION_H

#include <base/util/formats.h>
#include <render/namespace.h>

#include "json_util.h"

BASE_BEGIN_NAMESPACE()
// clang-format off
CORE_JSON_SERIALIZE_ENUM(BASE_NS::Format,
    {
        { BASE_NS::Format::BASE_FORMAT_UNDEFINED, "undefined" },
        { BASE_NS::Format::BASE_FORMAT_R4G4_UNORM_PACK8, "r4g4_unorm_pack8" },
        { BASE_NS::Format::BASE_FORMAT_R4G4B4A4_UNORM_PACK16, "r4g4b4a4_unorm_pack16" },
        { BASE_NS::Format::BASE_FORMAT_B4G4R4A4_UNORM_PACK16, "b4g4r4a4_unorm_pack16" },
        { BASE_NS::Format::BASE_FORMAT_R5G6B5_UNORM_PACK16, "r5g6b5_unorm_pack16" },
        { BASE_NS::Format::BASE_FORMAT_B5G6R5_UNORM_PACK16, "b5g6r5_unorm_pack16" },
        { BASE_NS::Format::BASE_FORMAT_R5G5B5A1_UNORM_PACK16, "r5g5b5a1_unorm_pack16" },
        { BASE_NS::Format::BASE_FORMAT_B5G5R5A1_UNORM_PACK16, "b5g5r5a1_unorm_pack16" },
        { BASE_NS::Format::BASE_FORMAT_A1R5G5B5_UNORM_PACK16, "a1r5g5b5_unorm_pack16" },
        { BASE_NS::Format::BASE_FORMAT_R8_UNORM, "r8_unorm" },
        { BASE_NS::Format::BASE_FORMAT_R8_SNORM, "r8_snorm" },
        { BASE_NS::Format::BASE_FORMAT_R8_USCALED, "r8_uscaled" },
        { BASE_NS::Format::BASE_FORMAT_R8_SSCALED, "r8_sscaled" },
        { BASE_NS::Format::BASE_FORMAT_R8_UINT, "r8_uint" },
        { BASE_NS::Format::BASE_FORMAT_R8_SINT, "r8_sint" },
        { BASE_NS::Format::BASE_FORMAT_R8_SRGB, "r8_srgb" },
        { BASE_NS::Format::BASE_FORMAT_R8G8_UNORM, "r8g8_unorm" },
        { BASE_NS::Format::BASE_FORMAT_R8G8_SNORM, "r8g8_snorm" },
        { BASE_NS::Format::BASE_FORMAT_R8G8_USCALED, "r8g8_uscaled" },
        { BASE_NS::Format::BASE_FORMAT_R8G8_SSCALED, "r8g8_sscaled" },
        { BASE_NS::Format::BASE_FORMAT_R8G8_UINT, "r8g8_uint" },
        { BASE_NS::Format::BASE_FORMAT_R8G8_SINT, "r8g8_sint" },
        { BASE_NS::Format::BASE_FORMAT_R8G8_SRGB, "r8g8_srgb" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8_UNORM, "r8g8b8_unorm" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8_SNORM, "r8g8b8_snorm" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8_USCALED, "r8g8b8_uscaled" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8_SSCALED, "r8g8b8_sscaled" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8_UINT, "r8g8b8_uint" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8_SINT, "r8g8b8_sint" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8_SRGB, "r8g8b8_srgb" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8_UNORM, "b8g8r8_unorm" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8_SNORM, "b8g8r8_snorm" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8_USCALED, "b8g8r8_uscaled" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8_SSCALED, "b8g8r8_sscaled" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8_UINT, "b8g8r8_uint" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8_SINT, "b8g8r8_sint" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8_SRGB, "b8g8r8_srgb" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8A8_UNORM, "r8g8b8a8_unorm" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SNORM, "r8g8b8a8_snorm" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8A8_USCALED, "r8g8b8a8_uscaled" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SSCALED, "r8g8b8a8_sscaled" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8A8_UINT, "r8g8b8a8_uint" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SINT, "r8g8b8a8_sint" },
        { BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SRGB, "r8g8b8a8_srgb" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8A8_UNORM, "b8g8r8a8_unorm" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8A8_SNORM, "b8g8r8a8_snorm" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8A8_USCALED, "b8g8r8a8_uscaled" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8A8_SSCALED, "b8g8r8a8_sscaled" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8A8_UINT, "b8g8r8a8_uint" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8A8_SINT, "b8g8r8a8_sint" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8A8_SRGB, "b8g8r8a8_srgb" },
        { BASE_NS::Format::BASE_FORMAT_A8B8G8R8_UNORM_PACK32, "a8b8g8r8_unorm_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A8B8G8R8_SNORM_PACK32, "a8b8g8r8_snorm_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A8B8G8R8_USCALED_PACK32, "a8b8g8r8_uscaled_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A8B8G8R8_SSCALED_PACK32, "a8b8g8r8_sscaled_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A8B8G8R8_UINT_PACK32, "a8b8g8r8_uint_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A8B8G8R8_SINT_PACK32, "a8b8g8r8_sint_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A8B8G8R8_SRGB_PACK32, "a8b8g8r8_srgb_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A2R10G10B10_UNORM_PACK32, "a2r10g10b10_unorm_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A2R10G10B10_SNORM_PACK32, "a2r10g10b10_snorm_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A2R10G10B10_USCALED_PACK32, "a2r10g10b10_uscaled_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A2R10G10B10_SSCALED_PACK32, "a2r10g10b10_sscaled_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A2R10G10B10_UINT_PACK32, "a2r10g10b10_uint_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A2R10G10B10_SINT_PACK32, "a2r10g10b10_sint_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A2B10G10R10_UNORM_PACK32, "a2b10g10r10_unorm_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A2B10G10R10_SNORM_PACK32, "a2b10g10r10_snorm_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A2B10G10R10_USCALED_PACK32, "a2b10g10r10_uscaled_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A2B10G10R10_SSCALED_PACK32, "a2b10g10r10_sscaled_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A2B10G10R10_UINT_PACK32, "a2b10g10r10_uint_pack32" },
        { BASE_NS::Format::BASE_FORMAT_A2B10G10R10_SINT_PACK32, "a2b10g10r10_sint_pack32" },
        { BASE_NS::Format::BASE_FORMAT_R16_UNORM, "r16_unorm" },
        { BASE_NS::Format::BASE_FORMAT_R16_SNORM, "r16_snorm" },
        { BASE_NS::Format::BASE_FORMAT_R16_USCALED, "r16_uscaled" },
        { BASE_NS::Format::BASE_FORMAT_R16_SSCALED, "r16_sscaled" },
        { BASE_NS::Format::BASE_FORMAT_R16_UINT, "r16_uint" },
        { BASE_NS::Format::BASE_FORMAT_R16_SINT, "r16_sint" },
        { BASE_NS::Format::BASE_FORMAT_R16_SFLOAT, "r16_sfloat" },
        { BASE_NS::Format::BASE_FORMAT_R16G16_UNORM, "r16g16_unorm" },
        { BASE_NS::Format::BASE_FORMAT_R16G16_UNORM, "r16g16_unorm" },
        { BASE_NS::Format::BASE_FORMAT_R16G16_SNORM, "r16g16_snorm" },
        { BASE_NS::Format::BASE_FORMAT_R16G16_USCALED, "r16g16_uscaled" },
        { BASE_NS::Format::BASE_FORMAT_R16G16_SSCALED, "r16g16_sscaled" },
        { BASE_NS::Format::BASE_FORMAT_R16G16_UINT, "r16g16_uint" },
        { BASE_NS::Format::BASE_FORMAT_R16G16_SINT, "r16g16_sint" },
        { BASE_NS::Format::BASE_FORMAT_R16G16_SFLOAT, "r16g16_sfloat" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16_UNORM, "r16g16b16_unorm" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16_SNORM, "r16g16b16_snorm" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16_USCALED, "r16g16b16_uscaled" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16_SSCALED, "r16g16b16_sscaled" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16_UINT, "r16g16b16_uint" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16_SINT, "r16g16b16_sint" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16_SFLOAT, "r16g16b16_sfloat" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16A16_UNORM, "r16g16b16a16_unorm" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16A16_SNORM, "r16g16b16a16_snorm" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16A16_USCALED, "r16g16b16a16_uscaled" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16A16_SSCALED, "r16g16b16a16_sscaled" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16A16_UINT, "r16g16b16a16_uint" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16A16_SINT, "r16g16b16a16_sint" },
        { BASE_NS::Format::BASE_FORMAT_R16G16B16A16_SFLOAT, "r16g16b16a16_sfloat" },
        { BASE_NS::Format::BASE_FORMAT_R32_UINT, "r32_uint" },
        { BASE_NS::Format::BASE_FORMAT_R32_SINT, "r32_sint" },
        { BASE_NS::Format::BASE_FORMAT_R32_SFLOAT, "r32_sfloat" },
        { BASE_NS::Format::BASE_FORMAT_R32G32_UINT, "r32g32_uint" },
        { BASE_NS::Format::BASE_FORMAT_R32G32_SINT, "r32g32_sint" },
        { BASE_NS::Format::BASE_FORMAT_R32G32_SFLOAT, "r32g32_sfloat" },
        { BASE_NS::Format::BASE_FORMAT_R32G32B32_UINT, "r32g32b32_uint" },
        { BASE_NS::Format::BASE_FORMAT_R32G32B32_SINT, "r32g32b32_sint" },
        { BASE_NS::Format::BASE_FORMAT_R32G32B32_SFLOAT, "r32g32b32_sfloat" },
        { BASE_NS::Format::BASE_FORMAT_R32G32B32A32_UINT, "r32g32b32a32_uint" },
        { BASE_NS::Format::BASE_FORMAT_R32G32B32A32_SINT, "r32g32b32a32_sint" },
        { BASE_NS::Format::BASE_FORMAT_R32G32B32A32_SFLOAT, "r32g32b32a32_sfloat" },
        { BASE_NS::Format::BASE_FORMAT_R64_UINT, "r64_uint" },
        { BASE_NS::Format::BASE_FORMAT_R64_SINT, "r64_sint" },
        { BASE_NS::Format::BASE_FORMAT_R64_SFLOAT, "r64_sfloat" },
        { BASE_NS::Format::BASE_FORMAT_R64G64_UINT, "r64g64_uint" },
        { BASE_NS::Format::BASE_FORMAT_R64G64_SINT, "r64g64_sint" },
        { BASE_NS::Format::BASE_FORMAT_R64G64_SFLOAT, "r64g64_sfloat" },
        { BASE_NS::Format::BASE_FORMAT_R64G64B64_UINT, "r64g64b64_uint" },
        { BASE_NS::Format::BASE_FORMAT_R64G64B64_SINT, "r64g64b64_sint" },
        { BASE_NS::Format::BASE_FORMAT_R64G64B64_SFLOAT, "r64g64b64_sfloat" },
        { BASE_NS::Format::BASE_FORMAT_R64G64B64A64_UINT, "r64g64b64a64_uint" },
        { BASE_NS::Format::BASE_FORMAT_R64G64B64A64_SINT, "r64g64b64a64_sint" },
        { BASE_NS::Format::BASE_FORMAT_R64G64B64A64_SFLOAT, "r64g64b64a64_sfloat" },
        { BASE_NS::Format::BASE_FORMAT_B10G11R11_UFLOAT_PACK32, "b10g11r11_ufloat_pack32" },
        { BASE_NS::Format::BASE_FORMAT_E5B9G9R9_UFLOAT_PACK32, "e5b9g9r9_ufloat_pack32" },
        { BASE_NS::Format::BASE_FORMAT_D16_UNORM, "d16_unorm" },
        { BASE_NS::Format::BASE_FORMAT_X8_D24_UNORM_PACK32, "x8_d24_unorm_pack32" },
        { BASE_NS::Format::BASE_FORMAT_D32_SFLOAT, "d32_sfloat" },
        { BASE_NS::Format::BASE_FORMAT_S8_UINT, "s8_uint" },
        { BASE_NS::Format::BASE_FORMAT_D24_UNORM_S8_UINT, "d24_unorm_s8_uint" },
        { BASE_NS::Format::BASE_FORMAT_BC1_RGB_UNORM_BLOCK, "bc1_rgb_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_BC1_RGB_SRGB_BLOCK, "bc1_rgb_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_BC1_RGBA_UNORM_BLOCK, "bc1_rgba_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_BC1_RGBA_SRGB_BLOCK, "bc1_rgba_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_BC2_UNORM_BLOCK, "bc2_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_BC2_SRGB_BLOCK, "bc2_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_BC3_UNORM_BLOCK, "bc3_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_BC3_SRGB_BLOCK, "bc3_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_BC4_UNORM_BLOCK, "bc4_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_BC4_SNORM_BLOCK, "bc4_snorm_block" },
        { BASE_NS::Format::BASE_FORMAT_BC5_UNORM_BLOCK, "bc5_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_BC5_SNORM_BLOCK, "bc5_snorm_block" },
        { BASE_NS::Format::BASE_FORMAT_BC6H_UFLOAT_BLOCK, "bc6h_ufloat_block" },
        { BASE_NS::Format::BASE_FORMAT_BC6H_SFLOAT_BLOCK, "bc6h_sfloat_block" },
        { BASE_NS::Format::BASE_FORMAT_BC7_UNORM_BLOCK, "bc7_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_BC7_SRGB_BLOCK, "bc7_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, "etc2_r8g8b8_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, "etc2_r8g8b8_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, "etc2_r8g8b8a1_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, "etc2_r8g8b8a1_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, "etc2_r8g8b8a8_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, "etc2_r8g8b8a8_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_EAC_R11_UNORM_BLOCK, "eac_r11_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_EAC_R11_SNORM_BLOCK, "eac_r11_snorm_block" },
        { BASE_NS::Format::BASE_FORMAT_EAC_R11G11_UNORM_BLOCK, "eac_r11g11_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_EAC_R11G11_SNORM_BLOCK, "eac_r11g11_snorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_4x4_UNORM_BLOCK, "astc_4x4_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_4x4_SRGB_BLOCK, "astc_4x4_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_5x4_UNORM_BLOCK, "astc_5x4_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_5x4_SRGB_BLOCK, "astc_5x4_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_5x5_UNORM_BLOCK, "astc_5x5_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_5x5_SRGB_BLOCK, "astc_5x5_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_6x5_UNORM_BLOCK, "astc_6x5_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_6x5_SRGB_BLOCK, "astc_6x5_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_6x6_UNORM_BLOCK, "astc_6x6_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_6x6_SRGB_BLOCK, "astc_6x6_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_8x5_UNORM_BLOCK, "astc_8x5_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_8x5_SRGB_BLOCK, "astc_8x5_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_8x6_UNORM_BLOCK, "astc_8x6_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_8x6_SRGB_BLOCK, "astc_8x6_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_8x8_UNORM_BLOCK, "astc_8x8_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_8x8_SRGB_BLOCK, "astc_8x8_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_10x5_UNORM_BLOCK, "astc_10x5_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_10x5_SRGB_BLOCK, "astc_10x5_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_10x6_UNORM_BLOCK, "astc_10x6_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_10x6_SRGB_BLOCK, "astc_10x6_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_10x8_UNORM_BLOCK, "astc_10x8_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_10x8_SRGB_BLOCK, "astc_10x8_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_10x10_UNORM_BLOCK, "astc_10x10_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_10x10_SRGB_BLOCK, "astc_10x10_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_12x10_UNORM_BLOCK, "astc_12x10_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_12x10_SRGB_BLOCK, "astc_12x10_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_12x12_UNORM_BLOCK, "astc_12x12_unorm_block" },
        { BASE_NS::Format::BASE_FORMAT_ASTC_12x12_SRGB_BLOCK, "astc_12x12_srgb_block" },
        { BASE_NS::Format::BASE_FORMAT_G8B8G8R8_422_UNORM, "G8B8G8R8_422_UNORM" },
        { BASE_NS::Format::BASE_FORMAT_B8G8R8G8_422_UNORM, "B8G8R8G8_422_UNORM" },
        { BASE_NS::Format::BASE_FORMAT_G8_B8_R8_3PLANE_420_UNORM, "G8_B8_R8_3PLANE_420_UNORM" },
        { BASE_NS::Format::BASE_FORMAT_G8_B8R8_2PLANE_420_UNORM, "G8_B8R8_2PLANE_420_UNORM" },
        { BASE_NS::Format::BASE_FORMAT_G8_B8_R8_3PLANE_422_UNORM, "G8_B8_R8_3PLANE_422_UNORM" },
        { BASE_NS::Format::BASE_FORMAT_G8_B8R8_2PLANE_422_UNORM, "G8_B8R8_2PLANE_422_UNORM" },
        { BASE_NS::Format::BASE_FORMAT_G8_B8_R8_3PLANE_444_UNORM, "G8_B8_R8_3PLANE_444_UNORM" },
        { BASE_NS::Format::BASE_FORMAT_MAX_ENUM, nullptr },
    })
// clang-format on
BASE_END_NAMESPACE()
#endif // LOADER_JSON_FORMAT_SERIALIZATION_H