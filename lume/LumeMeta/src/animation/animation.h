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
#ifndef META_SRC_ANIMATION_H
#define META_SRC_ANIMATION_H

#include <meta/api/make_callback.h>
#include <meta/ext/event_util.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/serialization/intf_serializable.h>

#include "object.h"
#include "animation_state.h"
#include "staggered_animation_state.h"

META_BEGIN_NAMESPACE()

namespace Internal {

/**
 * @brief A base class which can be used by generic animation implementations.
 */
template<class BaseAnimationInterface>
class BaseAnimationFwd : public IntroduceInterfaces<MetaObject, BaseAnimationInterface, INotifyOnChange, IAttachment,
                             IContainable, IMutableContainable, IAnimationInternal> {
    static_assert(BASE_NS::is_convertible_v<BaseAnimationInterface*, IAnimation*>,
        "BaseAnimationInterface of BaseAnimationFwd must inherit from IAnimation");

    using MyBase = IntroduceInterfaces<MetaObject, BaseAnimationInterface, INotifyOnChange, IAttachment, IContainable,
        IMutableContainable, IAnimationInternal>;
    META_OBJECT_NO_CLASSINFO(BaseAnimationFwd, MyBase)

protected:
    BaseAnimationFwd() = default;
    ~BaseAnimationFwd() override = default;

protected: // IObject
    BASE_NS::string GetName() const override
    {
        return META_ACCESS_PROPERTY_VALUE(Name);
    }

protected: // ILifecycle
    bool Build(const IMetadata::Ptr& data) override
    {
        if (Super::Build(data)) {
            META_ACCESS_PROPERTY(Name)->SetDefaultValue(Super::GetName());
            return GetState().Initialize(BASE_NS::move(GetParams()));
        }
        return false;
    }
    void Destroy() override
    {
        GetState().Uninitialize();
        Super::Destroy();
    }

    virtual AnimationState::AnimationStateParams GetParams() = 0;

protected: // IContainable
    IObject::Ptr GetParent() const override
    {
        return parent_.lock();
    }

protected: // IMutableContainable
    void SetParent(const IObject::Ptr& parent) override
    {
        parent_ = parent;
    }

protected: // IAttach
    bool Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext) override
    {
        return GetState().Attach(attachment, dataContext);
    }
    bool Detach(const IObject::Ptr& attachment) override
    {
        return GetState().Detach(attachment);
    }

protected: // IAnimationInternal
    void ResetClock() override
    {
        GetState().ResetClock();
    }

    bool Move(const IAnimationInternal::MoveParams& move) override
    {
        return GetState().Move(move).changed;
    }

