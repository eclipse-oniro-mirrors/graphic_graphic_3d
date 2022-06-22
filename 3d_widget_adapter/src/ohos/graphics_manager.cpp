/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "graphics_manager.h"
#include "i_engine.h"
#include "platform_data.h"
#include "3d_widget_adapter_log.h"

namespace OHOS::Render3D {
PlatformData GraphicsManager::GetPlatformData() const
{
    #define TO_STRING(name) #name
    #define PLATFORM_PATH_NAME(name) TO_STRING(name)
    PlatformData data {
        PLATFORM_PATH_NAME(PLATFORM_CORE_ROOT_PATH),
        PLATFORM_PATH_NAME(PLATFORM_APP_ROOT_PATH),
        PLATFORM_PATH_NAME(PLATFORM_APP_PLUGIN_PATH),
    };
    #undef TO_STRING
    #undef PLATFORM_PATH_NAME
    WIDGET_LOGD("platformdata %s, %s, %s,", data.coreRootPath_.c_str(),
        data.appRootPath_.c_str(), data.appPluginPath_.c_str());
    return data;
}

GraphicsManager& GraphicsManager::GetInstance()
{
    static GraphicsManager graphicsManager;
    return graphicsManager;
}

GraphicsManager::~GraphicsManager()
{}
} // namespace OHOS::Render3D
