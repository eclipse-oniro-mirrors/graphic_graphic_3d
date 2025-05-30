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

#ifndef META_INTERFACE_INAMED_H
#define META_INTERFACE_INAMED_H

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(INamed, "e5f8f341-94b5-479e-b124-ab53150ad1aa")

/**
 * @brief The INamed interface defines an interface for classes which
 *        have a user-settable Name property.
 *
 *        If the implementing class also implements IObject, it should
 *        override IObject::GetName to return the value of Name property.
 */
class INamed : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INamed)
public:
    /**
     * @brief Name of the object.
     */
    META_PROPERTY(BASE_NS::string, Name)
};

/// Separate name for object
class IObjectName : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IObjectName, "bca6bd12-ef83-4999-b3ca-f3ae91626673")
public:
    virtual BASE_NS::string GetName() const = 0;
    virtual bool SetName(BASE_NS::string_view name) = 0;
};

META_END_NAMESPACE()

#endif // META_INTERFACE_INAMED_H
