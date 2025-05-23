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

#include "importer.h"

#include <meta/api/util.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/serialization/intf_serializable.h>

#include "ser_nodes.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

static bool IsBuiltinAny(const ObjectId& oid)
{
    static constexpr BASE_NS::Uid uid = MakeUidImpl(0, BUILTIN_ANY_TAG);
    return oid.ToUid().data[1] == uid.data[1];
}

static bool IsBuiltinArrayAny(const ObjectId& oid)
{
    static constexpr BASE_NS::Uid uid = MakeUidImpl(0, BUILTIN_ARRAY_ANY_TAG);
    return oid.ToUid().data[1] == uid.data[1];
}

InstanceId Importer::ConvertInstanceId(const InstanceId& id) const
{
    auto it = mapInstanceIds_.find(id);
    return it != mapInstanceIds_.end() ? it->second : id;
}

BASE_NS::unordered_map<InstanceId, InstanceId> Importer::GetInstanceIdMapping() const
{
    BASE_NS::unordered_map<InstanceId, InstanceId> map;
    for (auto&& v : mapInstanceIds_) {
        map[v.second] = v.first;
    }
    return map;
}

void Importer::SetResourceManager(CORE_NS::IResourceManager::Ptr p)
{
    resources_ = BASE_NS::move(p);
}
void Importer::SetUserContext(IObject::Ptr p)
{
    userContext_ = BASE_NS::move(p);
}

bool Importer::IsRegisteredObjectType(const ObjectId& oid) const
{
    return registry_.GetObjectFactory(oid) != nullptr;
}

bool Importer::IsRegisteredObjectOrPropertyType(const ObjectId& oid) const
{
    return IsRegisteredObjectType(oid) || registry_.GetPropertyRegister().IsPropertyRegistered(oid);
}

IObject::Ptr Importer::Import(const ISerNode::ConstPtr& tree)
{
    IObject::Ptr object;
    if (auto root = interface_cast<IRootNode>(tree)) {
        metadata_ = root->GetMetadata();
        object = ImportObject(root->GetObject());
        if (object) {
            // resolve deferred ref uris
            for (auto&& d : deferred_) {
                if (auto obj = interface_pointer_cast<CORE_NS::IInterface>(ImportRef(d.uri))) {
                    d.target->SetValue(obj);
                } else {
                    CORE_LOG_W("Failed to resolve deferred ref uri");
                }
            }
            // execute finalizes
            for (auto&& f : finalizes_) {
                if (!f->Finalize(*this)) {
                    CORE_LOG_W("Failed to finalize imported object");
                }
            }
        }
    } else {
        CORE_LOG_W("Invalid serialisation tree, expected root node");
    }
    return object;
}

IObject::Ptr Importer::GetReferencedObject(const InstanceId& uid) const
{
    // first check for globals
    if (auto obj = globalData_.GetGlobalObject(uid)) {
        return obj;
    }
    // then see the object registry.
    return registry_.GetObjectInstanceByInstanceId(ConvertInstanceId(uid));
}

IObject::Ptr Importer::ImportRef(const RefUri& ref)
{
    if (ref.BaseObjectUid() == BASE_NS::Uid {}) {
        // for now we only support anchored references, relative references requires to know the current object
        CORE_LOG_W("Missing base object for ref uri [%s]", ref.ToString().c_str());
        return nullptr;
    }
    if (auto obj = interface_pointer_cast<IObjectInstance>(GetReferencedObject(ref.BaseObjectUid()))) {
        auto uri = ref.RelativeUri();
        // interpret all uris as absolute, pointing to the exact thing they say
        uri.SetAbsoluteInterpretation(true);
        if (auto ret = obj->Resolve(uri)) {
            return ret;
        }
    }
    CORE_LOG_W("Failed to find object for ref uri [%s]", ref.ToString().c_str());
    return nullptr;
}

