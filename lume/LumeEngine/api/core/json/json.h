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

#ifndef API_CORE_JSON_JSON_H
#define API_CORE_JSON_JSON_H

#include <cstdint>
#include <cstdio>
#include <new>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
namespace json {
using readonly_string_t = BASE_NS::string_view;
using writable_string_t = BASE_NS::string;
template<typename T>
using array_t = BASE_NS::vector<T>;

/** Type of a JSON value. */
enum class type : uint8_t {
    uninitialized = 0,
    object,
    array,
    string,
    floating_point,
    signed_int,
    unsigned_int,
    boolean,
    null,
};

/** Tag for JSON values which contain read-only strings. The source JSON string must be kept alive until the parsing
 * results are not used. */
struct readonly_tag {};

/** Tag for JSON values which contain writable strings. Life time of the parsing results doens't depend on the source
 * JSON string. */
struct writable_tag {};

template<typename T = readonly_tag>
struct value_t;

/** JSON structure which contains read-only strings. The source JSON string must be kept alive until the parsing
 * results are not used. */
using value = value_t<readonly_tag>;

/** JSON structure which contains writable strings. String values can be modified and if the instance was generated by
 * the parser, the source string doesn't need to be stored while the instance is used. */
using standalone_value = value_t<writable_tag>;

/** Parses 'data' and returns JSON structure. the value::type will be 'uninitialized' if parsing failed.
 * @param data JSON as a null terminated string.
 * @return Parsed JSON structure.
 */
template<typename T = readonly_tag>
value_t<T> parse(const char* data);

/** Converts a JSON structure into a string.
 * @param value JSON structure.
 * @return JSON as string.
 */
template<typename T = readonly_tag>
BASE_NS::string to_string(const value_t<T>& value);

BASE_NS::string unescape(BASE_NS::string_view str);

BASE_NS::string escape(BASE_NS::string_view str);

/** JSON value. */
template<typename Tag>
struct value_t {
    /** Type used for JSON strings and JSON object keys. */
    using string =
        typename BASE_NS::conditional_t<BASE_NS::is_same_v<Tag, writable_tag>, writable_string_t, readonly_string_t>;

    /** Type used for JSON null */
    struct null {};

    /** Type used for key-value pairs inside JSON objects. */
    struct pair {
        pair(string&& k, value_t&& v) : key(BASE_NS::forward<string>(k)), value(BASE_NS::forward<value_t>(v)) {}
        string key;
        value_t value;
    };

    /** Type used for JSON objects. */
    using object = array_t<pair>;

    /** Type used for JSON arrays. */
    using array = array_t<value_t>;

    /** Type of this JSON value. */
    type type { type::uninitialized };
    union {
        object object_;
        array array_;
        string string_;
        double float_;
        int64_t signed_;
        uint64_t unsigned_;
        bool boolean_;
    };

    value_t() noexcept : type { type::uninitialized } {}

    value_t(object&& value) noexcept : type { type::object }, object_(BASE_NS::move(value)) {}

    value_t(array&& value) noexcept : type { type::array }, array_(BASE_NS::move(value)) {}

    value_t(string value) noexcept : type { type::string }, string_(BASE_NS::move(value)) {}

    value_t(const char* value) noexcept : value_t(string(value)) {}

    template<typename Number, BASE_NS::enable_if_t<BASE_NS::is_arithmetic_v<Number>, bool> = true>
    value_t(Number value) noexcept
    {
        if constexpr (BASE_NS::is_same_v<Number, bool>) {
            type = type::boolean;
            boolean_ = value;
        } else if constexpr (BASE_NS::is_floating_point_v<Number>) {
            type = type::floating_point;
            float_ = static_cast<double>(value);
        } else if constexpr (BASE_NS::is_signed_v<Number>) {
            type = type::signed_int;
            signed_ = static_cast<int64_t>(value);
        } else if constexpr (BASE_NS::is_unsigned_v<Number>) {
            type = type::unsigned_int;
            unsigned_ = static_cast<uint64_t>(value);
        }
    }

    value_t(null /* value */) noexcept : type { type::null } {}

