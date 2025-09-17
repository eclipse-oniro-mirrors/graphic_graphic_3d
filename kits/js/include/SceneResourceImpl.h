/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef NAMED_IMPL_H
#define NAMED_IMPL_H

#include <napi_api.h>

#include <base/containers/vector.h>

class SceneResourceImpl {
public:
    static constexpr uint32_t ID = 1;
    enum SceneResourceType {
        /**
         * The resource is an Unknow.
         */
        UNKNOWN = 0,
        /**
         * The resource is an Node.
         */
        NODE = 1,
        /**
         * The resource is an Environment.
         */
        ENVIRONMENT = 2,
        /**
         * The resource is a Material.
         */
        MATERIAL = 3,
        /**
         * The resource is a Mesh.
         */
        MESH = 4,
        /**
         * The resource is an Animation.
         */
        ANIMATION = 5,
        /**
         * The resource is a Shader.
         */
        SHADER = 6,

        /**
         * The resource is a Image.
         */
        IMAGE = 7,
        /**
         * The resource is a MeshResource.
         */
        MESH_RESOURCE = 8,
    };
    SceneResourceImpl(SceneResourceType type);
    virtual ~SceneResourceImpl();
    static void RegisterEnums(NapiApi::Object exports);

    void* GetInstanceImpl(uint32_t id);
    NapiApi::WeakRef GetSceneWeakRef();

protected:
    static void GetPropertyDescs(BASE_NS::vector<napi_property_descriptor>& props);

    void SetUri(NapiApi::StrongRef uri);
    napi_value GetObjectType(NapiApi::FunctionContext<>& ctx);
    napi_value GetName(NapiApi::FunctionContext<>& ctx);
    void SetName(NapiApi::FunctionContext<BASE_NS::string>& ctx);
    napi_value GetUri(NapiApi::FunctionContext<>& ctx);
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);

    // May contain Scene or RenderContext
    NapiApi::WeakRef scene_;
    NapiApi::StrongRef uri_;

    // returns false if owning scene has been destroyed.
    bool validateSceneRef();

private:
    SceneResourceType type_;
    BASE_NS::string name_; // Cache for name if object does not support naming
};
#endif
