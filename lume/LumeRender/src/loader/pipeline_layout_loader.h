/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef LOADER_PIPELINE_LAYOUT_LOADER_H
#define LOADER_PIPELINE_LAYOUT_LOADER_H

#include <base/containers/string.h>
#include <core/namespace.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>

CORE_BEGIN_NAMESPACE()
class IFileManager;
CORE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()

/** Pipeline layout loader.
 * A class that can be used to load pipeline layout from json structure.
 */
class PipelineLayoutLoader final {
public:
    /** Describes result of the parsing operation. */
    struct LoadResult {
        LoadResult() = default;
        explicit LoadResult(const BASE_NS::string& error) : success(false), error(error) {}

        /** Indicates, whether the parsing operation is successful. */
        bool success { true };

        /** In case of parsing error, contains the description of the error. */
        BASE_NS::string error;
    };

    /** Retrieve uri of the pipeline layout.
     * @return String view to uri of the pipeline layout.
     */
    BASE_NS::string_view GetUri() const;

    /** Retrieve pipeline layout.
     * @return A pipeline layout, as defined in the json file.
     */
    const PipelineLayout& GetPipelineLayout() const;

    /** Loads pipeline layout from json string.
     * @param jsonString A string containing valid json as content.
     * @return A structure containing result for the parsing operation.
     */
    LoadResult Load(BASE_NS::string_view jsonString);

    /** Loads pipeline layout from given uri, using file manager.
     * @param fileManager A file manager to access the file in given uri.
     * @param uri Uri to json file.
     * @return A structure containing result for the parsing operation.
     */
    LoadResult Load(CORE_NS::IFileManager& fileManager, BASE_NS::string_view uri);

private:
    PipelineLayout pipelineLayout_;
    BASE_NS::string uri_;
};
RENDER_END_NAMESPACE()

#endif // LOADER_PIPELINE_LAYOUT_LOADER_H
