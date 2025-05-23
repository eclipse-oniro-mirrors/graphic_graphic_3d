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

#ifndef META_INTERFACE_VALUE_H
#define META_INTERFACE_VALUE_H

#include <meta/interface/intf_any.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IValue, "23c3c0c7-9937-468c-9199-28f982b40fe5")

/// Interface for values
class IValue : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IValue)
public:
    /// Set value as given any (the any has to be compatible)
    virtual AnyReturnValue SetValue(const IAny& value) = 0;
    /// Get value as any or invalid any in case of error
    virtual const IAny& GetValue() const = 0;
    /// Check if given type id if compatible with this value
    virtual bool IsCompatible(const TypeId& id) const = 0;
};

META_INTERFACE_TYPE(META_NS::IValue)

META_END_NAMESPACE()

#endif
