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
#ifndef META_SRC_OBJECT_H
#define META_SRC_OBJECT_H

#include <meta/ext/object.h>

META_BEGIN_NAMESPACE()
namespace Internal {
class Object : public MetaObject {
    META_OBJECT(Object, ClassId::Object, MetaObject)
};
} // namespace Internal

META_END_NAMESPACE()

#endif
