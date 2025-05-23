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

#ifndef META_BASE_NAMESPACE_H
#define META_BASE_NAMESPACE_H

/**
 * @brief Meta object library namespace
 */
#define META_NS Meta

/**
 * @brief Start Meta object library namespace
 */
#define META_BEGIN_NAMESPACE() namespace META_NS {
/**
 * @brief End Meta object library namespace
 */
#define META_END_NAMESPACE() } // namespace Meta

#define META_BEGIN_INTERNAL_NAMESPACE() \
    namespace META_NS {                 \
    namespace Internal {

#define META_END_INTERNAL_NAMESPACE() \
    } /* namespace Internal */        \
    } // namespace Meta

#endif
