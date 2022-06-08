/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "performance_data_manager.h"

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <map>
#include <mutex>

#include <base/math/mathf.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>
#include <core/plugin/intf_plugin_register.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::make_unique;
using BASE_NS::pair;
using BASE_NS::string_view;
using BASE_NS::Uid;
using BASE_NS::vector;

struct PerformanceDataManager::InternalData {
    NameToPerformanceMap data;
};

namespace {
#if (CORE_PERF_ENABLED == 1)
void UpdateTimingData(const string_view subCategory, const string_view name, const int64_t microSeconds,
    PerformanceDataManager::TypeDataSet& dataSet)
{
    auto iter = dataSet.find(subCategory);
    if (iter != dataSet.end()) {
        auto dataIter = iter->second.data.find(name);
        if (dataIter != iter->second.data.end()) {
            auto& ref = dataIter->second;
            const int64_t arrayIndex = ref.counter % IPerformanceDataManager::TIMING_DATA_POOL_SIZE;
            ref.counter++;
            ref.currentTime = microSeconds;
            if (microSeconds < ref.minTime) {
                ref.minTime = microSeconds;
            }
            if (microSeconds > ref.maxTime) {
                ref.maxTime = microSeconds;
            }
            ref.timings[arrayIndex] = microSeconds;
            int64_t frameAverage = 0;
            for (const auto& timingsRef : ref.timings) {
                frameAverage += timingsRef;
            }
            ref.averageTime = frameAverage / IPerformanceDataManager::TIMING_DATA_POOL_SIZE;
            ref.totalTime += ref.currentTime;
            if (ref.totalTime > ref.maxTotalTime) {
                ref.maxTotalTime = ref.totalTime;
            }
        } else {
            // new value
            auto& ref = iter->second.data[name];
            ref = {
                microSeconds, // currentTime
                microSeconds, // maxTime
                microSeconds, // minTime
                microSeconds, // averageTime
                microSeconds, // totalTime
                1,            // counter
            };
            std::fill_n(ref.timings, IPerformanceDataManager::TIMING_DATA_POOL_SIZE, microSeconds);
        }
    } else {
        // new subcategory and new value
        auto& ref = dataSet[subCategory].data[name];
        ref = {
            microSeconds, // currentTime
            microSeconds, // maxTime
            microSeconds, // minTime
            microSeconds, // averageTime
            microSeconds, // totalTime
            1,            // counter
        };
        std::fill_n(ref.timings, IPerformanceDataManager::TIMING_DATA_POOL_SIZE, microSeconds);
    }
}

vector<IPerformanceDataManager::PerformanceData> GetTimingData(const PerformanceDataManager::TypeDataSet& typeData)
{
    vector<IPerformanceDataManager::PerformanceData> data;
    data.reserve(typeData.size());
    for (const auto& typeRef : typeData) {
        IPerformanceDataManager::PerformanceData& pd = data.emplace_back();
        pd.subCategory = typeRef.first;
        for (const auto& perfRef : typeRef.second.data) {
            pd.timings[perfRef.first] = perfRef.second;
        }
    }
    return data;
}
#endif // CORE_PERF_ENABLED
} // namespace

PerformanceDataManager::~PerformanceDataManager() = default;

PerformanceDataManager::PerformanceDataManager(const string_view category) : category_(category) {}

string_view PerformanceDataManager::GetCategory() const
{
    return category_;
}

using Clock = std::chrono::system_clock;

IPerformanceDataManager::TimerHandle PerformanceDataManager::BeginTimer()
{
    return static_cast<IPerformanceDataManager::TimerHandle>(Clock::now().time_since_epoch().count());
}

int64_t PerformanceDataManager::EndTimer(IPerformanceDataManager::TimerHandle handle)
{
    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    const auto dt = Clock::now().time_since_epoch() - Clock::duration(handle);
    return static_cast<int64_t>(duration_cast<microseconds>(dt).count());
}

