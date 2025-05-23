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

#ifndef META_INTERFACE_EVENT_H
#define META_INTERFACE_EVENT_H

#include <base/containers/shared_ptr.h>

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()
class IEvent;
class ICallable;
template<typename EventT>
class Event;
template<>
class Event<IEvent> {
public:
    using EventType = IEvent;
    using ValueType = BASE_NS::shared_ptr<IEvent>;

    Event(const ValueType& ev = nullptr) : event_(ev) {}

    ValueType operator->() const
    {
        return event_;
    }
    IEvent& operator*() const
    {
        return *event_;
    }
    operator ValueType() const
    {
        return event_;
    }
    explicit operator bool() const
    {
        return event_ != nullptr;
    }
    ValueType GetEvent() const
    {
        return event_;
    }

protected:
    BASE_NS::shared_ptr<IEvent> event_;
};
template<typename EventT>
class Event : public Event<IEvent> {
public:
    using Event<IEvent>::Event;
    using ValueType = BASE_NS::shared_ptr<EventT>;

    Event(const ValueType& ev = nullptr) : Event<IEvent>(interface_pointer_cast<IEvent>(ev)) {}
    BASE_NS::shared_ptr<ICallable> GetCallable() const
    {
        return interface_pointer_cast<ICallable>(event_);
    }
};

META_END_NAMESPACE()

#endif