    template<typename Value>
    value_t(const array_t<Value>& values) noexcept : type { type::array }, array_()
    {
        array_.reserve(values.size());
        for (const auto& value : values) {
            array_.emplace_back(value);
        }
    }

    template<typename Value, size_t N>
    value_t(Value (&value)[N]) : type { type::array }, array_()
    {
        array_.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            array_.emplace_back(value[i]);
        }
    }

    value_t(const value_t& other) : type(other.type)
    {
        switch (type) {
            case type::uninitialized:
                break;
            case type::object:
                new (&object_) object(other.object_);
                break;
            case type::array:
                new (&array_) array(other.array_);
                break;
            case type::string:
                new (&string_) string(other.string_);
                break;
            case type::floating_point:
                float_ = other.float_;
                break;
            case type::signed_int:
                signed_ = other.signed_;
                break;
            case type::unsigned_int:
                unsigned_ = other.unsigned_;
                break;
            case type::boolean:
                boolean_ = other.boolean_;
                break;
            case type::null:
                break;
            default:
                break;
        }
    }

    value_t& operator=(const value_t& other)
    {
        if (this != &other) {
            cleanup();
            type = other.type;
            switch (type) {
                case type::uninitialized:
                    break;
                case type::object:
                    new (&object_) object(other.object_);
                    break;
                case type::array:
                    new (&array_) array(other.array_);
                    break;
                case type::string:
                    new (&string_) string(other.string_);
                    break;
                case type::floating_point:
                    float_ = other.float_;
                    break;
                case type::signed_int:
                    signed_ = other.signed_;
                    break;
                case type::unsigned_int:
                    unsigned_ = other.unsigned_;
                    break;
                case type::boolean:
                    boolean_ = other.boolean_;
                    break;
                case type::null:
                    break;
                default:
                    break;
            }
        }
        return *this;
    }

    value_t& operator=(string value)
    {
        cleanup();
        type = type::string;
        new (&string_) string(BASE_NS::move(value));
        return *this;
    }

    value_t& operator=(const char* value)
    {
        cleanup();
        type = type::string;
        new (&string_) string(value);
        return *this;
    }

    template<typename Number, BASE_NS::enable_if_t<BASE_NS::is_arithmetic_v<Number>, bool> = true>
    value_t& operator=(Number value)
    {
        cleanup();
        if constexpr (BASE_NS::is_same_v<Number, bool>) {
            type = type::boolean;
            boolean_ = value;
        } else if constexpr (BASE_NS::is_floating_point_v<Number>) {
            type = type::floating_point;
            float_ = static_cast<double>(value);
        } else if constexpr (BASE_NS::is_signed_v<Number>) {
            type = type::signed_int;
            signed_ = static_cast<int64_t>(value);
        } else if constexpr (BASE_NS::is_unsigned_v<Number>) {
            type = type::unsigned_int;
            unsigned_ = static_cast<uint64_t>(value);
        }
        return *this;
    }

    value_t(value_t&& rhs) noexcept : type { BASE_NS::exchange(rhs.type, type::uninitialized) }
    {
        switch (type) {
            case type::uninitialized:
                break;
            case type::object:
                new (&object_) object(BASE_NS::move(rhs.object_));
                break;
            case type::array:
                new (&array_) array(BASE_NS::move(rhs.array_));
                break;
            case type::string:
                new (&string_) string(BASE_NS::move(rhs.string_));
                break;
            case type::floating_point:
                float_ = rhs.float_;
                break;
            case type::signed_int:
                signed_ = rhs.signed_;
                break;
            case type::unsigned_int:
                unsigned_ = rhs.unsigned_;
                break;
            case type::boolean:
                boolean_ = rhs.boolean_;
                break;
            case type::null:
                break;
            default:
                break;
        }
    }