void Importer::MapInstance(const InstanceId& iid, const IObject::ConstPtr& object)
{
    if (auto instance = interface_cast<IObjectInstance>(object)) {
        if (iid.IsValid()) {
            auto& mi = mapInstanceIds_[iid];
            if (mi.IsValid()) {
                CORE_LOG_D("re-mapping object [%s] -> [%s]", iid.ToString().c_str(),
                    instance->GetInstanceId().ToString().c_str());
            } else {
                CORE_LOG_D("mapping object [%s] -> [%s]", iid.ToString().c_str(),
                    instance->GetInstanceId().ToString().c_str());
            }
            mi = instance->GetInstanceId();
        }
    }
}

IObject::Ptr Importer::ImportObject(const IObjectNode::ConstPtr& node, IObject::Ptr object)
{
    IObject::Ptr result;
    if (object) {
        auto iid = node->GetInstanceId();
        MapInstance(iid, object);
        if (auto ser = interface_cast<ISerializable>(object)) {
            ImportContext context(
                *this, node->GetObjectName(), object, iid, interface_pointer_cast<IMapNode>(node->GetMembers()));
            if (ser->Import(context)) {
                result = context.GetObject();
            } else {
                CORE_LOG_W("Failed to import object [type=%s]", node->GetObjectId().ToString().c_str());
            }
        } else if (AutoImportObject(node->GetMembers(), object)) {
            result = object;
        }
    }
    if (auto fin = interface_pointer_cast<IImportFinalize>(result)) {
        finalizes_.push_back(fin);
    }
    return result;
}

IObject::Ptr Importer::ImportObject(const ISerNode::ConstPtr& n)
{
    IObject::Ptr result;
    if (auto node = interface_cast<IRefUriNode>(n)) {
        result = ImportRef(node->GetValue());
    }
    if (auto node = interface_pointer_cast<IObjectNode>(n)) {
        if (!node->GetObjectId().IsValid()) {
            return nullptr;
        }
        IObject::Ptr object;
        if (IsRegisteredObjectType(node->GetObjectId())) {
            object = registry_.Create(node->GetObjectId());
        } else {
            // check if it is property?
            auto name = node->GetObjectName();
            if (!name.empty()) {
                object =
                    interface_pointer_cast<IObject>(registry_.GetPropertyRegister().Create(node->GetObjectId(), name));
            }
        }
        if (object) {
            result = ImportObject(node, object);
        } else {
            CORE_LOG_W("Failed to create requested object type [%s]", node->GetObjectId().ToString().c_str());
        }
    }
    return result;
}

ReturnError Importer::AutoImportObject(const ISerNode::ConstPtr& node, IObject::Ptr object)
{
    if (auto members = interface_cast<IMapNode>(node)) {
        return AutoImportObject(*members, object);
    }
    return GenericError::SUCCESS;
}

ReturnError Importer::AutoImportObject(const IMapNode& members, IObject::Ptr object)
{
    ReturnError res = GenericError::SUCCESS;
    if (auto flags = interface_cast<IObjectFlags>(object)) {
        if (auto fn = members.FindNode("__flags")) {
            res = ImportIObjectFlags(fn, *flags);
        }
    }
    if (auto attach = interface_pointer_cast<IAttach>(object); attach && res) {
        if (auto fn = members.FindNode("__attachments")) {
            res = ImportIAttach(fn, object, *attach);
        }
    }
    if (auto cont = interface_cast<IContainer>(object); cont && res) {
        if (auto fn = members.FindNode("__children")) {
            res = ImportIContainer(fn, *cont);
        }
    }
    return res;
}

ReturnError Importer::ImportIObjectFlags(const ISerNode::ConstPtr& node, IObjectFlags& flags)
{
    Any<uint64_t> any;
    if (ImportValue(node, any)) {
        uint64_t v {};
        if (any.GetValue(v)) {
            flags.SetObjectFlags(v);
            return GenericError::SUCCESS;
        }
    }
    return GenericError::FAIL;
}

static IObject::Ptr FindMatchingAttachment(IAttach& cont, IObject& n)
{
    IObject::Ptr res;
    IterateShared(cont.GetAttachmentContainer(true), [&](const IObject::Ptr& obj) {
        if (n.GetName() == obj->GetName() && n.GetClassId() == obj->GetClassId()) {
            res = obj;
        }
        return !res;
    });
    return res;
}

