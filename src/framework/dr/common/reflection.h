/*
 * Tencent is pleased to support the open source community by making Pebble available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 *
 */


#ifndef PEBBLE_DR_REFLECTION_H
#define PEBBLE_DR_REFLECTION_H

#include <string>
#include <tr1/unordered_map>
#include <map>
#include "framework/dr/protocol/protocol.h"

namespace pebble { namespace dr { namespace reflection {

/// @brief 字段序列化接口返回码
enum PackError {
    kINSUFFICIENTBUFFER = -1, ///< 给的BUFFER不足打包
    kINVALIDBUFFER = -2, ///< 无法解包
    kINVALIDOBJECT = -3, ///< 对象类型非法
    kINVALIDPARAMETER = -4, ///< 参数非法
    kUNKNOW = -99 ///< 未知错误
};


typedef std::map<std::string, std::map<std::string, std::string> > AttributesType;

/// @brief 支持注解的反射信息基类
class Attributable {
public:
    /// @brief 获取注解
    /// @param 注解名字
    /// @return 注解信息，一个map，对于IDL定义注解的括号部分
    inline const std::map<std::string, std::string>* GetAttribute(
            const std::string &attribute_name) const {
        AttributesType::const_iterator iter = m_attributes.find(attribute_name);
        if (iter == m_attributes.end()) {
            return NULL;
        }
        return &(iter->second);
    }

    explicit Attributable(const AttributesType &attributes)
        : m_attributes(attributes) {}

private:
    AttributesType m_attributes;
};

/// @brief 字段反射信息基类
class FieldInfo : public Attributable {
public:
    /// @brief 获取字段类型
    /// @return 字段类型
    inline pebble::dr::protocol::TType GetFieldType() const {
        return m_field_type;
    }

    /// @brief 获取字段ID
    /// @return 字段ID
    inline int16_t GetFieldId() const {
        return m_field_id;
    }

    /// @brief 获取字段名字
    /// @return 字段名字
    inline const std::string &GetFieldName() const {
        return m_field_name;
    }

    /// @brief 字段是否可选
    /// @return 字段是否可选
    inline bool IsOptional() const {
        return m_is_optional;
    }

    /// @brief 字段打包
    /// @param obj 要打包的对象
    /// @param buff 打包后对象放在这个buffer里
    /// @param buff_len buff的长度
    /// @return >0表示成功，值等于使用的buff。<=0表示失败, 具体看@ref PackError
    virtual int Pack(void *obj, uint8_t *buff, uint32_t buff_len) const = 0;

    /// @brief 字段解包
    /// @param obj 解包字段后放置的对象
    /// @param buff 要解包的buffer
    /// @param buff_len buff的长度
    /// @return >0表示成功，值等于使用的buff。<=0表示失败, 具体看@ref PackError
    virtual int UnPack(void *obj, uint8_t *buff, uint32_t buff_len) const = 0;

    inline int Pack(void *obj, char *buff, size_t buff_len) const {
        return Pack(obj, reinterpret_cast<uint8_t*>(buff), buff_len);
    }

    inline int UnPack(void *obj, char *buff, size_t buff_len) const {
        return UnPack(obj, reinterpret_cast<uint8_t*>(buff), buff_len);
    }

    FieldInfo(std::string field_name, int16_t field_id,
            pebble::dr::protocol::TType field_type,
            bool is_optional, AttributesType attributes)
        : Attributable(attributes),
          m_field_name(field_name),
          m_field_id(field_id),
          m_field_type(field_type),
          m_is_optional(is_optional)
    {
    }

    virtual ~FieldInfo() {}

private:
    std::string m_field_name;

    int16_t m_field_id;

    pebble::dr::protocol::TType m_field_type;

    bool m_is_optional;
};

/// @brief 反射信息
class TypeInfo : public Attributable {
public:
    /// @brief 获取所有字段的信息
    /// @return 返回字段名到字段信息的unordered_map
    inline const std::tr1::unordered_map<std::string, FieldInfo *>* GetFieldInfos() const {
        return &m_field_infos;
    }

    TypeInfo(std::tr1::unordered_map<std::string, FieldInfo *> field_infos,
            AttributesType attributes)
        : Attributable(attributes)
          , m_field_infos(field_infos)
    {
    }

private:
    std::tr1::unordered_map<std::string, FieldInfo *> m_field_infos;
};

template<typename T>
const TypeInfo* GetTypeInfo() {
    return T::S_GetTypeInfo();
}

template<typename T>
const TypeInfo* GetTypeInfo(T &obj) { // NOLINT
    return T::S_GetTypeInfo();
}

} // namespace reflection
} // namespace dr
} // namespace pebble

#endif