    value_t& operator=(value_t&& rhs) noexcept
    {
        if (this != &rhs) {
            cleanup();
            type = BASE_NS::exchange(rhs.type, type::uninitialized);
            switch (type) {
                case type::uninitialized:
                    break;
                case type::object:
                    new (&object_) object(BASE_NS::move(rhs.object_));
                    break;
                case type::array:
                    new (&array_) array(BASE_NS::move(rhs.array_));
                    break;
                case type::string:
                    new (&string_) string(BASE_NS::move(rhs.string_));
                    break;
                case type::floating_point:
                    float_ = rhs.float_;
                    break;
                case type::signed_int:
                    signed_ = rhs.signed_;
                    break;
                case type::unsigned_int:
                    unsigned_ = rhs.unsigned_;
                    break;
                case type::boolean:
                    boolean_ = rhs.boolean_;
                    break;
                case type::null:
                    break;
                default:
                    break;
            }
        }
        return *this;
    }

    template<typename OtherT>
    operator value_t<OtherT>() const
    {
        value_t<OtherT> other;
        other.type = type;
        switch (type) {
            case type::uninitialized:
                break;
            case type::object:
                new (&other.object_) typename value_t<OtherT>::object(BASE_NS::default_allocator());
                other.object_.reserve(object_.size());
                for (const auto& p : object_) {
                    other.object_.emplace_back(typename value_t<OtherT>::string(p.key), p.value);
                }
                break;
            case type::array:
                new (&other.array_) typename value_t<OtherT>::array(BASE_NS::default_allocator());
                other.array_.reserve(array_.size());
                for (const auto& v : array_) {
                    other.array_.emplace_back(v);
                }
                break;
            case type::string:
                new (&other.string_) typename value_t<OtherT>::string(string_);
                break;
            case type::floating_point:
                other.float_ = float_;
                break;
            case type::signed_int:
                other.signed_ = signed_;
                break;
            case type::unsigned_int:
                other.unsigned_ = unsigned_;
                break;
            case type::boolean:
                other.boolean_ = boolean_;
                break;
            case type::null:
                break;
            default:
                break;
        }
        return other;
    }

#if _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4583)
#endif
    ~value_t()
    {
        cleanup();
        type = type::uninitialized;
    }
#if _MSC_VER
#pragma warning(pop)
#endif
    template<typename T>
    inline void destroy(T& t)
    {
        t.~T();
    }

    void cleanup()
    {
        switch (type) {
            case type::uninitialized:
                break;
            case type::object:
                destroy(object_);
                break;
            case type::array:
                destroy(array_);
                break;
            case type::string:
                destroy(string_);
                break;
            case type::floating_point:
                break;
            case type::signed_int:
                break;
            case type::unsigned_int:
                break;
            case type::boolean:
                break;
            case type::null:
                break;
            default:
                break;
        }
    }

    explicit operator bool() const noexcept
    {
        return type != type::uninitialized;
    }

    bool is_object() const noexcept
    {
        return type == type::object;
    }

    bool is_array() const noexcept
    {
        return type == type::array;
    }

    bool is_string() const noexcept
    {
        return type == type::string;
    }

    bool is_floating_point() const noexcept
    {
        return type == type::floating_point;
    }

    bool is_signed_int() const noexcept
    {
        return type == type::signed_int;
    }

    bool is_unsigned_int() const noexcept
    {
        return type == type::unsigned_int;
    }

    bool is_number() const noexcept
    {
        return type == type::floating_point || type == type::signed_int || type == type::unsigned_int;
    }

    bool is_boolean() const noexcept
    {
        return type == type::boolean;
    }

    bool is_null() const noexcept
    {
        return type == type::null;
    }

    bool empty() const noexcept
    {
        if (is_object()) {
            return object_.empty();
        } else if (is_array()) {
            return array_.empty();
        }
        return true;
    }

    template<typename T>
    T as_number() const
    {
        switch (type) {
            case type::floating_point:
                return static_cast<T>(float_);
            case type::signed_int:
                return static_cast<T>(signed_);
            case type::unsigned_int:
                return static_cast<T>(unsigned_);
            default:
                return 0;
        }
    }

    const value_t* find(BASE_NS::string_view key) const noexcept
    {
        if (type == type::object) {
            for (auto& t : object_) {
                if (t.key == key) {
                    return &t.value;
                }
            }
        }
        return nullptr;
    }

