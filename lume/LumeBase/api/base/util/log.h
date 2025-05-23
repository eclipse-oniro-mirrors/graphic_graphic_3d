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

#ifndef API_BASE_UTIL_LOG_H
#define API_BASE_UTIL_LOG_H

#ifndef NDEBUG
#include <cassert>
#define BASE_ASSERT(cond) assert(cond)
#define BASE_ASSERT_MSG(cond, msg) assert((cond) && (msg))

#include <cstdio>
#define BASE_LOG_E(...) ::fprintf(stderr, __VA_ARGS__)
#else
#define BASE_ASSERT(cond) ((void)0)
#define BASE_ASSERT_MSG(cond, msg) ((void)0)
#define BASE_LOG_E(...) ((void)0)
#endif

#endif // API_BASE_UTIL_LOG_H
