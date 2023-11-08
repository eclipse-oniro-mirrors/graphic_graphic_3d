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

#include <core/implementation_uids.h>
#include <core/io/intf_file_manager.h>
#include <core/io/intf_file_monitor.h>
#include <core/io/intf_filesystem_api.h>
#include <core/plugin/intf_class_factory.h>

#include "dev/file_monitor.h"
#include "io/file_manager.h"
#include "io/memory_filesystem.h"
#include "io/path_tools.h"
#include "io/rofs_filesystem.h"
#include "io/std_filesystem.h"
#include "util/string_util.h"

CORE_BEGIN_NAMESPACE();
namespace {
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::Uid;
using BASE_NS::vector;

class FileMonitorImpl final : public IFileMonitor {
public:
    const IInterface* GetInterface(const Uid& uid) const override;
    IInterface* GetInterface(const Uid& uid) override;
    void Ref() override;
    void Unref() override;

    void Initialize(IFileManager&) override;
    bool AddPath(const string_view path) override;
    bool RemovePath(const string_view path) override;
    void ScanModifications(vector<string>& added, vector<string>& removed, vector<string>& modified) override;

    FileMonitorImpl() = default;
    explicit FileMonitorImpl(IFileManager& manager)
    {
        Initialize(manager);
    }
    ~FileMonitorImpl();
    FileMonitor* fileMonitor_ { nullptr };
    uint32_t refCount_ { 0 };
};

class FilesystemApi final : public IFileSystemApi {
public:
    string basePath_;
    IFilesystem::Ptr rootFs_;
    FilesystemApi()
    {
        basePath_ = GetCurrentDirectory();
        rootFs_ = CreateStdFileSystem();
    }

    const IInterface* GetInterface(const Uid& uid) const override
    {
        return const_cast<FilesystemApi*>(this)->GetInterface(uid);
    }
    IInterface* GetInterface(const Uid& uid) override
    {
        if ((uid == IInterface::UID) || (uid == IClassFactory::UID)) {
            return this;
        }
        if (uid == IFileSystemApi::UID) {
            return this;
        }
        return nullptr;
    }
    void Ref() override {}
    void Unref() override {}

    IInterface::Ptr CreateInstance(const Uid& uid) override
    {
        if (uid == UID_FILE_MONITOR) {
            return IInterface::Ptr(new FileMonitorImpl());
        }
        if (uid == UID_FILE_MANAGER) {
            return IInterface::Ptr(new FileManager());
        }
        return IInterface::Ptr();
    }

    IFileManager::Ptr CreateFilemanager() override
    {
        return IInterface::Ptr(new FileManager());
    }
    IFileMonitor::Ptr CreateFilemonitor(IFileManager& manager) override
    {
        return IInterface::Ptr(new FileMonitorImpl(manager));
    }
    string ResolvePath(string_view inPathRaw)
    {
#if _WIN32
        string_view cur_drive, cur_path, cur_filename, cur_ext;
        SplitPath(basePath_, cur_drive, cur_path, cur_filename, cur_ext);

        if (inPathRaw.empty()) {
            return {};
        }
        // fix slashes. (just change \\ to /)
        string_view pathIn = inPathRaw;
        string tmp;
        if (pathIn.find("\\") != string_view::npos) {
            tmp = pathIn;
            StringUtil::FindAndReplaceAll(tmp, "\\", "/");
            pathIn = tmp;
        }

        string_view drive, path, filename, ext;
        SplitPath(pathIn, drive, path, filename, ext);
        string res = "/";
        if (drive.empty()) {
            // relative to current drive then
            res += cur_drive;
        } else {
            res += drive;
        }
        res += ":";
        string normalized_path;
        if (path.empty()) {
            return "";
        } else {
            if (path[0] != '/') {
                // relative path.
                normalized_path = NormalizePath(cur_path + path);
            } else {
                normalized_path = NormalizePath(path);
            }
        }
        if (normalized_path.empty()) {
            return "";
        }
        return res + normalized_path;
#else
        if (IsRelative(inPathRaw)) {
            return NormalizePath(basePath_ + inPathRaw);
        }
        return NormalizePath(inPathRaw);
#endif
    }

    IFilesystem::Ptr CreateStdFileSystem() override
    {
        return IFilesystem::Ptr(new StdFilesystem("/"));
    }

    IFilesystem::Ptr CreateStdFileSystem(string_view rootPathIn) override
    {
        string_view protocol, path;
        if (ParseUri(rootPathIn, protocol, path)) {
            if (protocol != "file") {
                return {};
            }
            rootPathIn = path;
        }
        auto rootPath = ResolvePath(rootPathIn);
        if (!rootPath.empty()) {
            auto entry = rootFs_->GetEntry(rootPath);
            if (entry.type == IDirectory::Entry::DIRECTORY) {
                return IFilesystem::Ptr(new StdFilesystem(rootPath));
            }
        }
        return {};
    }
    IFilesystem::Ptr CreateMemFileSystem() override
    {
        return IFilesystem::Ptr(new MemoryFilesystem());
    }
    IFilesystem::Ptr CreateROFilesystem(const void* const data, uint64_t size) override
    {
        return IFilesystem::Ptr { new RoFileSystem(data, static_cast<size_t>(size)) };
    }
};
} // namespace

const IInterface* FileMonitorImpl::GetInterface(const Uid& uid) const
{
    if (uid == IFileMonitor::UID) {
        return this;
    }
    return nullptr;
}

IInterface* FileMonitorImpl::GetInterface(const Uid& uid)
{
    if (uid == IFileMonitor::UID) {
        return this;
    }
    return nullptr;
}

void FileMonitorImpl::Ref()
{
    refCount_++;
}

void FileMonitorImpl::Unref()
{
    if (--refCount_ == 0) {
        delete this;
    }
}

FileMonitorImpl::~FileMonitorImpl()
{
    delete fileMonitor_;
}

void FileMonitorImpl::Initialize(IFileManager& manager)
{
    fileMonitor_ = new FileMonitor(manager);
}

bool FileMonitorImpl::AddPath(const string_view path)
{
    if (fileMonitor_) {
        return fileMonitor_->AddPath(path);
    }
    return false;
}

bool FileMonitorImpl::RemovePath(const string_view path)
{
    if (fileMonitor_) {
        return fileMonitor_->RemovePath(path);
    }
    return false;
}

void FileMonitorImpl::ScanModifications(vector<string>& added, vector<string>& removed, vector<string>& modified)
{
    added.clear();
    removed.clear();
    modified.clear();
    if (fileMonitor_) {
        fileMonitor_->ScanModifications(added, removed, modified);
    }
}

IInterface* CreateFileMonitor(IClassFactory& registry, PluginToken token)
{
    return new FileMonitorImpl();
}
IInterface* GetFileApiFactory(IClassRegister& registry, PluginToken token)
{
    static FilesystemApi fact;
    return &fact;
}
CORE_END_NAMESPACE();