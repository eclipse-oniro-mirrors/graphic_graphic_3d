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

#ifndef CORE_OS_WINDOWS_LIBRARY_WINDOWS_H
#define CORE_OS_WINDOWS_LIBRARY_WINDOWS_H

#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <core/namespace.h>

#include "os/intf_library.h"

using HINSTANCE = struct HINSTANCE__*;
using HMODULE = HINSTANCE;

CORE_BEGIN_NAMESPACE()
struct IPlugin;
class LibraryWindows final : public ILibrary {
public:
    explicit LibraryWindows(const BASE_NS::string_view filename);
    ~LibraryWindows() override;

    IPlugin* GetPlugin() const override;

protected:
    void Destroy() override;

private:
    HMODULE libraryHandle_ { nullptr };
};
CORE_END_NAMESPACE()

#endif // CORE_OS_WINDOWS_LIBRARY_WINDOWS_H