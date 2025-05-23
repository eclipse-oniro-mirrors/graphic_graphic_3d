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

#ifndef LOADER_SHADER_DATA_LOADER_H
#define LOADER_SHADER_DATA_LOADER_H

#include <utility>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <core/namespace.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

CORE_BEGIN_NAMESPACE()
class IFileManager;
CORE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()

/** Shader data loader.
 * A class that can be used to load all shader related data from shader json structure.
 */
class ShaderDataLoader final {
public:
    /** Describes result of the parsing operation. */
    struct LoadResult {
        LoadResult() = default;
        explicit LoadResult(BASE_NS::string error) : success(false), error(BASE_NS::move(error)) {}

        /** Indicates, whether the parsing operation is successful. */
        bool success { true };

        /** In case of parsing error, contains the description of the error. */
        BASE_NS::string error;
    };

    /** Uri of the loaded file.
     * @return Uri path.
     */
    BASE_NS::string_view GetUri() const;

    /** Base category for all shader variants.
     * @return Base category for shaders.
     */
    BASE_NS::string_view GetBaseCategory() const;

    /** Get all shader variants.
     * @return Array view of shader variants.
     */
    BASE_NS::array_view<const IShaderManager::ShaderVariant> GetShaderVariants() const;

    /** Loads shader state from given uri, using file manager.
     * @param fileManager A file manager to access the file in given uri.
     * @param uri Uri to json file.
     * @return A structure containing result for the parsing operation.
     */
    LoadResult Load(CORE_NS::IFileManager& fileManager, BASE_NS::string_view uri);

private:
    LoadResult Load(BASE_NS::string_view uri, BASE_NS::string&& jsonData);

    BASE_NS::string uri_;
    BASE_NS::string baseCategory_;

    BASE_NS::vector<IShaderManager::ShaderVariant> shaderVariants_;
};
CORE_END_NAMESPACE()

#endif // LOADER_SHADER_DATA_LOADER_H
