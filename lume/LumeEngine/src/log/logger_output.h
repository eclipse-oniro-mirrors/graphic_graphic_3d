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

#ifndef CORE_LOG_LOGGER_OUTPUT_H
#define CORE_LOG_LOGGER_OUTPUT_H

#include <base/containers/string_view.h>
#include <core/log.h>
#include <core/namespace.h>

#include <string_view>
#include <ostream>

CORE_BEGIN_NAMESPACE()
/** Console output */
ILogger::IOutput::Ptr CreateLoggerConsoleOutput();

/** Debug output (On windows, this writes to output in visual studio) */
ILogger::IOutput::Ptr CreateLoggerDebugOutput();

/** File output */
ILogger::IOutput::Ptr CreateLoggerFileOutput(const BASE_NS::string_view filename);
CORE_END_NAMESPACE()

namespace LoggerUtils {
    /* Gets the filename part from the path. */
    std::string_view GetFilename(std::string_view path);
    /* Print time stamp to stream */
    void PrintTimeStamp(std::ostream& outputStream);
} // namespace LoggerUtils

#endif // CORE_LOG_LOGGER_OUTPUT_H