void PerformanceDataManager::UpdateData(
    const string_view subCategory, const string_view name, const int64_t microSeconds)
{
#if (CORE_PERF_ENABLED == 1)
    std::lock_guard<std::mutex> lock(dataMutex_);
    UpdateTimingData(subCategory, name, microSeconds, data_);
#endif
}

void PerformanceDataManager::ResetData()
{
#if (CORE_PERF_ENABLED == 1)
    std::lock_guard<std::mutex> lock(dataMutex_);
    data_.clear();
#endif
}

vector<IPerformanceDataManager::PerformanceData> PerformanceDataManager::GetData() const
{
#if (CORE_PERF_ENABLED == 1)
    std::lock_guard<std::mutex> lock(dataMutex_);
    return GetTimingData(data_);
#else
    return {};
#endif
}

void PerformanceDataManager::RemoveData(const string_view subCategory)
{
#if (CORE_PERF_ENABLED == 1)
    std::lock_guard<std::mutex> lock(dataMutex_);
    data_.erase(subCategory);
#endif
}

void PerformanceDataManager::DumpToLog() const
{
#if (CORE_PERF_ENABLED == 1)
    std::lock_guard<std::mutex> lock(dataMutex_);

    constexpr const string_view formatLegend = "%8s %8s %8s %9s %8s (microseconds)";
    constexpr const string_view formatData = "%8" PRIu64 " %8" PRIu64 " %8" PRIu64 " %9" PRIu64 " %8" PRIu64 " %s::%s";

    CORE_LOG_I(formatLegend.data(), "avg", "min", "max", "total", "count");

    for (const auto& typeRef : data_) {
        for (const auto& perfRef : typeRef.second.data) {
            const int64_t counter = BASE_NS::Math::max(perfRef.second.counter, int64_t(1));
            CORE_LOG_I(formatData.data(), perfRef.second.totalTime / counter, perfRef.second.minTime,
                perfRef.second.maxTime, perfRef.second.totalTime, perfRef.second.counter, typeRef.first.c_str(),
                perfRef.first.c_str());
        }
    }
#endif
}

// IInterface
const IInterface* PerformanceDataManager::GetInterface(const Uid& uid) const
{
    if (uid == IPerformanceDataManager::UID) {
        return this;
    }
    return nullptr;
}

IInterface* PerformanceDataManager::GetInterface(const Uid& uid)
{
    if (uid == IPerformanceDataManager::UID) {
        return this;
    }
    return nullptr;
}

void PerformanceDataManager::Ref() {}

void PerformanceDataManager::Unref() {}

IPerformanceDataManager* PerformanceDataManagerFactory::Get(const string_view category)
{
#if (CORE_PERF_ENABLED == 1)
    std::lock_guard lock(mutex_);
    if (auto pos = managers_.find(category); pos != managers_.end()) {
        return pos->second.get();
    } else {
        auto inserted = managers_.insert({ category, make_unique<PerformanceDataManager>(category) });
        return inserted.first->second.get();
    }
#else
    return {};
#endif
}

vector<IPerformanceDataManager*> PerformanceDataManagerFactory::GetAllCategories() const
{
    vector<IPerformanceDataManager*> categories;
#if (CORE_PERF_ENABLED == 1)
    std::lock_guard lock(mutex_);
    categories.reserve(managers_.size());
    for (const auto& manager : managers_) {
        if (auto* mgrPtr = manager.second.get(); mgrPtr) {
            categories.push_back(mgrPtr);
        }
    }
#endif
    return categories;
}

// IInterface
const IInterface* PerformanceDataManagerFactory::GetInterface(const Uid& uid) const
{
    if (uid == IPerformanceDataManagerFactory::UID) {
        return this;
    }
    return nullptr;
}

IInterface* PerformanceDataManagerFactory::GetInterface(const Uid& uid)
{
    if (uid == IPerformanceDataManagerFactory::UID) {
        return this;
    }
    return nullptr;
}

void PerformanceDataManagerFactory::Ref() {}

void PerformanceDataManagerFactory::Unref() {}
CORE_END_NAMESPACE()
