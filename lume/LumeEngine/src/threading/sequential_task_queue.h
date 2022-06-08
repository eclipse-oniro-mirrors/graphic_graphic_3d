/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_THREADING_SEQUENTIAL_TASK_QUEUE_H
#define CORE_THREADING_SEQUENTIAL_TASK_QUEUE_H

#include <base/containers/vector.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

#include "threading/task_queue.h"

CORE_BEGIN_NAMESPACE()
// Non-thread safe sequential task queue, executes tasks sequentially one-by-one.
// This queue type is not thread safe and should be only used from one thread.
class SequentialTaskQueue final : public TaskQueue {
public:
    /** Constructor for the sequential task queue.
        @param threads Optional thread pool, if support for threading is desired.
    */
    explicit SequentialTaskQueue(const IThreadPool::Ptr& threadPool);
    SequentialTaskQueue(const SequentialTaskQueue& other) = delete;
    ~SequentialTaskQueue() override;

    /** Submit task to execution queue, to be run after another task.
        @param afterIdentifier Identifier of the task that is run prior the submitted task.
        @param taskIdentifier Identifier of the task, must be unique.
        @param task Task to execute.
    */
    void SubmitAfter(uint64_t afterIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task);

    /** Submit task to execution queue, to be run before another task.
        @param beforeIdentifier Identifier of the task that is run after the submitted task.
        @param taskIdentifier Identifier of the task, must be unique.
        @param task Task to execute.
    */
    void SubmitBefore(uint64_t beforeIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task);

    void Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task) override;
    void Remove(uint64_t taskIdentifier) override;

    void Clear() override;

    void Execute() override;

private:
    BASE_NS::vector<TaskQueue::Entry> tasks_;
};
CORE_END_NAMESPACE()

#endif // CORE_THREADING_SEQUENTIAL_TASK_QUEUE_H
