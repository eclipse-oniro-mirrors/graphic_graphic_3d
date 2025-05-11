#ifndef META_INTERFACE_DETAIL_ANY_H
#define META_INTERFACE_DETAIL_ANY_H

#include <meta/interface/detail/base_any.h>
#include <meta/interface/detail/enum.h>

META_BEGIN_NAMESPACE()

template<typename Type, typename = EnableSpecialisationType>
struct MapAnyType {
    using AnyType = Any<Type>;
    using ArrayAnyType = ArrayAny<Type>;
};

/*
    Use something like this for partial specialising:
    template<typename Type>
    struct MapAnyType<Type, EnableSpecialisation<is_enum_v<Type>> {};
*/
template<typename Type>
struct MapAnyType<Type, EnableSpecialisation<is_enum_v<Type>>> {
    using AnyType = EnumBase<Type>;
    using ArrayAnyType = ArrayEnumBase<Type>;
};

template<typename Type>
using AnyType = typename MapAnyType<Type>::AnyType;

template<typename Type>
using ArrayAnyType = typename MapAnyType<Type>::ArrayAnyType;

template<typename Type>
static IAny::Ptr ConstructAny(Type v = {})
{
    return IAny::Ptr { new AnyType<Type>(BASE_NS::move(v)) };
}

template<typename Type>
static IAny::Ptr ConstructAny(BASE_NS::vector<Type> v)
{
    return IAny::Ptr { new ArrayAnyType<Type>(BASE_NS::move(v)) };
}

template<typename Type>
static IArrayAny::Ptr ConstructArrayAny(BASE_NS::vector<Type> v = {})
{
    return IArrayAny::Ptr { new ArrayAnyType<Type>(BASE_NS::move(v)) };
}

template<typename Type>
static IArrayAny::Ptr ConstructArrayAny(BASE_NS::array_view<const Type> v)
{
    return IArrayAny::Ptr { new ArrayAnyType<Type>(v) };
}

#ifdef BASE_VECTOR_HAS_INITIALIZE_LIST
template<typename Type>
static IArrayAny::Ptr ConstructArrayAny(std::initializer_list<Type> l)
{
    return IArrayAny::Ptr { new ArrayAnyType<Type>(l) };
}
#endif

META_END_NAMESPACE()

#endif