static IObject::Ptr FindMatchingAttachment(IAttach& cont, ISerNode& n)
{
    IObject::Ptr res;
    if (auto node = interface_cast<IObjectNode>(&n)) {
        IterateShared(cont.GetAttachmentContainer(true), [&](const IObject::Ptr& obj) {
            if (node->GetObjectName() == obj->GetName() && node->GetObjectId() == obj->GetClassId()) {
                res = obj;
            }
            return !res;
        });
    }
    return res;
}

ReturnError Importer::ImportIAttach(const ISerNode::ConstPtr& node, const IObject::Ptr& owner, IAttach& cont)
{
    if (auto array = interface_cast<IArrayNode>(node)) {
        for (auto&& m : array->GetMembers()) {
            if (auto att = FindMatchingAttachment(cont, *m)) {
                if (!ImportObject(interface_pointer_cast<IObjectNode>(m), att)) {
                    CORE_LOG_W("Failed to in-place import attachment");
                }
            } else if (auto att = ImportObject(m)) {
                // there can be already matching attachment in case we are handing a ref node
                // in which case we have to replace the attachment as they are supposed to
                // point to the same object
                if (auto existing = FindMatchingAttachment(cont, *att)) {
                    cont.Detach(existing);
                }
                if (auto attachment = interface_pointer_cast<IAttachment>(att)) {
                    cont.Attach(att, attachment->DataContext()->GetValue().lock());
                } else {
                    cont.Attach(att);
                }
            } else {
                CORE_LOG_W("Failed to import attachment");
                return GenericError::FAIL;
            }
        }
    }
    return GenericError::SUCCESS;
}

ReturnError Importer::ImportIContainer(const ISerNode::ConstPtr& node, IContainer& cont)
{
    if (auto array = interface_cast<IArrayNode>(node)) {
        cont.RemoveAll();
        for (auto&& m : array->GetMembers()) {
            if (auto object = ImportObject(m)) {
                cont.Insert(-1, object);
            }
        }
    }
    return GenericError::SUCCESS;
}

template<typename... Builtins>
static ReturnError ImportSingleBuiltinValue(TypeList<Builtins...>, const ISerNode::ConstPtr& n, IAny& value)
{
    AnyReturnValue res = AnyReturn::FAIL;
    [[maybe_unused]] bool r =
        ((META_NS::IsCompatible(value, Builtins::ID) ? (res = Builtins::ExtractValue(n, value), true) : false) || ...);
    return res ? GenericError::SUCCESS : GenericError::FAIL;
}

ReturnError Importer::ImportArray(const ISerNode::ConstPtr& n, IArrayAny& array)
{
    if (auto node = interface_cast<IArrayNode>(n)) {
        array.RemoveAll();
        for (auto&& m : node->GetMembers()) {
            if (auto any = array.Clone(AnyCloneOptions { CloneValueType::DEFAULT_VALUE, TypeIdRole::ITEM })) {
                if (!ImportValue(m, *any) || !array.InsertAnyAt(-1, *any)) {
                    return GenericError::FAIL;
                }
            } else {
                return GenericError::FAIL;
            }
        }
        return GenericError::SUCCESS;
    }
    return GenericError::FAIL;
}

ReturnError Importer::ImportBuiltinValue(const ISerNode::ConstPtr& n, IAny& entity)
{
    if (auto arr = interface_cast<IArrayAny>(&entity)) {
        return ImportArray(n, *arr);
    }
    if (entity.GetTypeId() == UidFromType<float>()) {
        // handle as double
        Any<double> d;
        auto ret = ImportSingleBuiltinValue(SupportedBuiltins {}, n, d);
        if (ret) {
            entity.SetValue(static_cast<float>(d.InternalGetValue()));
        }
        return ret;
    }
    return ImportSingleBuiltinValue(SupportedBuiltins {}, n, entity);
}

