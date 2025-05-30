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

#ifndef CORE_IO_PROXY_FILESYSTEM_H
#define CORE_IO_PROXY_FILESYSTEM_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/io/intf_directory.h>
#include <core/io/intf_file.h>
#include <core/io/intf_file_system.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class FileManager;

/** File protocol.
 * Protocol implementation that uses given protocol as proxy to destination uri, for example app:// to point in to
 * application working directory.
 */
class ProxyFilesystem final : public IFilesystem {
public:
    ProxyFilesystem(FileManager& fileManager, BASE_NS::string_view destination);

    ProxyFilesystem() = delete;
    ProxyFilesystem(ProxyFilesystem const&) = delete;
    ProxyFilesystem& operator=(ProxyFilesystem const&) = delete;

    ~ProxyFilesystem() override = default;

    void RemoveSearchPath(BASE_NS::string_view destination);

    IDirectory::Entry GetEntry(BASE_NS::string_view path) override;
    IFile::Ptr OpenFile(BASE_NS::string_view path, IFile::Mode mode) override;
    IFile::Ptr CreateFile(BASE_NS::string_view path) override;
    bool DeleteFile(BASE_NS::string_view path) override;
    bool FileExists(BASE_NS::string_view path) const override;

    IDirectory::Ptr OpenDirectory(BASE_NS::string_view path) override;
    IDirectory::Ptr CreateDirectory(BASE_NS::string_view path) override;
    bool DeleteDirectory(BASE_NS::string_view path) override;
    bool DirectoryExists(BASE_NS::string_view path) const override;

    bool Rename(BASE_NS::string_view fromPath, BASE_NS::string_view toPath) override;

    BASE_NS::vector<BASE_NS::string> GetUriPaths(BASE_NS::string_view uri) const override;

    void AppendSearchPath(BASE_NS::string_view path);
    void PrependSearchPath(BASE_NS::string_view path);

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    FileManager& fileManager_;
    BASE_NS::vector<BASE_NS::string> destinations_;
};
CORE_END_NAMESPACE()

#endif // CORE_IO_PROXY_FILESYSTEM_H
