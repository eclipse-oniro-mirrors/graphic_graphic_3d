/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_PROPERTY_PROPERTY_TYPES_H
#define API_CORE_PROPERTY_PROPERTY_TYPES_H

#include <cstddef>
#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/vector.h>
#include <base/math/matrix.h>
#include <base/math/quaternion.h>
#include <base/util/compile_time_hashes.h>
#include <base/util/uid.h>
#include <core/ecs/entity.h>
#include <core/ecs/entity_reference.h>
#include <core/namespace.h>
#include <core/property/property.h>

CORE_BEGIN_NAMESPACE()
namespace PropertyType {
/* Primitive types */
inline constexpr PropertyTypeDecl BOOL_T = PROPERTYTYPE(bool);
inline constexpr PropertyTypeDecl CHAR_T = PROPERTYTYPE(char);
inline constexpr PropertyTypeDecl INT8_T = PROPERTYTYPE(int8_t);
inline constexpr PropertyTypeDecl INT16_T = PROPERTYTYPE(int16_t);
inline constexpr PropertyTypeDecl INT32_T = PROPERTYTYPE(int32_t);
inline constexpr PropertyTypeDecl INT64_T = PROPERTYTYPE(int64_t);
inline constexpr PropertyTypeDecl UINT8_T = PROPERTYTYPE(uint8_t);
inline constexpr PropertyTypeDecl UINT16_T = PROPERTYTYPE(uint16_t);
inline constexpr PropertyTypeDecl UINT32_T = PROPERTYTYPE(uint32_t);
inline constexpr PropertyTypeDecl UINT64_T = PROPERTYTYPE(uint64_t);
#ifdef __APPLE__
inline constexpr PropertyTypeDecl SIZE_T = PROPERTYTYPE(size_t);
#endif
inline constexpr PropertyTypeDecl FLOAT_T = PROPERTYTYPE(float);
inline constexpr PropertyTypeDecl DOUBLE_T = PROPERTYTYPE(double);

inline constexpr PropertyTypeDecl BOOL_ARRAY_T = PROPERTYTYPE_ARRAY(bool);
inline constexpr PropertyTypeDecl CHAR_ARRAY_T = PROPERTYTYPE_ARRAY(char);
inline constexpr PropertyTypeDecl INT8_ARRAY_T = PROPERTYTYPE_ARRAY(int8_t);
inline constexpr PropertyTypeDecl INT16_ARRAY_T = PROPERTYTYPE_ARRAY(int16_t);
inline constexpr PropertyTypeDecl INT32_ARRAY_T = PROPERTYTYPE_ARRAY(int32_t);
inline constexpr PropertyTypeDecl INT64_ARRAY_T = PROPERTYTYPE_ARRAY(int64_t);
inline constexpr PropertyTypeDecl UINT8_ARRAY_T = PROPERTYTYPE_ARRAY(uint8_t);
inline constexpr PropertyTypeDecl UINT16_ARRAY_T = PROPERTYTYPE_ARRAY(uint16_t);
inline constexpr PropertyTypeDecl UINT32_ARRAY_T = PROPERTYTYPE_ARRAY(uint32_t);
inline constexpr PropertyTypeDecl UINT64_ARRAY_T = PROPERTYTYPE_ARRAY(uint64_t);
#ifdef __APPLE__
inline constexpr PropertyTypeDecl SIZE_ARRAY_T = PROPERTYTYPE_ARRAY(size_t);
#endif
inline constexpr PropertyTypeDecl FLOAT_ARRAY_T = PROPERTYTYPE_ARRAY(float);
inline constexpr PropertyTypeDecl DOUBLE_ARRAY_T = PROPERTYTYPE_ARRAY(double);

/* Extended types */
inline constexpr PropertyTypeDecl VEC2_T = PROPERTYTYPE(BASE_NS::Math::Vec2);
inline constexpr PropertyTypeDecl VEC3_T = PROPERTYTYPE(BASE_NS::Math::Vec3);
inline constexpr PropertyTypeDecl VEC4_T = PROPERTYTYPE(BASE_NS::Math::Vec4);
inline constexpr PropertyTypeDecl UVEC2_T = PROPERTYTYPE(BASE_NS::Math::UVec2);
inline constexpr PropertyTypeDecl UVEC3_T = PROPERTYTYPE(BASE_NS::Math::UVec3);
inline constexpr PropertyTypeDecl UVEC4_T = PROPERTYTYPE(BASE_NS::Math::UVec4);
inline constexpr PropertyTypeDecl QUAT_T = PROPERTYTYPE(BASE_NS::Math::Quat);
inline constexpr PropertyTypeDecl MAT3X3_T = PROPERTYTYPE(BASE_NS::Math::Mat3X3);
inline constexpr PropertyTypeDecl MAT4X4_T = PROPERTYTYPE(BASE_NS::Math::Mat4X4);
inline constexpr PropertyTypeDecl UID_T = PROPERTYTYPE(BASE_NS::Uid);
inline constexpr PropertyTypeDecl ENTITY_T = PROPERTYTYPE(Entity);
inline constexpr PropertyTypeDecl ENTITY_REFERENCE_T = PROPERTYTYPE(EntityReference);
inline constexpr PropertyTypeDecl STRING_T = PROPERTYTYPE(BASE_NS::string);

inline constexpr PropertyTypeDecl VEC2_ARRAY_T = PROPERTYTYPE_ARRAY(BASE_NS::Math::Vec2);
inline constexpr PropertyTypeDecl VEC3_ARRAY_T = PROPERTYTYPE_ARRAY(BASE_NS::Math::Vec3);
inline constexpr PropertyTypeDecl VEC4_ARRAY_T = PROPERTYTYPE_ARRAY(BASE_NS::Math::Vec4);
inline constexpr PropertyTypeDecl UVEC2_ARRAY_T = PROPERTYTYPE_ARRAY(BASE_NS::Math::UVec2);
inline constexpr PropertyTypeDecl UVEC3_ARRAY_T = PROPERTYTYPE_ARRAY(BASE_NS::Math::UVec3);
inline constexpr PropertyTypeDecl UVEC4_ARRAY_T = PROPERTYTYPE_ARRAY(BASE_NS::Math::UVec4);
inline constexpr PropertyTypeDecl QUAT_ARRAY_T = PROPERTYTYPE_ARRAY(BASE_NS::Math::Quat);
inline constexpr PropertyTypeDecl MAT3X3_ARRAY_T = PROPERTYTYPE_ARRAY(BASE_NS::Math::Mat3X3);
inline constexpr PropertyTypeDecl MAT4X4_ARRAY_T = PROPERTYTYPE_ARRAY(BASE_NS::Math::Mat4X4);
inline constexpr PropertyTypeDecl UID_ARRAY_T = PROPERTYTYPE_ARRAY(BASE_NS::Uid);
inline constexpr PropertyTypeDecl ENTITY_ARRAY_T = PROPERTYTYPE_ARRAY(Entity);
inline constexpr PropertyTypeDecl ENTITY_REFERENCE_ARRAY_T = PROPERTYTYPE_ARRAY(EntityReference);

inline constexpr PropertyTypeDecl FLOAT_VECTOR_T = PROPERTYTYPE(BASE_NS::vector<float>);
inline constexpr PropertyTypeDecl MAT4X4_VECTOR_T = PROPERTYTYPE(BASE_NS::vector<BASE_NS::Math::Mat4X4>);
inline constexpr PropertyTypeDecl ENTITY_REFERENCE_VECTOR_T = PROPERTYTYPE(BASE_NS::vector<EntityReference>);

inline constexpr PropertyTypeDecl INVALID = PropertyTypeDecl { false, 0, 0, "" };
} // namespace PropertyType

