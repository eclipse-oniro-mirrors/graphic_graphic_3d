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

#ifndef API_UTIL_JSONUTIL_H
#define API_UTIL_JSONUTIL_H

#include <cstdlib>
#include <type_traits>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/math/vector.h>
#include <base/math/quaternion.h>
#include <base/util/uid_util.h>

#include <core/json/json.h>

#include <util/namespace.h>

UTIL_BEGIN_NAMESPACE()

inline ::string JsonUnescape(::string_view str)
{
    return CORE_NS::json::unescape(str);
}

template<class T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
inline bool SafeGetJsonValue(
    const CORE_NS::json::value& jsonData, const ::string_view element, ::string& error, T& output)
{
    if (auto const pos = jsonData.find(element); pos) {
        if (pos->is_number()) {
            output = pos->as_number<T>();
            return true;
        } else if (pos->is_boolean()) {
            output = pos->boolean_;
            return true;
        } else {
            error += element + ": expected number.\n";
        }
    }
    return false;
}

template<class T, std::enable_if_t<std::is_convertible_v<T, ::string_view>, bool> = true>
inline bool SafeGetJsonValue(
    const CORE_NS::json::value& jsonData, const ::string_view element, ::string& error, T& output)
{
    if (auto const pos = jsonData.find(element); pos) {
        if (pos->is_string()) {
            output = JsonUnescape(T(pos->string_.data(), pos->string_.size()));
            return true;
        } else {
            error += element + ": expected string.\n";
        }
    }
    return false;
}

template<class T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
inline bool FromJson(const CORE_NS::json::value& jsonIn, T& output)
{
    if (jsonIn.is_number()) {
        output = jsonIn.as_number<T>();
        return true;
    }
    return false;
}

template<class T, std::enable_if_t<std::is_convertible_v<T, ::string_view>, bool> = true>
inline bool FromJson(const CORE_NS::json::value& jsonIn, T& output)
{
    if (jsonIn.is_string()) {
        output = JsonUnescape(static_cast<T>(jsonIn.string_));
        return true;
    }
    return false;
}

inline bool FromJson(const CORE_NS::json::value& jsonIn, bool& output)
{
    if (jsonIn.is_boolean()) {
        output = jsonIn.boolean_;
        return true;
    }
    return false;
}

inline bool FromJson(const CORE_NS::json::value& jsonIn, ::Uid& output)
{
    constexpr size_t UID_LENGTH = 36;
    if (jsonIn.is_string() && jsonIn.string_.size() == UID_LENGTH) {
        output = StringToUid(JsonUnescape(jsonIn.string_));
        return true;
    }
    return false;
}

template<class T>
inline bool FromJsonArray(const CORE_NS::json::value& jsonIn, T* output, size_t size)
{
    if (jsonIn.is_array() && jsonIn.array_.size() == size) {
        for (const auto& element : jsonIn.array_) {
            if (!FromJson(element, *output)) {
                return false;
            }
            output++;
        }
        return true;
    }
    return false;
}

inline bool FromJson(const CORE_NS::json::value& jsonIn, ::Math::Vec2& output)
{
    return FromJsonArray(jsonIn, output.data, 2);
}

inline bool FromJson(const CORE_NS::json::value& jsonIn, ::Math::Vec3& output)
{
    return FromJsonArray(jsonIn, output.data, 3);
}

inline bool FromJson(const CORE_NS::json::value& jsonIn, ::Math::Vec4& output)
{
    return FromJsonArray(jsonIn, output.data, 4);
}

inline bool FromJson(const CORE_NS::json::value& jsonIn, ::Math::UVec2& output)
{
    return FromJsonArray(jsonIn, output.data, 2);
}

inline bool FromJson(const CORE_NS::json::value& jsonIn, ::Math::UVec3& output)
{
    return FromJsonArray(jsonIn, output.data, 3);
}

inline bool FromJson(const CORE_NS::json::value& jsonIn, ::Math::UVec4& output)
{
    return FromJsonArray(jsonIn, output.data, 4);
}

inline bool FromJson(const CORE_NS::json::value& jsonIn, ::Math::Quat& output)
{
    return FromJsonArray(jsonIn, output.data, 4);
}

template<class T>
inline CORE_NS::json::standalone_value ToJson(T value)
{
    return CORE_NS::json::standalone_value(value);
}

// FIXME: how to make more generic?, Does not understand fixed_string
template<>
inline CORE_NS::json::standalone_value ToJson(::string_view value)
{
    return CORE_NS::json::standalone_value(::string{ value });
}

template<>
inline CORE_NS::json::standalone_value ToJson(::string value)
{
    return CORE_NS::json::standalone_value(value);
}

template<>
inline CORE_NS::json::standalone_value ToJson<::Math::Vec2>(::Math::Vec2 value)
{
    CORE_NS::json::standalone_value json = CORE_NS::json::standalone_value::array();
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.x));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.y));
    return json;
}
template<>
inline CORE_NS::json::standalone_value ToJson<::Math::UVec2>(::Math::UVec2 value)
{
    CORE_NS::json::standalone_value json = CORE_NS::json::standalone_value::array();
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.x));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.y));
    return json;
}

template<>
inline CORE_NS::json::standalone_value ToJson<::Math::Vec3>(::Math::Vec3 value)
{
    CORE_NS::json::standalone_value json = CORE_NS::json::standalone_value::array();
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.x));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.y));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.z));
    return json;
}
template<>
inline CORE_NS::json::standalone_value ToJson<::Math::UVec3>(::Math::UVec3 value)
{
    CORE_NS::json::standalone_value json = CORE_NS::json::standalone_value::array();
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.x));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.y));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.z));
    return json;
}

template<>
inline CORE_NS::json::standalone_value ToJson<::Math::Vec4>(::Math::Vec4 value)
{
    CORE_NS::json::standalone_value json = CORE_NS::json::standalone_value::array();
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.x));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.y));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.z));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.w));
    return json;
}
template<>
inline CORE_NS::json::standalone_value ToJson<::Math::UVec4>(::Math::UVec4 value)
{
    CORE_NS::json::standalone_value json = CORE_NS::json::standalone_value::array();
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.x));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.y));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.z));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.w));
    return json;
}

template<>
inline CORE_NS::json::standalone_value ToJson<::Math::Quat>(::Math::Quat value)
{
    CORE_NS::json::standalone_value json = CORE_NS::json::standalone_value::array();
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.x));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.y));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.z));
    json.array_.emplace_back(CORE_NS::json::standalone_value(value.w));
    return json;
}

template<>
inline CORE_NS::json::standalone_value ToJson<::Uid>(::Uid value)
{
    return ToJson(::string_view{ to_string(value) });
}

UTIL_END_NAMESPACE()
#endif // API_UTIL_JSONUTIL_H
