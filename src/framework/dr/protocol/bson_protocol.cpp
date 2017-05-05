/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdlib.h>
#include <string.h>

#include "framework/dr/protocol/bson_protocol.h"

# if defined(__GNUC__) && defined(__GLIBC__)
#  include <byteswap.h>
#else /* GNUC & GLIBC */
#  define bswap_16(n) \
    ( (((n) & 0xff00u) >> 8) \
    | (((n) & 0x00ffu) << 8) )
#  define bswap_32(n) \
    ( (((n) & 0xff000000u) >> 24)  \
    | (((n) & 0x00ff0000u) >> 8) \
    | (((n) & 0x0000ff00u) << 8) \
    | (((n) & 0x000000ffu) << 24) )
#endif

/*
 * bson must be little endian
 */
#if __THRIFT_BYTE_ORDER == __THRIFT_BIG_ENDIAN
#define htole_16(n) bswap_16(n)
#define htole_32(n) bswap_32(n)
#define htole_64(n) bswap_64(n)
#define letoh_16(n) bswap_16(n)
#define letoh_32(n) bswap_32(n)
#define letoh_64(n) bswap_64(n)
#elif __THRIFT_BYTE_ORDER == __THRIFT_LITTLE_ENDIAN
#define htole_16(n) (n)
#define htole_32(n) (n)
#define htole_64(n) (n)
#define letoh_16(n) (n)
#define letoh_32(n) (n)
#define letoh_64(n) (n)
#else /* __THRIFT_BYTE_ORDER */
# error "Can't define htonll or ntohll!"
#endif


using namespace pebble::dr::transport;

