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

#ifndef LOADER_SHADER_STATE_LOADER_H
#define LOADER_SHADER_STATE_LOADER_H

#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

CORE_BEGIN_NAMESPACE()
class IFileManager;
CORE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()

/** Shader state loader.
 * A class that can be used to load shader (graphics) state data from a json.
 */
class ShaderStateLoader final {
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

    struct GraphicsStates {
        BASE_NS::vector<GraphicsState> states;
        BASE_NS::vector<IShaderManager::ShaderStateLoaderVariantData> variantData;
    };

    /** Retrieve uri of shader state.
     * @return String view to uri of shader state.
     */
    BASE_NS::string_view GetUri() const;

    /** Retrieve graphics state variant names.
     * @return Graphics state variant names, as defined in the json file.
     */
    BASE_NS::array_view<const IShaderManager::ShaderStateLoaderVariantData> GetGraphicsStateVariantData() const;

    /** Retrieve graphics states.
     * @return Graphics states, as defined in the json file.
     */
    BASE_NS::array_view<const GraphicsState> GetGraphicsStates() const;

    /** Loads shader state from given uri, using file manager.
     * @param fileManager A file manager to access the file in given uri.
     * @param uri Uri to json file.
     * @return A structure containing result for the parsing operation.
     */
    LoadResult Load(CORE_NS::IFileManager& fileManager, BASE_NS::string_view uri);

private:
    BASE_NS::string uri_;
    BASE_NS::vector<GraphicsState> graphicsStates_;
    BASE_NS::vector<IShaderManager::ShaderStateLoaderVariantData> graphicsStateVariantData_;
};
RENDER_END_NAMESPACE()

#endif // LOADER_SHADER_STATE_LOADER_H