ReturnError Importer::ImportPointer(const ISerNode::ConstPtr& n, IAny& entity)
{
    if (auto nil = interface_cast<INilNode>(n)) {
        entity.SetValue(SharedPtrIInterface {});
        return GenericError::SUCCESS;
    }
    if (auto intf = GetPointer(entity)) {
        if (auto node = interface_pointer_cast<IObjectNode>(n)) {
            if (auto object = interface_pointer_cast<IObject>(intf)) {
                if (ImportObject(node, object)) {
                    return GenericError::SUCCESS;
                }
            } else if (auto any = interface_pointer_cast<IAny>(intf)) {
                return ImportAny(node, any);
            }
        }
    } else {
        if (auto node = interface_pointer_cast<IObjectNode>(n);
            (node && IsRegisteredObjectOrPropertyType(node->GetObjectId())) ||
            interface_cast<IBuiltinValueNode<RefUri>>(n)) {
            if (auto obj = ImportObject(n)) {
                if (entity.SetValue(interface_pointer_cast<CORE_NS::IInterface>(obj))) {
                    return GenericError::SUCCESS;
                }
            }
        } else if (auto any = ImportAny(n)) {
            if (entity.SetValue(interface_pointer_cast<CORE_NS::IInterface>(any))) {
                return GenericError::SUCCESS;
            }
        }
    }
    return GenericError::FAIL;
}

ReturnError Importer::ImportValue(const ISerNode::ConstPtr& n, IAny& entity)
{
    if (auto imp = globalData_.GetValueSerializer(entity.GetTypeId())) {
        if (auto any = imp->Import(*this, n)) {
            if (entity.CopyFrom(*any)) {
                return GenericError::SUCCESS;
            }
        } else {
            CORE_LOG_W("Value import registered for type [%s] but it failed", entity.GetTypeId().ToString().c_str());
        }
    }
    if (ImportBuiltinValue(n, entity)) {
        return GenericError::SUCCESS;
    }
    if (IsGetCompatibleWith<SharedPtrConstIInterface>(entity)) {
        return ImportPointer(n, entity);
    }
    CORE_LOG_F("Failed to import type [%s]", entity.GetTypeId().ToString().c_str());
    return GenericError::FAIL;
}

ReturnError Importer::ImportWeakPtrInAny(const ISerNode::ConstPtr& node, const IAny::Ptr& any)
{
    if (auto nil = interface_cast<INilNode>(node)) {
        any->SetValue(SharedPtrIInterface {});
        return GenericError::SUCCESS;
    }
    if (auto n = interface_cast<IRefUriNode>(node)) {
        // defer resolving the ref uri, might point to object that has not been imported yet.
        deferred_.push_back(DeferredUriResolve { any, n->GetValue() });
        return GenericError::SUCCESS;
    }
    CORE_LOG_F("Cannot import something else than ref uri to weak ptr");
    return GenericError::FAIL;
}

ReturnError Importer::ImportAny(const IObjectNode::ConstPtr& node, const IAny::Ptr& any)
{
    if (auto ser = interface_cast<ISerializable>(any)) {
        ImportContext context(*this, node->GetObjectName(), interface_pointer_cast<IObject>(any), node->GetInstanceId(),
            interface_pointer_cast<IMapNode>(node->GetMembers()));
        if (ser->Import(context)) {
            return GenericError::SUCCESS;
        }
        CORE_LOG_W("Failed to import object [type=%s]", node->GetObjectId().ToString().c_str());
    } else {
        if (auto members = interface_cast<IMapNode>(node->GetMembers())) {
            if (auto value = members->FindNode("value")) {
                if (IsGetCompatibleWith<WeakPtrConstIInterface>(*any)) {
                    if (ImportWeakPtrInAny(value, any)) {
                        return GenericError::SUCCESS;
                    }
                } else if (ImportValue(value, *any)) {
                    return GenericError::SUCCESS;
                }
            }
        }
    }
    CORE_LOG_F("Failed to import any [%s]", any->GetClassId().ToString().c_str());
    return GenericError::FAIL;
}