    void OnAnimationStateChanged(const IAnimationInternal::AnimationStateChangedInfo& info) override
    {
        Evaluate();
    }
    void OnEvaluationNeeded() override
    {
        Evaluate();
    }

protected: // IAttachment
    bool Attaching(const IAttach::Ptr& target, const IObject::Ptr& dataContext) override
    {
        SetValue(META_ACCESS_PROPERTY(AttachedTo), target);
        SetValue(META_ACCESS_PROPERTY(DataContext), dataContext);
        return true;
    }
    bool Detaching(const IAttach::Ptr& target) override
    {
        SetValue(META_ACCESS_PROPERTY(AttachedTo), {});
        SetValue(META_ACCESS_PROPERTY(DataContext), {});
        return true;
    }

protected: // IAnimation
    void Step(const IClock::ConstPtr& clock) override
    {
        GetState().Step(clock);
    }

protected: // INotifyOnChange
    void NotifyChanged()
    {
        Invoke<IOnChanged>(EventOnChanged(MetadataQuery::EXISTING));
    }

protected:
    virtual void Evaluate() = 0;
    virtual Internal::AnimationState& GetState() noexcept = 0;

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(INamed, BASE_NS::string, Name)
    META_STATIC_PROPERTY_DATA(IAttachment, IObject::WeakPtr, DataContext)
    META_STATIC_PROPERTY_DATA(IAttachment, IAttach::WeakPtr, AttachedTo, {}, DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_STATIC_PROPERTY_DATA(IAnimation, bool, Enabled, true)
    META_STATIC_PROPERTY_DATA(IAnimation, bool, Valid, {}, DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_STATIC_PROPERTY_DATA(IAnimation, TimeSpan, TotalDuration, {}, DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_STATIC_PROPERTY_DATA(IAnimation, bool, Running, {}, DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_STATIC_PROPERTY_DATA(IAnimation, float, Progress, {}, DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_STATIC_PROPERTY_DATA(IAnimation, IAnimationController::WeakPtr, Controller, {}, DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_STATIC_PROPERTY_DATA(IAnimation, ICurve1D::Ptr, Curve)
    META_STATIC_EVENT_DATA(IAnimation, IOnChanged, OnFinished)
    META_STATIC_EVENT_DATA(IAnimation, IOnChanged, OnStarted)
    META_STATIC_EVENT_DATA(INotifyOnChange, IOnChanged, OnChanged)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)
    META_IMPLEMENT_READONLY_PROPERTY(IObject::WeakPtr, DataContext)
    META_IMPLEMENT_READONLY_PROPERTY(IAttach::WeakPtr, AttachedTo)
    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_READONLY_PROPERTY(bool, Valid)
    META_IMPLEMENT_READONLY_PROPERTY(TimeSpan, TotalDuration)
    META_IMPLEMENT_READONLY_PROPERTY(bool, Running)
    META_IMPLEMENT_READONLY_PROPERTY(float, Progress)
    META_IMPLEMENT_PROPERTY(IAnimationController::WeakPtr, Controller)
    META_IMPLEMENT_PROPERTY(ICurve1D::Ptr, Curve)
    META_IMPLEMENT_EVENT(IOnChanged, OnFinished)
    META_IMPLEMENT_EVENT(IOnChanged, OnStarted)
    META_IMPLEMENT_EVENT(IOnChanged, OnChanged)

private:
    IObject::WeakPtr parent_;
};

template<class BaseAnimationInterface>
class BaseStartableAnimationFwd
    : public IntroduceInterfaces<BaseAnimationFwd<BaseAnimationInterface>, IStartableAnimation> {
    using Super = IntroduceInterfaces<BaseAnimationFwd<BaseAnimationInterface>, IStartableAnimation>;
    using Super::GetState;

protected: // IStartableAnimation
    void Pause() override
    {
        GetState().Pause();
    }
    void Restart() override
    {
        GetState().Restart();
    }
    void Seek(float position) override
    {
        GetState().Seek(position);
    }
    void Start() override
    {
        GetState().Start();
    }
    void Stop() override
    {
        GetState().Stop();
    }
    void Finish() override
    {
        GetState().Finish();
    }
};

/**
 * @brief A base class which can be used by animation container implementations.
 */
template<class BaseAnimationInterface>
class BaseAnimationContainerFwd : public IntroduceInterfaces<BaseStartableAnimationFwd<BaseAnimationInterface>,
                                      IContainer, ILockable, IIterable, IImportFinalize> {
    static_assert(BASE_NS::is_convertible_v<BaseAnimationInterface*, IStaggeredAnimation*>,
        "BaseAnimationInterface of BaseAnimationContainerFwd must inherit from IStaggeredAnimation");
    using Super = IntroduceInterfaces<BaseStartableAnimationFwd<BaseAnimationInterface>, IContainer, ILockable,
        IIterable, IImportFinalize>;
    using IContainer::SizeType;

public: // ILockable
    void Lock() const override
    {
        if (auto lockable = interface_cast<ILockable>(&GetContainer())) {
            lockable->Lock();
        }
    }
    void Unlock() const override
    {
        if (auto lockable = interface_cast<ILockable>(&GetContainer())) {
            lockable->Unlock();
        }
    }
    void LockShared() const override
    {
        if (auto lockable = interface_cast<ILockable>(&GetContainer())) {
            lockable->LockShared();
        }
    }
    void UnlockShared() const override
    {
        if (auto lockable = interface_cast<ILockable>(&GetContainer())) {
            lockable->UnlockShared();
        }
    }

protected:
    Internal::StaggeredAnimationState& GetState() noexcept override = 0;

    ReturnError Finalize(IImportFunctions&) override
    {
        GetState().ChildrenChanged();
        return GenericError::SUCCESS;
    }

public: // IIterable
    IterationResult Iterate(const IterationParameters& params) override
    {
        auto iterable = interface_cast<IIterable>(&GetContainer());
        return iterable ? iterable->Iterate(params) : IterationResult::FAILED;
    }
    IterationResult Iterate(const IterationParameters& params) const override
    {
        const auto iterable = interface_cast<IIterable>(&GetContainer());
        return iterable ? iterable->Iterate(params) : IterationResult::FAILED;
    }

public: // IContainer
    bool Add(const IObject::Ptr& object) override
    {
        return GetContainer().Add(object);
    }
    bool Insert(SizeType index, const IObject::Ptr& object) override
    {
        return GetContainer().Insert(index, object);
    }
    bool Remove(SizeType index) override
    {
        return GetContainer().Remove(index);
    }
    bool Remove(const IObject::Ptr& child) override
    {
        return GetContainer().Remove(child);
    }
    bool Move(SizeType fromIndex, SizeType toIndex) override
    {
        return GetContainer().Move(fromIndex, toIndex);
    }
    bool Move(const IObject::Ptr& child, SizeType toIndex) override
    {
        return GetContainer().Move(child, toIndex);
    }
    bool Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways) override
    {
        return GetContainer().Replace(child, replaceWith, addAlways);
    }
    BASE_NS::vector<IObject::Ptr> GetAll() const override
    {
        return GetContainer().GetAll();
    }
    void RemoveAll() override
    {
        GetContainer().RemoveAll();
    }
    IObject::Ptr GetAt(SizeType index) const override
    {
        return GetContainer().GetAt(index);
    }
    SizeType GetSize() const override
    {
        return GetContainer().GetSize();
    }
    BASE_NS::vector<IObject::Ptr> FindAll(const IContainer::FindOptions& options) const override
    {
        return GetContainer().FindAll(options);
    }
    IObject::Ptr FindAny(const IContainer::FindOptions& options) const override
    {
        return GetContainer().FindAny(options);
    }
    IObject::Ptr FindByName(BASE_NS::string_view name) const override
    {
        return GetContainer().FindByName(name);
    }
    bool IsAncestorOf(const IObject::ConstPtr& object) const override
    {
        return GetContainer().IsAncestorOf(object);
    }

    META_FORWARD_EVENT(IEvent, OnContainerChanged, GetContainer().EventOnContainerChanged)

protected: // IStaggeredAnimation
    void AddAnimation(const IAnimation::Ptr& animation) override
    {
        GetContainer().Add(animation);
    }
    void RemoveAnimation(const IAnimation::Ptr& animation) override
    {
        GetContainer().Remove(animation);
    }
    BASE_NS::vector<IAnimation::Ptr> GetAnimations() const override
    {
        return GetContainer().template GetAll<IAnimation>();
    }

protected:
    virtual IContainer& GetContainer() noexcept = 0;
    virtual const IContainer& GetContainer() const noexcept = 0;
};

/**
 * @brief A base class which can be used by property animation implementations.
 */
template<class BaseAnimationInterface>
class BasePropertyAnimationFwd : public IntroduceInterfaces<BaseAnimationFwd<BaseAnimationInterface>,
                                     IPropertyAnimation, IModifier, IImportFinalize> {
    static_assert(BASE_NS::is_convertible_v<BaseAnimationInterface*, ITimedAnimation*>,
        "BaseAnimationInterface of BasePropertyAnimationFwd must inherit from ITimedAnimation");

    using MyBase =
        IntroduceInterfaces<BaseAnimationFwd<BaseAnimationInterface>, IPropertyAnimation, IModifier, IImportFinalize>;
    META_OBJECT_NO_CLASSINFO(BasePropertyAnimationFwd, MyBase)

protected:
    BasePropertyAnimationFwd() = default;
    ~BasePropertyAnimationFwd() override = default;

protected: // ILifecycle
    bool Build(const IMetadata::Ptr& data) override
    {
        if (Super::Build(data)) {
            META_ACCESS_PROPERTY(Property)->OnChanged()->AddHandler(
                MakeCallback<IOnChanged>(this, &BasePropertyAnimationFwd::PropertyChanged));
            return true;
        }
        return false;
    }

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IPropertyAnimation, IProperty::WeakPtr, Property)
    META_STATIC_PROPERTY_DATA(ITimedAnimation, TimeSpan, Duration)
    META_END_STATIC_DATA()
    META_IMPLEMENT_PROPERTY(IProperty::WeakPtr, Property)
    META_IMPLEMENT_PROPERTY(TimeSpan, Duration)

protected: // IModifier
    EvaluationResult ProcessOnGet(IAny& value) override
    {
        return EvaluationResult::EVAL_CONTINUE;
    }
    EvaluationResult ProcessOnSet(IAny& value, const IAny& current) override
    {
        return EvaluationResult::EVAL_CONTINUE;
    }
    bool IsCompatible(const TypeId& id) const override
    {
        if (auto p = GetTargetProperty()) {
            PropertyLock lock { p.property };
            return META_NS::IsCompatible(lock->GetValueAny(), id);
        }
        return false;
    }
    Internal::PropertyAnimationState& GetState() noexcept override = 0;

protected:
    ReturnError Finalize(IImportFunctions&) override
    {
        GetState().UpdateTotalDuration();
        auto p = GetTargetProperty();
        PropertyLock lock { p.property };
        GetState().SetInterpolator(lock ? lock->GetTypeId() : TypeId {});
        SetValue(Super::META_ACCESS_PROPERTY(Valid), p);
        return GenericError::SUCCESS;
    }

protected:
    struct TargetProperty {
        IProperty::Ptr property;
        IStackProperty::Ptr stack;
        operator bool() const noexcept
        {
            return property && stack;
        }
    };

    void PropertyChanged()
    {
        auto p = GetTargetProperty();
        PropertyLock lock { p.property };
        this->GetState().SetInterpolator(lock ? lock->GetTypeId() : TypeId {});
        SetValue(Super::META_ACCESS_PROPERTY(Valid), p);
        OnPropertyChanged(p, property_.lock());
        property_ = p.stack;
    }

    virtual void OnPropertyChanged(const TargetProperty& property, const IStackProperty::Ptr& previous) = 0;

    TargetProperty GetTargetProperty() const noexcept
    {
        auto p = META_ACCESS_PROPERTY_VALUE(Property).lock();
        return { p, interface_pointer_cast<IStackProperty>(p) };
    }

private:
    IStackProperty::WeakPtr property_;
};

/**
 * @brief A base class which can be used by property animation implementations.
 */
template<class BaseAnimationInterface>
class PropertyAnimationFwd : public BasePropertyAnimationFwd<BaseAnimationInterface> {
    using Super = BasePropertyAnimationFwd<BaseAnimationInterface>;

protected:
    PropertyAnimationFwd() = default;
    ~PropertyAnimationFwd() override = default;

protected:
    // Note covariance
    Internal::PropertyAnimationState& GetState() noexcept override
    {
        return state_;
    }

private:
    Internal::PropertyAnimationState state_;
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_SRC_ANIMATION_H
