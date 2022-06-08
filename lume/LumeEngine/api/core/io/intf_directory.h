/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_IO_IDIRECTORY_H
#define API_CORE_IO_IDIRECTORY_H

#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
/** @ingroup group_io_idirectory */
/** IThe IDirectory interface provides access to directory structures and their contents. */
class IDirectory {
public:
    /** Entry, is description of either a file or directory */
    struct Entry {
        /** Enumeration for entry type */
        enum Type {
            /** Unknown type specification */
            UNKNOWN,
            /** Type specification: File */
            FILE,
            /** Type specification: Directory */
            DIRECTORY
        };

        /** Type of the entry */
        Type type;
        /** Name of the entry */
        BASE_NS::string name;
        /** timestamp, 0 if not applicable. */
        uint64_t timestamp { 0 };
    };

    /** Close the directory. */
    virtual void Close() = 0;

    /** Get list of file/directory entries in this directory.
     *  @return List of file / directory entries in this directory.
     */
    virtual BASE_NS::vector<Entry> GetEntries() const = 0;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IDirectory* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IDirectory, Deleter>;

protected:
    IDirectory() = default;
    virtual ~IDirectory() = default;
    virtual void Destroy() = 0;
};
CORE_END_NAMESPACE()

#endif // API_CORE_IO_IDIRECTORY_H
