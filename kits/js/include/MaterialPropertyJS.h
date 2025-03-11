/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef MATERIAL_PROPERTY_JS_H
#define MATERIAL_PROPERTY_JS_H

#include <meta/interface/intf_object.h>

#include <meta/interface/intf_object.h>

#include "BaseObjectJS.h"

class MaterialPropertyJS : public BaseObject<MaterialPropertyJS> {
public:
    static constexpr uint32_t ID = 35;
    static void Init(napi_env env, napi_value exports);
    MaterialPropertyJS(napi_env, napi_callback_info);
    ~MaterialPropertyJS() override;

private:
    void* GetInstanceImpl(uint32_t) override;
    napi_value Dispose(NapiApi::FunctionContext<>& ctx);
    void DisposeNative(void*) override;
    void Finalize(napi_env env) override;

    napi_value GetImage(NapiApi::FunctionContext<>& ctx);
    void SetImage(NapiApi::FunctionContext<NapiApi::Object>& ctx);
    napi_value GetFactor(NapiApi::FunctionContext<>& ctx);
    void SetFactor(NapiApi::FunctionContext<NapiApi::Object>& ctx);
};
#endif
