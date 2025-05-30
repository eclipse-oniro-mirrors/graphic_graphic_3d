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

#ifndef CORE_IO_PROXY_DIRECTORY_H
#define CORE_IO_PROXY_DIRECTORY_H

#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/io/intf_directory.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class ProxyDirectory final : public IDirectory {
public:
    explicit ProxyDirectory(BASE_NS::vector<IDirectory::Ptr>&& directories);
    ~ProxyDirectory() override = default;

    void Close() override;

    BASE_NS::vector<Entry> GetEntries() const override;

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    BASE_NS::vector<IDirectory::Ptr> directories_;
};
CORE_END_NAMESPACE()

#endif // CORE_IO_PROXY_DIRECTORY_H
