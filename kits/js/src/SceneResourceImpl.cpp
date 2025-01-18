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

#include "SceneResourceImpl.h"

#include "BaseObjectJS.h"
#include "SceneJS.h"

SceneResourceImpl::SceneResourceImpl(SceneResourceType type) : type_(type)
{
    LOG_V("SceneResourceImpl ++");
}
SceneResourceImpl::~SceneResourceImpl()
{
    LOG_V("SceneResourceImpl --");
}

void SceneResourceImpl::RegisterEnums(NapiApi::Object exports)
{
    napi_value v;
    NapiApi::Object SceneResourceType(exports.GetEnv());

#define DECL_ENUM(enu, x)                                           \
    {                                                               \
        napi_create_uint32(enu.GetEnv(), SceneResourceType::x, &v); \
        enu.Set(#x, v);                                             \
    }
    DECL_ENUM(SceneResourceType, UNKNOWN);
    DECL_ENUM(SceneResourceType, NODE);
    DECL_ENUM(SceneResourceType, ENVIRONMENT);
    DECL_ENUM(SceneResourceType, MATERIAL);
    DECL_ENUM(SceneResourceType, MESH);
    DECL_ENUM(SceneResourceType, ANIMATION);
    DECL_ENUM(SceneResourceType, SHADER);
    DECL_ENUM(SceneResourceType, IMAGE);
#undef DECL_ENUM

    exports.Set("SceneResourceType", SceneResourceType);
}

void SceneResourceImpl::GetPropertyDescs(BASE_NS::vector<napi_property_descriptor>& props)
{
    props.push_back(
        TROGetSetProperty<BASE_NS::string, SceneResourceImpl, &SceneResourceImpl::GetName, &SceneResourceImpl::SetName>(
            "name"));
    props.push_back(TROGetProperty<NapiApi::Object, SceneResourceImpl, &SceneResourceImpl::GetUri>("uri"));
    props.push_back(
        TROGetProperty<BASE_NS::string, SceneResourceImpl, &SceneResourceImpl::GetObjectType>("resourceType"));
    props.push_back(
        MakeTROMethod<NapiApi::FunctionContext<>, SceneResourceImpl, &SceneResourceImpl::Dispose>("destroy"));
}

void* SceneResourceImpl::GetInstanceImpl(uint32_t id)
{
    if (id == SceneResourceImpl::ID)
        return this;
    return nullptr;
}
bool SceneResourceImpl::validateSceneRef()
{
    if (scene_.GetObject()) {
        // scene reference still valid.
        // so resource should still be valid.
        return true;
    }
    // scene reference lost, so the resource should be disposed
    LOG_V("Scene reference lost.");
    return false;
}

napi_value SceneResourceImpl::Dispose(NapiApi::FunctionContext<>& ctx)
{
    // Dispose of the native object. (makes the js object invalid)
    if (TrueRootObject* instance = GetThisRootObject(ctx)) {
        // see if we have "scenejs" as ext (prefer one given as argument)
        napi_status stat;
        SceneJS* ptr { nullptr };
        NapiApi::FunctionContext<NapiApi::Object> args(ctx);
        if (args) {
            if (NapiApi::Object obj = args.Arg<0>()) {
                if (napi_value ext = obj.Get("SceneJS")) {
                    stat = napi_get_value_external(ctx.GetEnv(), ext, (void**)&ptr);
                }
            }
        }
        if (!ptr) {
            NapiApi::Object obj = scene_.GetObject();
            if (obj) {
                auto* tro = obj.Native<TrueRootObject>();
                if (tro) {
                    ptr = static_cast<SceneJS*>(tro->GetInstanceImpl(SceneJS::ID));
                }
            }
        }
        instance->DisposeNative(ptr);
    }
    scene_.Reset();
    uri_.Reset();
    return ctx.GetUndefined();
}

napi_value SceneResourceImpl::GetObjectType(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    uint32_t type = -1; // return -1 if the resource does not exist anymore
    if (GetThisNativeObject(ctx)) {
        type = type_;
    }
    return ctx.GetNumber(type);
}

napi_value SceneResourceImpl::GetName(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    BASE_NS::string name;
    if (auto named = interface_pointer_cast<META_NS::INamed>(GetThisNativeObject(ctx))) {
        name = named->Name()->GetValue();
    } else if (auto node = interface_pointer_cast<META_NS::IObject>(GetThisNativeObject(ctx))) {
        name = node->GetName();
    }
    return ctx.GetString(name);
}
void SceneResourceImpl::SetName(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }
    BASE_NS::string name = ctx.Arg<0>();
    if (auto named = interface_pointer_cast<META_NS::INamed>(GetThisNativeObject(ctx))) {
        named->Name()->SetValue(name);
    } else {
        LOG_E("renaming resource not implemented, trying to name (%d) to '%s'", type_, name.c_str());
    }
}
void SceneResourceImpl::SetUri(NapiApi::StrongRef uri)
{
    if (!validateSceneRef()) {
        return;
    }
    uri_ = BASE_NS::move(uri);
}
napi_value SceneResourceImpl::GetUri(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }
    return uri_.GetValue();
}
