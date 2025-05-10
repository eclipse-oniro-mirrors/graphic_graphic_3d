#ifndef META_API_METADATA_UTIL_H
#define META_API_METADATA_UTIL_H

#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/property/property.h>

META_BEGIN_NAMESPACE()

inline bool IsValueSet(const IProperty& p)
{
    if (p.IsDefaultValue()) {
        return false;
    }
    if (auto stack = interface_cast<IStackProperty>(&p)) {
        // only way to compare anys for now
        if (auto copy = p.GetValue().Clone()) {
            return copy->CopyFrom(stack->GetDefaultValue()) != AnyReturn::NOTHING_TO_DO;
        }
    }
    return true;
}

inline bool IsValueSame(const IProperty& p1, const IProperty& p2)
{
    // only way to compare anys for now
    if (auto copy = p1.GetValue().Clone()) {
        return copy->CopyFrom(p2.GetValue()) == AnyReturn::NOTHING_TO_DO;
    }
    return false;
}

inline void CloneToDefaultIfSet(const IProperty::ConstPtr& p, IMetadata& out, BASE_NS::string_view name = {})
{
    if (PropertyLock lock { p }) {
        if (IsValueSet(*p)) {
            if (auto copy = DuplicatePropertyType(GetObjectRegistry(), p, name)) {
                if (auto sc = interface_cast<IStackProperty>(copy)) {
                    sc->SetDefaultValue(p->GetValue());
                }
                out.AddProperty(copy);
            }
        }
    }
}

inline void CloneToDefault(const IProperty::ConstPtr& p, IMetadata& out, BASE_NS::string_view name = {})
{
    if (PropertyLock lock { p }) {
        if (auto copy = DuplicatePropertyType(GetObjectRegistry(), p, name)) {
            if (auto sc = interface_cast<IStackProperty>(copy)) {
                sc->SetDefaultValue(p->GetValue());
            }
            out.AddProperty(copy);
        }
    }
}

inline void Clone(const IProperty::ConstPtr& p, IMetadata& out, BASE_NS::string_view name = {})
{
    if (PropertyLock lock { p }) {
        if (auto copy = DuplicatePropertyType(GetObjectRegistry(), p, name)) {
            copy->SetValue(p->GetValue());
            out.AddProperty(copy);
        }
    }
}

inline void CloneToDefaultIfSet(const IMetadata& in, IMetadata& out)
{
    for (auto&& p : in.GetProperties()) {
        CloneToDefaultIfSet(p, out);
    }
}

inline void CloneToDefault(const IMetadata& in, IMetadata& out)
{
    for (auto&& p : in.GetProperties()) {
        CloneToDefault(p, out);
    }
}

inline void Clone(const IMetadata& in, IMetadata& out)
{
    for (auto&& p : in.GetProperties()) {
        Clone(p, out);
    }
}

inline void CopyValue(const IProperty::ConstPtr& p, IProperty& out)
{
    if (PropertyLock plock { p }) {
        if (PropertyLock lock { &out }) {
            if (!out.SetValue(p->GetValue())) {
                CORE_LOG_W("Failed to set property value [%s]", out.GetName().c_str());
            }
        }
    }
}

inline void CopyValue(const IProperty::ConstPtr& p, IMetadata& out, BASE_NS::string_view name = {})
{
    BASE_NS::string n(name.empty() ? p->GetName() : name);
    if (auto target = out.GetProperty(n)) {
        CopyValue(p, *target);
    } else {
        CORE_LOG_W("No such property [%s]", n.c_str());
    }
}

inline void CopyToDefaultAndReset(const IProperty::ConstPtr& p, IProperty& out)
{
    if (PropertyLock plock { p }) {
        if (PropertyLock lock { &out }) {
            if (auto sc = interface_cast<IStackProperty>(&out)) {
                sc->SetDefaultValue(p->GetValue());
            }
            out.ResetValue();
        }
    }
}

inline void CopyToDefaultAndReset(const IProperty::ConstPtr& p, IMetadata& out, BASE_NS::string_view name = {})
{
    if (PropertyLock plock { p }) {
        BASE_NS::string n(name.empty() ? p->GetName() : name);
        if (auto target = out.GetProperty(n)) {
            CopyToDefaultAndReset(p, *target);
        } else {
            CORE_LOG_W("No such property [%s]", n.c_str());
        }
    }
}

inline void CopyValue(const IMetadata& in, IMetadata& out)
{
    for (auto&& p : in.GetProperties()) {
        CopyValue(p, out);
    }
}

inline void CopyToDefaultAndReset(const IMetadata& in, IMetadata& out)
{
    for (auto&& p : in.GetProperties()) {
        CopyToDefaultAndReset(p, out);
    }
}

inline bool CreatePropertiesFromStaticMeta(const ObjectId& id, IMetadata& out, bool includeReadOnly = true)
{
    auto& r = META_NS::GetObjectRegistry();
    auto fac = r.GetObjectFactory(id);
    if (!fac) {
        return false;
    }
    if (auto sm = fac->GetClassStaticMetadata()) {
        for (auto&& v : GetAllStaticMetadata(*sm, MetadataType::PROPERTY)) {
            if (!v.readOnly || includeReadOnly) {
                if (v.data && v.data->runtimeValue) {
                    if (auto def = v.data->runtimeValue()) {
                        if (auto p = ConstructPropertyAny(v.data->name, *def)) {
                            out.AddProperty(p);
                        }
                    }
                }
            }
        }
    }
    return true;
}

META_END_NAMESPACE()

#endif
