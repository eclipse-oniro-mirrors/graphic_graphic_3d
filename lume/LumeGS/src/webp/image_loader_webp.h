/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: WebP image loader factory function.
 * Author: Jarno Paananen
 * Create: 2020-06-17
 */
#ifndef PLUGIN_IMAGE_LOADER_WEBP_H
#define PLUGIN_IMAGE_LOADER_WEBP_H

#include <core/image/intf_image_loader_manager.h>
#include <core/namespace.h>

namespace WebPPlugin {
static const CORE_NS::IImageLoaderManager::ImageType IMAGE_TYPES[] = { { "image/webp", "webp" } };

CORE_NS::IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderWebP(CORE_NS::PluginToken);
} // namespace WebPPlugin

#endif //  PLUGIN_IMAGE_LOADER_WEBP_H