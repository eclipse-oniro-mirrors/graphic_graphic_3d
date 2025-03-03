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

#include "QuatProxy.h"

#include <napi_api.h>
QuatProxy::QuatProxy(napi_env env, META_NS::Property<BASE_NS::Math::Quat> prop) : ObjectPropertyProxy(prop)
{
    Create(env, "Quaternion");
    Hook("x");
    Hook("y");
    Hook("z");
    Hook("w");
    SyncGet();
}
QuatProxy::~QuatProxy()
{
    Reset();
}
void QuatProxy::UpdateLocalValues()
{
    // update local values. (runs in engine thread)
    value = META_NS::Property<BASE_NS::Math::Quat>(prop_)->GetValue();
}
void QuatProxy::UpdateRemoteValues()
{
    META_NS::Property<BASE_NS::Math::Quat>(prop_)->SetValue(value);
}
void QuatProxy::SetValue(const BASE_NS::Math::Quat& v)
{
    Lock();
    if (value != v) {
        value = v;
        ScheduleUpdate();
    }
    Unlock();
}
void QuatProxy::SetMemberValue(NapiApi::FunctionContext<>& cb, BASE_NS::string_view memb)
{
    NapiApi::FunctionContext<float> info(cb);
    if (info) {
        float val = info.Arg<0>();
        Lock();
        if ((memb == "x") && (val != value.x)) {
            value.x = val;
            ScheduleUpdate();
        } else if ((memb == "y") && (val != value.y)) {
            value.y = val;
            ScheduleUpdate();
        } else if ((memb == "z") && (val != value.z)) {
            value.z = val;
            ScheduleUpdate();
        } else if ((memb == "w") && (val != value.w)) {
            value.w = val;
            ScheduleUpdate();
        }
        Unlock();
    }
}
napi_value QuatProxy::GetMemberValue(const NapiApi::Env info, BASE_NS::string_view memb)
{
    float res;
    Lock();
    if (memb == "x") {
        res = value.x;
    } else if (memb == "y") {
        res = value.y;
    } else if (memb == "z") {
        res = value.z;
    } else if (memb == "w") {
        res = value.w;
    } else {
        // invalid member?
        Unlock();
        return info.GetUndefined();
    }
    Unlock();
    return info.GetNumber(res);
}

void QuatProxy::SetValue(NapiApi::Object obj)
{
    auto x = obj.Get<float>("x");
    auto y = obj.Get<float>("y");
    auto z = obj.Get<float>("z");
    auto w = obj.Get<float>("w");
    if (x.IsValid() && y.IsValid() && z.IsValid() && w.IsValid()) {
        SetValue({ x, y, z, w });
    }
}
