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

#ifndef CORE_OS_OHOS_PLATFORM_OHOS_H
#define CORE_OS_OHOS_PLATFORM_OHOS_H
#include <memory>
#include <platform/ohos/core/os/intf_platform.h>

#include <base/containers/string.h>
#include <core/namespace.h>

namespace OHOS::Global::Resource {
class ResourceManager;
}

CORE_BEGIN_NAMESPACE()
struct PlatformData {
    /** Core root path */
    BASE_NS::string coreRootPath = "./";
    /** Core plugin path */
    BASE_NS::string corePluginPath = "./";
    /** Application root path */
    BASE_NS::string appRootPath = "./";
    /** Application plugin path */
    BASE_NS::string appPluginPath = "./";

    /** Hap Info */
    BASE_NS::string hapPath = "./";
    BASE_NS::string bundleName = "./";
    BASE_NS::string moduleName = "./";
    std::shared_ptr<OHOS::Global::Resource::ResourceManager> resourceManager = nullptr;
};

/** Interface for platform-specific functions. */
class PlatformOHOS final : public IPlatform {
public:
    explicit PlatformOHOS(PlatformCreateInfo const& createInfo);
    ~PlatformOHOS() override;

    const PlatformData& GetPlatformData() const override;
    BASE_NS::string RegisterDefaultPaths(IFileManager& fileManage) const override;
    void RegisterPluginLocations(IPluginRegister& registry) override;

private:
    PlatformData plat_;
};

CORE_END_NAMESPACE()

#endif // CORE_OS_OHOS_PLATFORM_OHOS_H
