/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IPluginRegister;
CORE_END_NAMESPACE()

namespace {
static CORE_NS::IPluginRegister* gPluginRegistry { nullptr };
} // namespace

#if CORE_DYNAMIC == 1
CORE_NS::IPluginRegister& (*CORE_NS::GetPluginRegister)() = nullptr;
#else
CORE_BEGIN_NAMESPACE()
IPluginRegister& GetPluginRegister()
{
    return *gPluginRegistry;
}
CORE_END_NAMESPACE()
#endif

extern "C" void InitRegistry(CORE_NS::IPluginRegister& pluginRegistry)
{
    // Initializing dynamic plugin.
    // Pluginregistry access via the provided registry instance which is saved here.
    gPluginRegistry = &pluginRegistry;

#if CORE_DYNAMIC == 1
    CORE_NS::GetPluginRegister = []() -> CORE_NS::IPluginRegister& { return *gPluginRegistry; };
#endif
}