namespace pebble { namespace dr { namespace protocol {

// Static data

static const uint32_t kBSONMaxLength = 16 * 1024 * 1024;

static const int8_t kBSONDoubleFieldType = 0x01;
static const int8_t kBSONStringFieldType = 0x02;
static const int8_t kBSONDocumentFieldType = 0x03;
static const int8_t kBSONBinaryFieldType = 0x05;
static const int8_t kBSONBoolFieldType = 0x08;
static const int8_t kBSONEmptyFieldType = 0x0A;
static const int8_t kBSONInt32FieldType = 0x10;
static const int8_t kBSONInt64FieldType = 0x12;

static const uint8_t kBSONBinaryUserType = 0x80;

static const int8_t kBSONThriftVersion = 1;

static const int8_t kBSONStop[2] = { 0x0A, 0 };
static const int8_t kBsonDocInitStru[2] = { 0x03, 0 };

/*
    thrift protocol类型 --> BSON类型表
    0  TType::T_STOP    --> kBSONEmptyFieldType
    1  TType::T_VOID    --> throw error
    2  TType::T_BOOL    --> kBSONBoolFieldType
    3  TType::T_BYTE    --> kBSONInt32FieldType
    3  TType::T_I08     --> kBSONInt32FieldType
    6  TType::T_I16     --> kBSONInt32FieldType
    8  TType::T_I32     --> kBSONInt32FieldType
    9  TType::T_U64     --> throw error
    10 TType::T_I64     --> kBSONInt64FieldType
    4  TType::T_DOUBLE  --> kBSONDoubleFieldType
    11 TType::T_STRING  --> kBSONStringFieldType
    11 TType::T_UTF7    --> kBSONStringFieldType
    12 TType::T_STRUCT  --> kBSONDocumentFieldType
    13 TType::T_MAP     --> kBSONBinaryFieldType
    14 TType::T_SET     --> kBSONBinaryFieldType
    15 TType::T_LIST    --> kBSONBinaryFieldType
    16 TType::T_UTF8    --> throw error
    17 TType::T_UTF16   --> throw error
*/
static const int8_t kTType2BSONTypeTable[0x20] =
{
//  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x0A, 0,    0x08, 0x10, 0x01, 0,    0x10, 0,    0x10, 0,    0x12, 0x02, 0x03, 0x05, 0x05, 0x05,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

#pragma pack(push, 1)
struct BSONMessageNameHeader
{
    int8_t  _seq_field;
    int8_t  _type;
    int8_t  _separator1;
    int64_t _seq;
    int8_t  _name_field;
    int8_t  _version;
    int8_t  _separator2;

    void SetHeader(TMessageType message_type, int64_t seq_id)
    {
        _seq_field = kBSONInt64FieldType;
        _type = static_cast<int8_t>(message_type);
        _separator1 = 0;
        _seq = seq_id;
        _name_field = kBSONStringFieldType;
        _version = kBSONThriftVersion;
        _separator2 = 0;
    }

    // check if the head is valid
    bool IsValid() const
    {
        return (kBSONInt64FieldType == _seq_field)
            && (0 != _type)
            && (0 == _separator1)
            && (kBSONStringFieldType == _name_field)
            && (0 == _separator2);
    }
};

struct BSONBinaryHeader
{
    int32_t  _binary_length;
    uint8_t  _binary_type;

    bool IsValid() const
    {
        return (kBSONBinaryUserType == _binary_type);
    }
};

struct BSONMapHeader
{
    int8_t   _head_field;
    int8_t   _key_type;
    int8_t   _val_type;
    int8_t   _separator;
    uint32_t _size;

    void SetHeader(TType key_type, TType val_type, uint32_t size)
    {
        _head_field = kBSONInt32FieldType;
        _key_type = static_cast<int8_t>(key_type);
        _val_type = static_cast<int8_t>(val_type);
        _separator = 0;
        _size = size;
    }

    // check if the head is valid
    bool IsValid() const
    {
        return (kBSONInt32FieldType == _head_field)
            && (0 != _key_type)
            && (0 != _val_type)
            && (0 == _separator);
    }
};

struct BSONListHeader
{
    int8_t   _head_field;
    int8_t   _ele_type;
    int8_t   _separator;
    uint32_t _size;

    void SetHeader(TType ele_type, uint32_t size)
    {
        _head_field = kBSONInt32FieldType;
        _ele_type = static_cast<int8_t>(ele_type);
        _separator = 0;
        _size = size;
    }

    // check if the head is valid
    bool IsValid() const
    {
        return (kBSONInt32FieldType == _head_field)
            && (0 != _ele_type)
            && (0 == _separator);
    }
};
#pragma pack(pop)

static int8_t getBsonFieldType(TType ttype)
{
    int8_t bson_type = kTType2BSONTypeTable[ttype];
    if (0 == bson_type)
    {
        throw TProtocolException(TProtocolException::NOT_IMPLEMENTED,
            "Unrecognized type");
    }

    return bson_type;
}

TBSONProtocol::WriteBuff::~WriteBuff()
{
    if (NULL != _str_buff)
    {
        free(_str_buff);
    }
}

void TBSONProtocol::WriteBuff::clear()
{
    _write_pos = _str_buff;
}

uint32_t TBSONProtocol::WriteBuff::write(const char* buff, uint32_t length)
{
    if (_write_pos + length > _capacity)
    {
        resize(length);
    }

    memcpy(_write_pos, buff, length);
    _write_pos += length;
    return length;
}

char* TBSONProtocol::WriteBuff::alloc(uint32_t length)
{
    if (_write_pos + length > _capacity)
    {
        resize(length);
    }

    char* alloc_pos = _write_pos;
    _write_pos += length;
    return alloc_pos;
}

char* TBSONProtocol::WriteBuff::data()
{
    return _str_buff;
}

uint32_t TBSONProtocol::WriteBuff::size()
{
    return static_cast<uint32_t>(_write_pos - _str_buff);
}

void TBSONProtocol::WriteBuff::resize(uint32_t length)
{
    uint32_t write_pos = static_cast<uint32_t>(_write_pos - _str_buff);
    uint32_t old_length = static_cast<uint32_t>(_capacity - _str_buff);
    uint32_t req_length = ((length >> 8) + ((length & 0xFF) ? 1 : 0)) << 8;
    uint32_t new_length = ((req_length > old_length) ? (req_length * 2) : (old_length * 2));
    if (new_length > kBSONMaxLength)
    {
        throw TProtocolException(TProtocolException::SIZE_LIMIT);
    }

    if (NULL == _str_buff)
    {
        _str_buff = _write_pos = reinterpret_cast<char*>(malloc(new_length));
        _capacity = _str_buff + new_length;
    }
    else
    {
        _str_buff = reinterpret_cast<char*>(realloc(_str_buff, new_length));
        _write_pos = _str_buff + write_pos;
        _capacity = _str_buff + new_length;
    }
}

void TBSONProtocol::ReadBuff::clear()
{
    _read_ptr = NULL;
    while (!_dom_length.empty())
    {
        _dom_length.pop();
    }
}

void TBSONProtocol::ReadBuff::borrow(uint32_t length)
{
    if (length > kBSONMaxLength)
    {
        throw TProtocolException(TProtocolException::SIZE_LIMIT);
    }

    _dom_length.push(length);
    if (1 == _dom_length.size() && NULL == _read_ptr)
    {
        _read_ptr = _trans->borrow(NULL, &length);
    }
}

void TBSONProtocol::ReadBuff::consume()
{
    if (1 == _dom_length.size() && NULL != _read_ptr)
    {
        _trans->consume(_dom_length.top());
        _read_ptr = NULL;
    }
    // TODO(lamby): check read length == dom length
    _dom_length.pop();
}

const uint8_t* TBSONProtocol::ReadBuff::readAll(uint8_t* buf, uint32_t length)
{
    const uint8_t* ret_ptr = _read_ptr;
    if (NULL != _read_ptr)
    {
        _read_ptr += length;
        return ret_ptr;
    }
    if (NULL != buf)
    {
        _trans->readAll(buf, length);
    }

    return buf;
}



TBSONProtocol::TBSONProtocol(cxx::shared_ptr<TTransport> ptrans)
    :   TVirtualProtocol<TBSONProtocol>(ptrans), m_trans(ptrans.get()), m_read_buff(ptrans.get())
{
}

TBSONProtocol::~TBSONProtocol()
{
}

uint32_t TBSONProtocol::writeMessageBegin(const std::string& name,
                                          const TMessageType messageType,
                                          const int64_t seqid)
{
    m_write_buff.clear();
    uint32_t result = writeBsonDocumentBegin();

    BSONMessageNameHeader* header = reinterpret_cast<BSONMessageNameHeader*>(
                    m_write_buff.alloc(sizeof(BSONMessageNameHeader)));
    header->SetHeader(messageType, htole_64(seqid));
    result += sizeof(BSONMessageNameHeader);

    result += writeString(name);

    // append for the struct begin next the message begin
    m_write_buff.write(reinterpret_cast<const char*>(kBsonDocInitStru), 2);
    return (result + 2);
}

uint32_t TBSONProtocol::writeMessageEnd()
{
    uint32_t result = writeBsonDocumentEnd();

    return result;
}

uint32_t TBSONProtocol::writeStructBegin(const char* name)
{
    return writeBsonDocumentBegin();
}

uint32_t TBSONProtocol::writeStructEnd()
{
    return writeBsonDocumentEnd();
}

uint32_t TBSONProtocol::writeFieldBegin(const char* name,
                                        const TType fieldType,
                                        const int16_t fieldId)
{
    // write field type and name
    return writeBsonFieldName(fieldType, fieldId);
}

uint32_t TBSONProtocol::writeFieldEnd()
{
    return 0;
}

uint32_t TBSONProtocol::writeFieldStop()
{
    return writeBsonFieldName(T_STOP, 0);
}

uint32_t TBSONProtocol::writeMapBegin(const TType keyType, const TType valType, const uint32_t size)
{
    uint32_t result = writeBsonBinaryBegin();

    // write map header
    BSONMapHeader* map_header = reinterpret_cast<BSONMapHeader*>(
                m_write_buff.alloc(sizeof(BSONMapHeader)));
    map_header->SetHeader(keyType, valType, htole_32(size));
    result += sizeof(BSONMapHeader);

    return result;
}

uint32_t TBSONProtocol::writeMapEnd()
{
    return writeBsonBinaryEnd();
}

uint32_t TBSONProtocol::writeListBegin(const TType elemType, const uint32_t size)
{
    uint32_t result = writeBsonBinaryBegin();

    // write map header
    BSONListHeader* list_header = reinterpret_cast<BSONListHeader*>(
        m_write_buff.alloc(sizeof(BSONListHeader)));
    list_header->SetHeader(elemType, htole_32(size));
    result += sizeof(BSONListHeader);

    return result;
}

uint32_t TBSONProtocol::writeListEnd()
{
    return writeBsonBinaryEnd();
}

uint32_t TBSONProtocol::writeSetBegin(const TType elemType, const uint32_t size)
{
    uint32_t result = writeBsonBinaryBegin();

    // write map header
    BSONListHeader* set_header = reinterpret_cast<BSONListHeader*>(
        m_write_buff.alloc(sizeof(BSONListHeader)));
    set_header->SetHeader(elemType, htole_32(size));
    result += sizeof(BSONListHeader);

    return result;
}

uint32_t TBSONProtocol::writeSetEnd()
{
    return writeBsonBinaryEnd();
}

uint32_t TBSONProtocol::writeBool(const bool value)
{
    char tmp = (value ? 1 : 0);
    m_write_buff.write(&tmp, 1);
    return 1;
}

uint32_t TBSONProtocol::writeByte(const int8_t byte)
{
    int32_t val = htole_32(static_cast<int32_t>(byte));
    m_write_buff.write(reinterpret_cast<const char*>(&val), sizeof(val));
    return 4;
}

uint32_t TBSONProtocol::writeI16(const int16_t i16)
{
    int32_t val = htole_32(static_cast<int32_t>(i16));
    m_write_buff.write(reinterpret_cast<const char*>(&val), sizeof(val));
    return 4;
}

uint32_t TBSONProtocol::writeI32(const int32_t i32)
{
    int32_t val = htole_32(i32);
    m_write_buff.write(reinterpret_cast<const char*>(&val), sizeof(val));
    return 4;
}

uint32_t TBSONProtocol::writeI64(const int64_t i64)
{
    int64_t val = htole_64(i64);
    m_write_buff.write(reinterpret_cast<const char*>(&val), sizeof(val));
    return 8;
}

uint32_t TBSONProtocol::writeDouble(const double dub)
{
    int64_t val = htole_64(bitwise_cast<uint64_t>(dub));
    m_write_buff.write(reinterpret_cast<const char*>(&val), sizeof(val));
    return 8;
}

uint32_t TBSONProtocol::writeString(const std::string& str)
{
    int32_t length = static_cast<int32_t>(str.size()) + 1;
    writeI32(length);

    m_write_buff.write(str.c_str(), length);
    return (length + 4);
}

uint32_t TBSONProtocol::writeBinary(const std::string& str)
{
    return writeString(str);
}

uint32_t TBSONProtocol::readMessageBegin(std::string& name, // NOLINT
                                         TMessageType& messageType,
                                         int64_t& seqid)
{
    m_read_buff.clear();
    uint32_t result = readBsonDocumentBegin();

    BSONMessageNameHeader header;
    const BSONMessageNameHeader* header_ptr = reinterpret_cast<const BSONMessageNameHeader*>(
            m_read_buff.readAll(reinterpret_cast<uint8_t*>(&header), sizeof(header)));
    result += sizeof(header);
    if (false == header_ptr->IsValid())
    {
        throw TProtocolException(TProtocolException::NOT_IMPLEMENTED,
            "Unrecognized type");
    }

    messageType = static_cast<TMessageType>(header_ptr->_type);
    seqid = letoh_64(header_ptr->_seq);
    result += readString(name);

    // read for the struct begin next the message begin
    uint8_t next_struct_field[2];
    m_read_buff.readAll(next_struct_field, 2);

    return (result + 2);
}

uint32_t TBSONProtocol::readMessageEnd()
{
    return readBsonDocumentEnd();
}

uint32_t TBSONProtocol::readStructBegin(std::string& name) // NOLINT
{
    return readBsonDocumentBegin();
}

uint32_t TBSONProtocol::readStructEnd()
{
    return readBsonDocumentEnd();
}

uint32_t TBSONProtocol::readFieldBegin(std::string& name, TType& fieldType, int16_t& fieldId) // NOLINT
{
    // read field type and name
    return readBsonFieldName(fieldType, fieldId);
}

uint32_t TBSONProtocol::readFieldEnd()
{
    return 0;
}

uint32_t TBSONProtocol::readMapBegin(TType& keyType, TType& valType, uint32_t& size) // NOLINT
{
    uint32_t result = readBsonBinaryBegin();

    BSONMapHeader map_header;
    const BSONMapHeader* map_header_ptr = reinterpret_cast<const BSONMapHeader*>(
            m_read_buff.readAll(reinterpret_cast<uint8_t*>(&map_header), sizeof(map_header)));
    result += sizeof(map_header);
    if (false == map_header_ptr->IsValid())
    {
        throw TProtocolException(TProtocolException::NOT_IMPLEMENTED,
            "Unrecognized type");
    }

    keyType = static_cast<TType>(map_header_ptr->_key_type);
    valType = static_cast<TType>(map_header_ptr->_val_type);
    size = letoh_32(map_header_ptr->_size);
    return result;
}

uint32_t TBSONProtocol::readMapEnd()
{
    return 0;
}

uint32_t TBSONProtocol::readListBegin(TType& elemType, uint32_t& size) // NOLINT
{
    uint32_t result = readBsonBinaryBegin();

    BSONListHeader list_header;
    const BSONListHeader* list_header_ptr = reinterpret_cast<const BSONListHeader*>(
            m_read_buff.readAll(reinterpret_cast<uint8_t*>(&list_header), sizeof(list_header)));
    result += sizeof(list_header);
    if (false == list_header_ptr->IsValid())
    {
        throw TProtocolException(TProtocolException::NOT_IMPLEMENTED,
            "Unrecognized type");
    }

    elemType = static_cast<TType>(list_header_ptr->_ele_type);
    size = letoh_32(list_header_ptr->_size);
    return result;
}

uint32_t TBSONProtocol::readListEnd()
{
    return 0;
}

uint32_t TBSONProtocol::readSetBegin(TType& elemType, uint32_t& size) // NOLINT
{
    uint32_t result = readBsonBinaryBegin();

    BSONListHeader set_header;
    const BSONListHeader* set_header_ptr = reinterpret_cast<const BSONListHeader*>(
            m_read_buff.readAll(reinterpret_cast<uint8_t*>(&set_header), sizeof(set_header)));
    result += sizeof(set_header);
    if (false == set_header_ptr->IsValid())
    {
        throw TProtocolException(TProtocolException::NOT_IMPLEMENTED,
            "Unrecognized type");
    }

    elemType = static_cast<TType>(set_header_ptr->_ele_type);
    size = letoh_32(set_header_ptr->_size);
    return result;
}

uint32_t TBSONProtocol::readSetEnd()
{
    return 0;
}

uint32_t TBSONProtocol::readBool(bool& value) // NOLINT
{
    uint8_t b;
    const uint8_t* val_ptr = m_read_buff.readAll(&b, 1);
    value = ((*val_ptr) != 0);
    return 1;
}

uint32_t TBSONProtocol::readByte(int8_t& byte) // NOLINT
{
    int32_t val;
    const int32_t* val_ptr = reinterpret_cast<const int32_t*>(
            m_read_buff.readAll(reinterpret_cast<uint8_t*>(&val), 4));
    byte = static_cast<int8_t>(letoh_32(*val_ptr));
    return 4;
}

uint32_t TBSONProtocol::readI16(int16_t& i16) // NOLINT
{
    int32_t val;
    const int32_t* val_ptr = reinterpret_cast<const int32_t*>(
            m_read_buff.readAll(reinterpret_cast<uint8_t*>(&val), 4));
    i16 = static_cast<int16_t>(letoh_32(*val_ptr));
    return 4;
}

uint32_t TBSONProtocol::readI32(int32_t& i32) // NOLINT
{
    int32_t val;
    const int32_t* val_ptr = reinterpret_cast<const int32_t*>(
            m_read_buff.readAll(reinterpret_cast<uint8_t*>(&val), 4));
    i32 = letoh_32(*val_ptr);
    return 4;
}

uint32_t TBSONProtocol::readI64(int64_t& i64) // NOLINT
{
    int64_t val;
    const int64_t* val_ptr = reinterpret_cast<const int64_t*>(
            m_read_buff.readAll(reinterpret_cast<uint8_t*>(&val), 8));
    i64 = letoh_64(*val_ptr);
    return 8;
}

uint32_t TBSONProtocol::readDouble(double& dub) // NOLINT
{
    int64_t val;
    const int64_t* val_ptr = reinterpret_cast<const int64_t*>(
            m_read_buff.readAll(reinterpret_cast<uint8_t*>(&val), 8));
    dub = bitwise_cast<double>(letoh_64(*val_ptr));
    return 8;
}

uint32_t TBSONProtocol::readString(std::string& str) // NOLINT
{
    int32_t length = 0;
    readI32(length);
    if (kBSONMaxLength < static_cast<uint32_t>(length))
    {
        throw TProtocolException(TProtocolException::SIZE_LIMIT, "invalid string length");
    }

    const uint8_t* borrow_buf = m_read_buff.readAll(NULL, length);
    if (NULL != borrow_buf)
    {
        str.assign(reinterpret_cast<const char*>(borrow_buf), length - 1); // exclude '\0' end
    }
    else
    {
        str.resize(length);
        m_read_buff.readAll(reinterpret_cast<uint8_t*>(&str[0]), length);
        str.resize(length - 1); // exclude '\0' end
    }

    return (length + 4);
}

uint32_t TBSONProtocol::readBinary(std::string& str) // NOLINT
{
    return readString(str);
}

/**
* Bson functions.
*/

uint32_t TBSONProtocol::writeBsonDocumentBegin()
{
    m_length_pos.push(static_cast<int32_t>(m_write_buff.size()));
    m_write_buff.alloc(4);
    return 4;
}

uint32_t TBSONProtocol::writeBsonDocumentEnd()
{
    m_write_buff.write("\0", 1); // add document end

    // write back the document length
    int32_t doc_length = static_cast<int32_t>(m_write_buff.size()) - m_length_pos.top();
    int32_t* length_ptr = reinterpret_cast<int32_t*>(m_write_buff.data() + m_length_pos.top());
    *length_ptr = htole_32(doc_length);
    m_length_pos.pop();

    // stack为空，可以写出到transport
    if (0 == m_length_pos.size())
    {
        m_trans->write(reinterpret_cast<const uint8_t*>(m_write_buff.data()),
            static_cast<uint32_t>(m_write_buff.size()));
        m_write_buff.clear();
    }

    return 1;
}

uint32_t TBSONProtocol::writeBsonBinaryBegin()
{
    m_length_pos.push(static_cast<int32_t>(m_write_buff.size()));

    BSONBinaryHeader* binary_header = reinterpret_cast<BSONBinaryHeader*>(
        m_write_buff.alloc(sizeof(BSONBinaryHeader)));

    // binary length not include binary_header
    binary_header->_binary_type = kBSONBinaryUserType;

    return sizeof(BSONBinaryHeader);
}

uint32_t TBSONProtocol::writeBsonBinaryEnd()
{
    // write back the binary length
    // note : binary length not include the binary header
    int32_t binary_length = static_cast<int32_t>(m_write_buff.size()) -
                m_length_pos.top() - sizeof(BSONBinaryHeader);
    int32_t* length_ptr = reinterpret_cast<int32_t*>(m_write_buff.data() + m_length_pos.top());
    *length_ptr = htole_32(binary_length);
    m_length_pos.pop();

    return 0;
}

uint32_t TBSONProtocol::writeBsonFieldName(const TType fieldType, const int16_t fieldId)
{
    // bson_field[0] : bson field type
    // bson_field[1] : protocol field type，protocol的类型转为bson类型有损失，需要记录原始类型
    // bson_field[2..4] : protocol field id，bson field name为cstring，需要将fieldID转为3个非0的字节
    // 将fieldID的16bit按5，5，6分割到3个byte，保证每个byte都不为0
    // bson_field[5] : bson field name的结束符
    if (T_STOP == fieldType)
    {
        m_write_buff.write(reinterpret_cast<const char*>(kBSONStop), 2);
        return 2;
    }

    char *bson_field = m_write_buff.alloc(6);
    bson_field[0] = getBsonFieldType(fieldType);
    bson_field[1] = static_cast<char>(fieldType);

    int16_t bson_field_id = htole_16(fieldId);
    bson_field[2] = static_cast<char>(bson_field_id >> 11) | 0x40;    // [11,15] bits
    bson_field[3] = static_cast<char>(bson_field_id >> 6) | 0x40;     // [6,10] bits
    bson_field[4] = static_cast<char>(bson_field_id) | 0x40;          // [0,5] bits
    bson_field[5] = '\0';

    return 6;
}

uint32_t TBSONProtocol::readBsonDocumentBegin()
{
    int32_t doc_length;
    uint32_t result = readI32(doc_length);

    // borrow length should decrease sizeof(length) of document
    m_read_buff.borrow(static_cast<uint32_t>(doc_length - sizeof(doc_length)));

    return result;
}

uint32_t TBSONProtocol::readBsonDocumentEnd()
{
    uint8_t byte_val;
    m_read_buff.readAll(&byte_val, 1);

    m_read_buff.consume();

    return 1;
}

uint32_t TBSONProtocol::readBsonBinaryBegin()
{
    BSONBinaryHeader binary_header;
    m_read_buff.readAll(reinterpret_cast<uint8_t*>(&binary_header), sizeof(binary_header));

    return sizeof(binary_header);
}

uint32_t TBSONProtocol::readBsonBinaryEnd()
{
    return 0;
}

uint32_t TBSONProtocol::readBsonFieldName(TType& fieldType, int16_t& fieldId) // NOLINT
{
    // bson_field[0] : bson field type
    // bson_field[1] : protocol field type，protocol的类型转为bson类型有损失，需要记录原始类型
    // bson_field[2..4] : protocol field id，bson field name为cstring，需要将fieldID转为3个非0的字节
    // 将fieldID的16bit按5，5，6分割到3个byte，保证每个byte都不为0
    // bson_field[5] : bson field name的结束符
    uint8_t bson_field_type[2];
    const uint8_t* field_type = m_read_buff.readAll(bson_field_type, 2);
    fieldType = static_cast<TType>(field_type[1]);
    if (T_STOP == fieldType)
    {
        return 2;
    }
    if (field_type[0] != getBsonFieldType(fieldType))
    {
        throw TProtocolException(TProtocolException::NOT_IMPLEMENTED,
            "Unrecognized type");
    }

    uint8_t bson_field_id[4];
    const uint8_t* field_id = m_read_buff.readAll(bson_field_id, 4);
    fieldId = (field_id[0] & 0x1F);
    fieldId = ((fieldId << 5) | (field_id[1] & 0x1F));
    fieldId = ((fieldId << 6) | (field_id[2] & 0x3F));
    fieldId = letoh_16(fieldId);

    return 6;
}

}}} // pebble::dr::protocol // NOLINT
