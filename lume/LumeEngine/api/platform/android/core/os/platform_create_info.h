/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef API_CORE_OS_ANDROID_PLATFORM_CREATE_INFO_H
#define API_CORE_OS_ANDROID_PLATFORM_CREATE_INFO_H

#ifdef __ANDROID__
#include <jni.h>

#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
// NOTE: solve includes in IPlatform so that it wont print android and win data to same place
/** @ingroup group_os_android_platformcreateinfo */
/** Platform create info */
struct PlatformCreateInfo {
    /** JNI environment pointer */
    JNIEnv* env { nullptr };
    /** Android context object */
    jobject context { nullptr };
};
CORE_END_NAMESPACE()

#endif // __ANDROID__

#endif // API_CORE_OS_ANDROID_PLATFORM_CREATE_INFO_H