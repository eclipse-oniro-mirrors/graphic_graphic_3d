/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef META_SRC_STARTABLE_OBJECT_CONTROLLER_H
#define META_SRC_STARTABLE_OBJECT_CONTROLLER_H

#include <mutex>
#include <shared_mutex>

#include <base/containers/unordered_map.h>

#include <meta/api/timer.h>
#include <meta/base/namespace.h>
#include <meta/ext/attachment/attachment.h>
#include <meta/ext/implementation_macros.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_hierarchy_observer.h>
#include <meta/interface/intf_startable_controller.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_tickable_controller.h>

META_BEGIN_NAMESPACE()

class IStartableObjectControllerInternal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IStartableObjectControllerInternal, "fd5c13af-de72-4cc4-bed0-07cb6a751ac1")
public:
    /**
     * @brief Run tasks with given queueId.
     */
    virtual void RunTasks(const ITaskQueue::Ptr& queue) = 0;
};

class StartableObjectController : public IntroduceInterfaces<ObjectFwd, IStartableController, IObjectHierarchyObserver,
                                      IStartableObjectControllerInternal, ITickableController> {
    META_OBJECT(StartableObjectController, ClassId::StartableObjectController, IntroduceInterfaces)
public: // ILifeCycle
    bool Build(const IMetadata::Ptr& data) override;
    void Destroy() override;

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(
        IStartableController, META_NS::TraversalType, TraversalType, META_NS::TraversalType::DEPTH_FIRST_POST_ORDER)
    META_STATIC_PROPERTY_DATA(
        IStartableController, META_NS::StartBehavior, StartBehavior, META_NS::StartBehavior::AUTOMATIC)
    META_STATIC_PROPERTY_DATA(ITickableController, META_NS::TimeSpan, TickInterval, META_NS::TimeSpan::Infinite())
    META_STATIC_PROPERTY_DATA(
        ITickableController, META_NS::TraversalType, TickOrder, META_NS::TraversalType::DEPTH_FIRST_PRE_ORDER)
    META_END_STATIC_DATA()
    META_IMPLEMENT_PROPERTY(META_NS::TraversalType, TraversalType)
    META_IMPLEMENT_PROPERTY(META_NS::StartBehavior, StartBehavior)
    META_IMPLEMENT_PROPERTY(META_NS::TimeSpan, TickInterval)
    META_IMPLEMENT_PROPERTY(META_NS::TraversalType, TickOrder)

public: // IStartableController
    bool StartAll(ControlBehavior behavior) override;
    bool StopAll(ControlBehavior behavior) override;
    BASE_NS::vector<IStartable::Ptr> GetAllStartables() const override;
    bool SetStartableQueueId(
        const BASE_NS::Uid& startStartableQueueId, const BASE_NS::Uid& stopStartableQueueId) override;
    bool SetStartableQueue(const META_NS::ITaskQueue::Ptr& startStartableQueue,
        const META_NS::ITaskQueue::Ptr& stopStartableQueue) override;

public: // IObjectHierarchyObserver
    void SetTarget(const IObject::Ptr& root, HierarchyChangeModeValue mode) override;
    IObject::Ptr GetTarget() const override;
    BASE_NS::vector<IObject::Ptr> GetAllObserved() const override;
    META_FORWARD_EVENT(IOnHierarchyChanged, OnHierarchyChanged, observer_->EventOnHierarchyChanged)

public: // ITickableController
    BASE_NS::vector<ITickable::Ptr> GetTickables() const override;
    void TickAll(const TimeSpan& time) override;
    bool SetTickableQueueuId(const BASE_NS::Uid& queueId) override;

private:
    void StartHierarchy(const IObject::Ptr& root, ControlBehavior behavior);
    void StopHierarchy(const IObject::Ptr& root, ControlBehavior behavior);
    void StartStartable(IStartable* const startable, ControlBehavior behavior);
    void StopStartable(IStartable* const startable, ControlBehavior behavior);
    void HierarchyChanged(const HierarchyChangedInfo& info);

    IObject::WeakPtr target_;
    IObjectHierarchyObserver::Ptr observer_;

private: // IStartableObjectControllerInternal
    void RunTasks(const ITaskQueue::Ptr& queue) override;

private: // Task queue handling
    struct StartableOperation {
        enum Operation {
            /** Run StartHierarchy() for root_ */
            START,
            /** Run StopHierarchy() for root_ */
            STOP,
        };

        Operation operation_;
        IObject::WeakPtr root_;
    };

    mutable META_NS::ITaskQueue::WeakPtr startQueue_;
    mutable META_NS::ITaskQueue::WeakPtr stopQueue_;
    BASE_NS::Uid startQueueId_;
    BASE_NS::Uid stopQueueId_;
    bool HasTasks(const ITaskQueue::Ptr& queue) const;
    bool ProcessOps(const ITaskQueue::Ptr& queue);
    bool AddOperation(StartableOperation&& operation, const ITaskQueue::Ptr& queue);
    BASE_NS::unordered_map<ITaskQueue*, BASE_NS::vector<StartableOperation>> operations_;
    mutable std::shared_mutex mutex_;
    std::size_t executingStart_ {};

    META_NS::ITaskQueue::Ptr GetStartQueue() const;
    META_NS::ITaskQueue::Ptr GetStopQueue() const;

private: // Tickables
    void InvalidateTickables();
    void UpdateTicker();
    mutable BASE_NS::vector<ITickable::WeakPtr> tickables_;
    mutable std::shared_mutex tickMutex_;
    mutable bool tickablesValid_ { false };
    TimeSpan lastTick_ { TimeSpan::Infinite() };
    IClock::Ptr clock_;
    BASE_NS::Uid tickQueueId_;
    ITaskQueue::Ptr tickerQueue_;
    ITaskQueue::Ptr defaultTickerQueue_;
    ITaskQueueTask::Ptr tickerTask_;
    ITaskQueue::Token tickerToken_ {};
};

META_END_NAMESPACE()

#endif // META_SRC_STARTABLE_OBJECT_CONTROLLER_H