IAny::Ptr Importer::ImportAny(const ISerNode::ConstPtr& n)
{
    IAny::Ptr any;
    if (auto node = interface_pointer_cast<IObjectNode>(n)) {
        if (!node->GetObjectId().IsValid()) {
            return nullptr;
        }
        any = registry_.GetPropertyRegister().ConstructAny(node->GetObjectId());
        if (any) {
            if (!ImportAny(node, any)) {
                any.reset();
            }
        } else {
            CORE_LOG_F("No such any-type registered [%s, classname=%s, name=%s]",
                node->GetObjectId().ToString().c_str(), node->GetObjectClassName().c_str(),
                node->GetObjectName().c_str());
        }
    }
    return any;
}

IObject::Ptr Importer::ResolveRefUri(const RefUri& uri)
{
    return ImportRef(uri);
}

ReturnError Importer::ImportFromNode(const ISerNode::ConstPtr& node, IAny& entity)
{
    return ImportValue(node, entity);
}

bool ImportContext::HasMember(BASE_NS::string_view name) const
{
    return node_ && node_->FindNode(name);
}

ReturnError ImportContext::Import(BASE_NS::string_view name, IAny& entity)
{
    if (node_) {
        if (auto n = node_->FindNode(name)) {
            return importer_.ImportValue(n, entity);
        }
    }
    CORE_LOG_W("Failed to import member with name '%s'", BASE_NS::string(name).c_str());
    return GenericError::FAIL;
}

ReturnError ImportContext::ImportAny(BASE_NS::string_view name, IAny::Ptr& any)
{
    if (node_) {
        if (auto n = node_->FindNode(name)) {
            any = importer_.ImportAny(n);
            return GenericError::SUCCESS;
        }
    }
    CORE_LOG_W("Failed to import member with name '%s'", BASE_NS::string(name).c_str());
    return GenericError::FAIL;
}

ReturnError ImportContext::ImportWeakPtr(BASE_NS::string_view name, IObject::WeakPtr& ptr)
{
    if (node_) {
        if (auto n = node_->FindNode(name)) {
            if (auto node = interface_cast<IRefUriNode>(n)) {
                ptr = importer_.ImportRef(node->GetValue());
                return GenericError::SUCCESS;
            }
            CORE_LOG_W("Failed to import weak ptr from non ref uri node");
        }
    }
    CORE_LOG_W("Failed to import member with name '%s'", BASE_NS::string(name).c_str());
    return GenericError::FAIL;
}

ReturnError ImportContext::AutoImport()
{
    if (!object_) {
        CORE_LOG_W("Failed to auto import, imported type is not IObject");
        return GenericError::FAIL;
    }
    if (node_) {
        return importer_.AutoImportObject(*node_, object_);
    }
    CORE_LOG_W("Failed to auto import, invalid node");
    return GenericError::FAIL;
}

IObject::Ptr ImportContext::ResolveRefUri(const RefUri& uri)
{
    return importer_.ImportRef(uri);
}

ReturnError ImportContext::ImportFromNode(const ISerNode::ConstPtr& node, IAny& entity)
{
    return importer_.ImportFromNode(node, entity);
}

CORE_NS::IInterface* ImportContext::Context() const
{
    return importer_.GetInterface(CORE_NS::IInterface::UID);
}
IObject::Ptr ImportContext::UserContext() const
{
    return importer_.GetUserContext();
}
ReturnError ImportContext::SubstituteThis(IObject::Ptr obj)
{
    ReturnError res = GenericError::FAIL;
    if (obj) {
        object_ = BASE_NS::move(obj);
        // re-map the instance
        importer_.MapInstance(iid_, object_);
        res = GenericError::SUCCESS;
    }
    return res;
}
BASE_NS::string ImportContext::GetName() const
{
    return name_;
}
SerMetadata ImportContext::GetMetadata() const
{
    return importer_.GetMetadata();
}
} // namespace Serialization
META_END_NAMESPACE()
