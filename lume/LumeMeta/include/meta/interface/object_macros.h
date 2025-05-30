/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef META_INTERFACE_OBJECT_MACROS_H
#define META_INTERFACE_OBJECT_MACROS_H

#include <base/containers/atomics.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/intf_lifecycle.h>

/**
 * @brief Implement reference counting with ILifecycle support for destruction.
 */
#define META_IMPLEMENT_REF_COUNT()                                         \
    int32_t refcnt_ { 0 };                                                 \
    void Ref() override                                                    \
    {                                                                      \
        BASE_NS::AtomicIncrement(&refcnt_);                                \
    }                                                                      \
    void Unref() override                                                  \
    {                                                                      \
        if (BASE_NS::AtomicDecrement(&refcnt_) == 0) {                     \
            if (auto i = this->GetInterface(::META_NS::ILifecycle::UID)) { \
                static_cast<::META_NS::ILifecycle*>(i)->Destroy();         \
            }                                                              \
            delete this;                                                   \
        }                                                                  \
    }

#endif
