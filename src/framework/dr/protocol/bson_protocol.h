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

#ifndef PEBBLE_DR_PROTOCOL_BSONPROTOCOL_H
#define PEBBLE_DR_PROTOCOL_BSONPROTOCOL_H

#include <stack>
#include <string>

#include "framework/dr/protocol/virtual_protocol.h"

namespace pebble { namespace dr { namespace protocol {

class TBSONProtocol : public TVirtualProtocol<TBSONProtocol>
{
public:
    TBSONProtocol(cxx::shared_ptr<TTransport> ptrans);

    ~TBSONProtocol();

    /**
    * Writing functions.
    */

    uint32_t writeMessageBegin(const std::string& name,
                                const TMessageType messageType,
                                const int64_t seqid);

    uint32_t writeMessageEnd();

    uint32_t writeStructBegin(const char* name);

    uint32_t writeStructEnd();

    uint32_t writeFieldBegin(const char* name,
                             const TType fieldType,
                             const int16_t fieldId);

    uint32_t writeFieldEnd();

    uint32_t writeFieldStop();

    uint32_t writeMapBegin(const TType keyType,
                           const TType valType,
                           const uint32_t size);

    uint32_t writeMapEnd();

    uint32_t writeListBegin(const TType elemType,
                            const uint32_t size);

    uint32_t writeListEnd();

    uint32_t writeSetBegin(const TType elemType,
                            const uint32_t size);

    uint32_t writeSetEnd();

    uint32_t writeBool(const bool value);

    uint32_t writeByte(const int8_t byte);

    uint32_t writeI16(const int16_t i16);

    uint32_t writeI32(const int32_t i32);

    uint32_t writeI64(const int64_t i64);

    uint32_t writeDouble(const double dub);

    uint32_t writeString(const std::string& str);

    uint32_t writeBinary(const std::string& str);

    /**
    * Reading functions
    */

    uint32_t readMessageBegin(std::string& name,
                            TMessageType& messageType,
                            int64_t& seqid);

    uint32_t readMessageEnd();

    uint32_t readStructBegin(std::string& name);

    uint32_t readStructEnd();

    uint32_t readFieldBegin(std::string& name,
                            TType& fieldType,
                            int16_t& fieldId);

    uint32_t readFieldEnd();

    uint32_t readMapBegin(TType& keyType,
                        TType& valType,
                        uint32_t& size);

    uint32_t readMapEnd();

    uint32_t readListBegin(TType& elemType,
                            uint32_t& size);

    uint32_t readListEnd();

    uint32_t readSetBegin(TType& elemType,
                        uint32_t& size);

    uint32_t readSetEnd();

    uint32_t readBool(bool& value);

    // Provide the default readBool() implementation for std::vector<bool>
    using TVirtualProtocol<TBSONProtocol>::readBool;

    uint32_t readByte(int8_t& byte);

    uint32_t readI16(int16_t& i16);

    uint32_t readI32(int32_t& i32);

    uint32_t readI64(int64_t& i64);

    uint32_t readDouble(double& dub);

    uint32_t readString(std::string& str);

    uint32_t readBinary(std::string& str);

private:
    uint32_t writeBsonDocumentBegin();

    uint32_t writeBsonDocumentEnd();

    uint32_t writeBsonFieldName(const TType fieldType, const int16_t fieldId);

    uint32_t writeBsonBinaryBegin();

    uint32_t writeBsonBinaryEnd();

    uint32_t readBsonDocumentBegin();

    uint32_t readBsonDocumentEnd();

    uint32_t readBsonBinaryBegin();

    uint32_t readBsonBinaryEnd();

    uint32_t readBsonFieldName(TType& fieldType, int16_t& fieldId);

    TTransport*             m_trans;

    struct WriteBuff
    {
        WriteBuff()
            :   _str_buff(NULL), _write_pos(NULL), _capacity(NULL)
        {
        }

        ~WriteBuff();

        void clear();

        uint32_t write(const char* buff, uint32_t length);

        char* alloc(uint32_t length);

        char* data();

        uint32_t size();

        void resize(uint32_t length);

        char*               _str_buff;
        char*               _write_pos;
        char*               _capacity;
    };

    WriteBuff               m_write_buff;   // 写buffer
    std::stack<int32_t>     m_length_pos;   // 待回写的长度位置

    struct ReadBuff
    {
        ReadBuff(TTransport* trans)
            :   _trans(trans), _read_ptr(NULL)
        {
        }

        void clear();

        void borrow(uint32_t length);

        void consume();

        const uint8_t* readAll(uint8_t* buf, uint32_t length);

        TTransport* _trans;

        const uint8_t*          _read_ptr;
        std::stack<uint32_t>    _dom_length;
    };
    ReadBuff                m_read_buff;
};

/**
 * Constructs input and output protocol objects given transports.
 */
class TBSONProtocolFactory : public TProtocolFactory
{
public:
    TBSONProtocolFactory() {}

    virtual ~TBSONProtocolFactory() {}

    cxx::shared_ptr<TProtocol> getProtocol(cxx::shared_ptr<TTransport> trans)
    {
        return cxx::shared_ptr<TProtocol>(new TBSONProtocol(trans));
    }
};

}}} // pebble::dr::protocol

#endif // PEBBLE_DR_PROTOCOL_BSONPROTOCOL_H