// The following declarations should not be used at public headers anymore, it's purely implementation stuff now
namespace PropertySystem {
template<class T>
struct is_defined : BASE_NS::false_type {
    using type = BASE_NS::false_type;
};
template<class T, class B>
inline constexpr auto PropertyTypeDeclFromType()
{
    static_assert(is_defined<T>().value, "Missing DECLARE_PROPERTY_TYPE for used type.");
    return CORE_NS::PropertyTypeDecl {};
}
} // namespace PropertySystem

#define DECLARE_PROPERTY_TYPE(aType)                                                   \
    namespace PropertySystem {                                                         \
    template<>                                                                         \
    struct is_defined<aType> : BASE_NS::true_type {                                    \
        using type = BASE_NS::true_type;                                               \
    };                                                                                 \
    template<>                                                                         \
    inline constexpr auto PropertyTypeDeclFromType<const aType, BASE_NS::false_type>() \
    {                                                                                  \
        return PROPERTYTYPE(aType);                                                    \
    }                                                                                  \
    template<>                                                                         \
    inline constexpr auto PropertyTypeDeclFromType<const aType, BASE_NS::true_type>()  \
    {                                                                                  \
        return PROPERTYTYPE_ARRAY(aType);                                              \
    }                                                                                  \
    template<>                                                                         \
    inline constexpr auto PropertyTypeDeclFromType<aType, BASE_NS::false_type>()       \
    {                                                                                  \
        return PROPERTYTYPE(aType);                                                    \
    }                                                                                  \
    template<>                                                                         \
    inline constexpr auto PropertyTypeDeclFromType<aType, BASE_NS::true_type>()        \
    {                                                                                  \
        return PROPERTYTYPE_ARRAY(aType);                                              \
    }                                                                                  \
    } // namespace PropertySystem
CORE_END_NAMESPACE()
#endif // API_CORE_PROPERTY_PROPERTY_TYPES_H
