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

#ifndef PLUGIN_IMAGE_LOADER_JPG_H
#define PLUGIN_IMAGE_LOADER_JPG_H

#include <core/image/intf_image_loader_manager.h>
#include <core/namespace.h>

namespace JPGPlugin {
static const CORE_NS::IImageLoaderManager::ImageType IMAGE_TYPES[] = {     { "image/jpeg", "jpeg" },
    { "image/jpeg", "jpg" } };

CORE_NS::IImageLoaderManager::IImageLoader::Ptr CreateImageLoaderJPG(CORE_NS::PluginToken);
} // namespace JPGPlugin

#endif //  PLUGIN_IMAGE_LOADER_JPG_H