    value_t& operator[](const BASE_NS::string_view& key)
    {
        if (type == type::object) {
            for (auto& t : object_) {
                if (t.key == key) {
                    return t.value;
                }
            }
            object_.emplace_back(value_t<Tag>::string(key), value_t<Tag> {});
            return object_.back().value;
        }
        return *this;
    }
};
} // namespace json
CORE_END_NAMESPACE()

#ifdef JSON_IMPL
#include <securec.h>

#include <base/containers/fixed_string.h>
#include <base/util/uid.h>

CORE_BEGIN_NAMESPACE()
namespace json {
inline bool isWhite(char data)
{
    return ((data == ' ') || (data == '\n') || (data == '\r') || (data == '\t'));
}

inline bool isSign(char data)
{
    return ((data == '+') || (data == '-'));
}

inline bool isDigit(char data)
{
    return ((data >= '0') && (data <= '9'));
}

inline bool isHex(char data)
{
    return ((data >= '0') && (data <= '9')) || ((data >= 'a') && (data <= 'f')) || ((data >= 'A') && (data <= 'F'));
}

inline const char* trim(const char* data)
{
    while (*data && isWhite(*data)) {
        data++;
    }
    return data;
}

// values
template<typename T>
const char* parse_string(const char* data, value_t<T>& res)
{
    const char* start = data;
    for (; *data != 0; data++) {
        if (*data == '\\' && data[1]) {
            // escape.. (parse just enough to not stop too early)
            if (data[1] == '\\' || data[1] == '"' || data[1] == '/' || data[1] == 'b' || data[1] == 'f' ||
                data[1] == 'n' || data[1] == 'r' || data[1] == 't') {
                ++data;
                continue;
            } else if (data[1] == 'u') {
                data += 2;
                for (const char* end = data + 4; data != end; ++data) {
                    if (*data == 0 || !isHex(*data)) {
                        // invalid Unicode
                        return data;
                    }
                }
                --data;
            } else {
                // invalid escape
                return data;
            }
        } else if (*data == '"') {
            res = value_t<T> { typename value_t<T>::string { start, static_cast<size_t>(data - start) } };
            return data + 1;
        } else if (static_cast<unsigned char>(*data) < 0x20) {
            // unescaped control
            return data;
        }
    }
    return data;
}

template<typename T>
const char* parse_number(const char* data, value_t<T>& res)
{
    bool negative = false;
    const char* beg = data;
    if (*data == '-') {
        negative = true;
        data++;
        if (!isDigit(*data)) {
            // no digits after '-'
            return data;
        }
    }
    bool fraction = false;
    bool exponent = false;

    if (*data == '0') {
        ++data;
        // after leading zero only '.', 'e' and 'E' allowed
        if (*data == '.') {
            ++data;
            fraction = true;
        } else if (*data == 'e' || *data == 'E') {
            ++data;
            exponent = true;
        }
    } else {
        while (isDigit(*data)) {
            ++data;
        }
        if (*data == '.') {
            ++data;
            fraction = true;
        } else if (*data == 'e' || *data == 'E') {
            ++data;
            exponent = true;
        }
    }

    if (fraction) {
        // fraction must start with a digit
        if (isDigit(*data)) {
            ++data;
        } else {
            // fraction missing first digit
            return data;
        }
        // fraction may contain digits up to begining of exponent ('e' or 'E')
        while (isDigit(*data)) {
            ++data;
        }
        if (*data == 'e' || *data == 'E') {
            ++data;
            exponent = true;
        }
    }
    if (exponent) {
        // exponent must start with '-' or '+' followed by a digit, or digit
        if (*data == '-' || *data == '+') {
            ++data;
        }
        if (isDigit(*data)) {
            ++data;
        } else {
            // exponent missing first digit
            return data;
        }
        while (isDigit(*data)) {
            ++data;
        }
    }
    if (data != beg) {
        char* end;
        if (fraction || exponent) {
            res = value_t<T>(strtod(beg, &end));
        } else if (negative) {
            res = value_t<T>(strtoll(beg, &end, 10));
        } else {
            res = value_t<T>(strtoull(beg, &end, 10));
        }
        return data;
    }
    // invalid json
    return data;
}

template<typename T>
const char* parse_boolean(const char* data, value_t<T>& res)
{
    if (*data == 't') {
        ++data;
        const char rue[] = { 'r', 'u', 'e' };
        for (unsigned i = 0u; i < sizeof(rue); ++i) {
            if (data[i] == 0 || data[i] != rue[i]) {
                // non-string starting with 't' but != "true"
                return data;
            }
        }

        res = value_t<T>(true);
        data += sizeof(rue);
    } else if (*data == 'f') {
        ++data;
        const char alse[] = { 'a', 'l', 's', 'e' };
        for (unsigned i = 0u; i < sizeof(alse); ++i) {
            if (data[i] == 0 || data[i] != alse[i]) {
                // non-string starting with 'f' but != "false"
                return data;
            }
        }
        res = value_t<T>(false);
        data += sizeof(alse);
    } else {
        // non-string not starting with 'f' or 't'
        return data;
    }
    return data;
}

template<typename T>
const char* parse_null(const char* data, value_t<T>& res)
{
    if (*data == 'n') {
        ++data;
        const char ull[] = { 'u', 'l', 'l' };
        for (unsigned i = 0u; i < sizeof(ull); ++i) {
            if (data[i] == 0 || data[i] != ull[i]) {
                // non-string starting with 'n' but != "null"
                return data;
            }
        }
        res = value_t<T>(typename value_t<T>::null {});
        data += sizeof(ull);
    } else {
        // invalid json
        return data;
    }
    return data;
}

template<typename T>
void add(value_t<T>& v, value_t<T>&& value)
{
    switch (v.type) {
        case type::uninitialized:
            v = BASE_NS::move(value);
            break;
        case type::object:
            v.object_.back().value = BASE_NS::move(value);
            break;
        case type::array:
            v.array_.push_back(BASE_NS::move(value));
            break;
        case type::string:
        case type::floating_point:
        case type::signed_int:
        case type::unsigned_int:
        case type::boolean:
        case type::null:
        default:
            break;
    }
}

template<typename T>
value_t<T> parse(const char* data)
{
    if (!data) {
        return {};
    }
    using jsonValue = value_t<T>;
    typename jsonValue::array stack;
    // push an uninitialized value which will get the final value during parsing
    stack.emplace_back();

    bool acceptValue = true;
    while (*data) {
        data = trim(data);
        if (*data == '{') {
            // start of an object
            if (!acceptValue) {
                return {};
            }
            data = trim(data + 1);
            if (*data == '}') {
                data = trim(data + 1);
                // handle empty object.
                add(stack.back(), jsonValue(typename jsonValue::object {}));
                acceptValue = false;
            } else if (*data == '"') {
                // try to read the key
                jsonValue key;
                data = trim(parse_string(data + 1, key));

                if (*data != ':') {
                    // missing : after key
                    return {};
                }
                data = trim(data + 1);
                // push the object with key and missing value on the stack and hope to find a value next
                stack.emplace_back(typename jsonValue::object {})
                    .object_.emplace_back(BASE_NS::move(key.string_), jsonValue {});
                acceptValue = true;
            } else {
                // missing start of key or end of object
                return {};
            }
        } else if (*data == '}') {
            // end of an object
            if (stack.back().type != type::object) {
                // unexpected }
                return {};
            }
            // check are we missing a value ('{"":}', '{"":"",}' )
            if (acceptValue) {
                return {};
            }
            data = trim(data + 1);
            // move this object to the next in the stack
            auto value = BASE_NS::move(stack.back());
            stack.pop_back();
            if (stack.empty()) {
                // invalid json
                return {};
            }
            add(stack.back(), BASE_NS::move(value));
            acceptValue = false;
        } else if (*data == '[') {
            // start of an array
            if (!acceptValue) {
                // unexpected [
                return {};
            }
            data = trim(data + 1);
            if (*data == ']') {
                data = trim(data + 1);
                // handle empty array.
                add(stack.back(), jsonValue(typename jsonValue::array {}));
                acceptValue = false;
            } else {
                // push the empty array on the stack and hope to find values
                stack.push_back(typename jsonValue::array {});
                acceptValue = true;
            }
        } else if (*data == ']') {
            // end of an array
            if (stack.back().type != type::array) {
                // unexpected ]
                return {};
            }
            // check are we missing a value ('[1,]' '[1]]')
            if (acceptValue) {
                // unexpected ]
                return {};
            }
            data = trim(data + 1);

            auto value = BASE_NS::move(stack.back());
            stack.pop_back();
            if (stack.empty()) {
                // invalid json
                return {};
            }
            add(stack.back(), BASE_NS::move(value));
            acceptValue = false;
        } else if (*data == ',') {
            // comma is allowed when the previous value was complete and we have an incomplete object or array on the
            // stack.
            if (!acceptValue && stack.back().type == type::object) {
                data = trim(data + 1);
                if (*data != '"') {
                    // missing key for next object
                    return {};
                }
                // try to read the key
                jsonValue key;
                data = trim(parse_string(data + 1, key));

                if (*data != ':') {
                    // missing value for next object
                    return {};
                }
                data = trim(data + 1);
                stack.back().object_.emplace_back(BASE_NS::move(key.string_), jsonValue {});
                acceptValue = true;
            } else if (!acceptValue && stack.back().type == type::array) {
                data = trim(data + 1);
                acceptValue = true;
            } else {
                // comma allowed only between objects and values inside an array
                return {};
            }
        } else if (*data == '"') {
            jsonValue value;
            data = trim(parse_string(data + 1, value));
            if (acceptValue && value.type == type::string) {
                add(stack.back(), BASE_NS::move(value));
                acceptValue = false;
            } else {
                // unexpected "
                return {};
            }
        } else if (isSign(*data) || isDigit(*data)) {
            jsonValue value;
            data = trim(parse_number(data, value));
            if (acceptValue && value.type != type::uninitialized) {
                add(stack.back(), BASE_NS::move(value));
                acceptValue = false;
            } else {
                // failed parsing number
                return {};
            }
        } else if ((*data == 't') || (*data == 'f')) {
            jsonValue value;
            data = trim(parse_boolean(data, value));
            if (acceptValue && value.type == type::boolean) {
                add(stack.back(), BASE_NS::move(value));
                acceptValue = false;
            } else {
                // failed parsing boolean
                return {};
            }
        } else if (*data == 'n') {
            jsonValue value;
            data = trim(parse_null(data, value));
            if (acceptValue && value.type == type::null) {
                add(stack.back(), BASE_NS::move(value));
                acceptValue = false;
            } else {
                // failed parsing null
                return {};
            }
        } else {
            // unexpected character
            return {};
        }
    }
    // check if we are missing a value ('{"":' '[')
    if (acceptValue) {
        return {};
    }

    auto value = BASE_NS::move(stack.front());
    return value;
}

template value parse(const char*);
template standalone_value parse(const char*);
// end of parser
template<typename T>
void append(BASE_NS::string& out, const typename value_t<T>::string& string)
{
    out += '"';
    out.append(escape(string));
    out += '"';
}

template<typename T>
void append(BASE_NS::string& out, const typename value_t<T>::object& object)
{
    out += '{';
    int count = 0;
    for (const auto& v : object) {
        if (count++) {
            out += ',';
        }
        append<T>(out, v.key);
        out += ':';
        out += to_string(v.value);
    }
    out += '}';
}

template<typename T>
void append(BASE_NS::string& out, const typename value_t<T>::array& array)
{
    out += '[';
    int count = 0;
    for (const auto& v : array) {
        if (count++) {
            out += ',';
        }
        out += to_string(v);
    }
    out += ']';
}

template<typename T>
void append(BASE_NS::string& out, const double floatingPoint)
{
    constexpr const char* FLOATING_FORMAT_STR = "%.17g";
    if (const int size = snprintf(nullptr, 0, FLOATING_FORMAT_STR, floatingPoint); size > 0) {
        const size_t oldSize = out.size();
        out.resize(oldSize + size);
        const size_t newSize = out.size();
        // "At most bufsz - 1 characters are written." string has size() characters + 1 for null so use size() +
        // 1 as the total size. If resize() failed string size() hasn't changed, buffer will point to the null
        // character and bufsz will be 1 i.e. only the null character will be written.
        snprintf_s(
            out.data() + oldSize, newSize + 1 - oldSize, static_cast<size_t>(size), FLOATING_FORMAT_STR, floatingPoint);
    }
}

template<typename T>
BASE_NS::string to_string(const value_t<T>& value)
{
    BASE_NS::string out;
    switch (value.type) {
        case type::uninitialized:
            break;

        case type::object:
            append<T>(out, value.object_);
            break;

        case type::array:
            append<T>(out, value.array_);
            break;

        case type::string:
            append<T>(out, value.string_);
            break;

        case type::floating_point:
            append<T>(out, value.float_);
            break;

        case type::signed_int:
            out += BASE_NS::to_string(value.signed_);
            break;

        case type::unsigned_int:
            out += BASE_NS::to_string(value.unsigned_);
            break;

        case type::boolean:
            if (value.boolean_) {
                out += "true";
            } else {
                out += "false";
            }
            break;

        case type::null:
            out += "null";
            break;

        default:
            break;
    }
    return out;
}

template BASE_NS::string to_string(const value& value);
template BASE_NS::string to_string(const standalone_value& value);

int codepoint(BASE_NS::string_view str)
{
    int code = 0;
    for (size_t u = 0U; u < 4U; ++u) {
        const char chr = str[u];
        code <<= 4U;
        code += BASE_NS::HexToDec(chr);
    }
    return code;
}

BASE_NS::string unescape(BASE_NS::string_view str)
{
    BASE_NS::string unescaped;
    unescaped.reserve(str.size());
    for (size_t i = 0, end = str.size(); i < end;) {
        auto chr = str[i];
        ++i;
        if (chr != '\\') {
            unescaped += chr;
        } else {
            chr = str[i];
            ++i;
            if (chr == '"') {
                unescaped += '"'; // Quotation mark
            } else if (chr == '\\') {
                unescaped += '\\'; // Reverse solidus
            } else if (chr == '/') {
                unescaped += '/'; // Solidus.. we do unescape this..
            } else if (chr == 'b') {
                unescaped += '\b'; // Backspace
            } else if (chr == 'f') {
                unescaped += '\f'; // Formfeed
            } else if (chr == 'n') {
                unescaped += '\n'; // Linefeed
            } else if (chr == 'r') {
                unescaped += '\r'; // Carriage return
            } else if (chr == 't') {
                unescaped += '\t';     // Horizontal tab
            } else if (chr == 'u') {   // Unicode character
                if ((i + 4U) <= end) { // Expecting 4 hexadecimal values
                    // Read the Unicode code point.
                    int code = codepoint(str.substr(i, 4U));
                    i += 4U;
                    // Codepoints U+010000 to U+10FFFF are encoded as UTF-16 surrogate pairs. High surrogate between
                    // 0xD800-0xDBFF and low between 0xDC00-0xDFFF.
                    if (code >= 0xd800 && code <= 0xdbff) {
                        // See if there's an other \uXXXX value in the correct range.
                        if ((i + 6U) <= end) {
                            auto next = str.substr(i, 6U);
                            if (next[0] == '\\' && next[1] == 'u') {
                                next.remove_prefix(2);
                                int low = codepoint(next);
                                i += 6U;
                                if (low >= 0xdc00 && low <= 0xdfff) {
                                    // Surrogate pair encoding: 0x10000 + (Hi - 0xD800) * 0x400 + (Lo - 0xDC00)
                                    code = (static_cast<int>(static_cast<unsigned int>(code) << 10U) + low -
                                            (static_cast<int>(0xd800U << 10U) + 0xdc00 - 0x10000));
                                }
                            }
                        }
                    }
                    // Convert code point to UTF-8.
                    if (code < 0x80) {
                        // 1-byte characters: 0xxxxxxx (ASCII)
                        unescaped += static_cast<char>(code);
                    } else if (code < 0x7ff) {
                        // 2-byte characters: 110xxxxx 10xxxxxx
                        unescaped += static_cast<char>(0xc0U | (static_cast<unsigned int>(code) >> 6U));
                        unescaped += static_cast<char>(0x80U | (static_cast<unsigned int>(code) & 0x3fU));
                    } else if (code <= 0xffff) {
                        // 3-byte characters: 1110xxxx 10xxxxxx 10xxxxxx
                        unescaped += static_cast<char>(0xe0U | (static_cast<unsigned int>(code) >> 12u));
                        unescaped += static_cast<char>(0x80U | ((static_cast<unsigned int>(code) >> 6U) & 0x3FU));
                        unescaped += static_cast<char>(0x80U | (static_cast<unsigned int>(code) & 0x3Fu));
                    } else {
                        // 4-byte characters: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                        unescaped += (static_cast<char>(0xf0U | (static_cast<unsigned int>(code) >> 18U)));
                        unescaped += (static_cast<char>(0x80U | ((static_cast<unsigned int>(code) >> 12U) & 0x3fU)));
                        unescaped += (static_cast<char>(0x80U | ((static_cast<unsigned int>(code) >> 6U) & 0x3fU)));
                        unescaped += (static_cast<char>(0x80U | (static_cast<unsigned int>(code) & 0x3fU)));
                    }
                }
            }
        }
    }
    return unescaped;
}

BASE_NS::string escape(BASE_NS::string_view str)
{
    BASE_NS::string escaped;
    escaped.reserve(str.size());
    for (size_t i = 0, end = str.size(); i < end;) {
        auto chr = static_cast<uint8_t>(str[i]);
        ++i;
        if (chr == '"') {
            escaped += "\\\""; // Quotation mark
        } else if (chr == '\\') {
            escaped += "\\\\"; // Reverse solidus
        } else if (chr == '\b') {
            escaped += "\\b"; // Backspace
        } else if (chr == '\f') {
            escaped += "\\f"; // Formfeed
        } else if (chr == '\n') {
            escaped += "\\n"; // Linefeed
        } else if (chr == '\r') {
            escaped += "\\r"; // Carriage return
        } else if (chr == '\t') {
            escaped += "\\t"; // Horizontal tab
        } else if (chr < 0x80) {
            escaped += static_cast<BASE_NS::string::value_type>(chr); // 1-byte characters: 0xxxxxxx (ASCII)
        } else {
            // Unicode
            unsigned code = 0U;
            unsigned left = 0U;
            // Decode first byte and figure out how many additional bytes are needed for the codepoint.
            if (chr < 0xE0) {
                // 2-byte characters: 110xxxxx 10xxxxxx
                if (i < end) {
                    code = ((chr & ~0xC0U));
                    left = 1U;
                }
            } else if (chr < 0xF0) {
                // 3-byte characters: 1110xxxx 10xxxxxx 10xxxxxx
                if ((i + 1U) < end) {
                    code = (chr & ~0xE0U);
                    left = 2U;
                }
            } else if (chr < 0xF8) {
                // 4-byte characters: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                if ((i + 2U) < end) {
                    code = (chr & ~0xF0U);
                    left = 3U;
                }
            }

            // Combine the remaining bytes.
            while (left--) {
                code = (code << 6U) + (str[i++] & 0x3FU);
            }

            // Codepoints U+010000 to U+10FFFF are encoded as UTF-16 surrogate pairs.
            // Surrogate pair encoding: 0x10000 + (Hi - 0xD800) * 0x400 + (Lo - 0xDC00)
            if (code >= 0x010000 && code <= 0x10FFFF) {
                // First append the Hi value.
                code -= 0x10000U;
                const auto hi = (code >> 10U) + 0xD800U;
                escaped += '\\';
                escaped += 'u';
                escaped += BASE_NS::to_hex(hi);

                // Calculate the Lo value.
                code = (code & 0x3FFU) + 0xDC00U;
            }

            // Append the codepoint zero padded to four hex values.
            escaped += '\\';
            escaped += 'u';
            const auto codepoint = BASE_NS::to_hex(code);
            if (codepoint.size() < 4U) {
                escaped.append(4U - codepoint.size(), '0');
            }
            escaped += codepoint;
        }
    }
    return escaped;
}
} // namespace json
CORE_END_NAMESPACE()
#endif // JSON_IMPL

#endif // !API_CORE_JSON_JSON_H
