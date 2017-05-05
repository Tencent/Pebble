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

#ifndef PEBBLE_DR_PROTOCOL_RAPIDJSONPROTOCOL_H
#define PEBBLE_DR_PROTOCOL_RAPIDJSONPROTOCOL_H

#include "framework/dr/protocol/virtual_protocol.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"

#include <stack>

namespace pebble { namespace dr { namespace protocol {

class TraverseContext;

struct LookaheadStream {
    typedef uint8_t Ch;

    LookaheadStream(cxx::shared_ptr<pebble::dr::transport::TTransport> tran) : tran_(tran) {
        Reset();
    }

    void Reset() {
        taked_ = false;
        buff_ = '\0';
        eof_ = false;
        tell_ = 0;
    }

    Ch Peek() {
        if (eof_) {
            return '\0';
        }

        if (!taked_) {
            try {
	        if (tran_->read(&buff_, 1) != 1) {
                    eof_ = true;
                    return '\0';
                }
            } catch (...) {
                eof_ = true;
                return '\0';
            }

            taked_ = true;
        }

        return buff_;
    }

    Ch Take() {
        if (eof_) {
            return '\0';
        }

        Ch buff;
        try {
            if (taked_) {
                taked_ = false;
                buff = buff_;
            } else if (tran_->read(&buff, 1) != 1) {
                eof_ = true;
                return '\0';
            }
        } catch (...) {
            eof_ = true;
            return '\0';
        }
        tell_++;

        return buff;
    }

    size_t Tell() const {
        return tell_;
    }

    void Put(Ch) { RAPIDJSON_ASSERT(false); }
    void Flush() { RAPIDJSON_ASSERT(false); } 
    Ch* PutBegin() { RAPIDJSON_ASSERT(false); return 0; }
    size_t PutEnd(Ch*) { RAPIDJSON_ASSERT(false); return 0; }

    cxx::shared_ptr<pebble::dr::transport::TTransport> tran_;

    bool taked_;

    Ch buff_;

    bool eof_;

    size_t tell_;
};

class TRAPIDJSONProtocol : public TVirtualProtocol<TRAPIDJSONProtocol> {
 public:

  TRAPIDJSONProtocol(cxx::shared_ptr<TTransport> ptrans);

  ~TRAPIDJSONProtocol();

 public:

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
  using TVirtualProtocol<TRAPIDJSONProtocol>::readBool;

  uint32_t readByte(int8_t& byte);

  uint32_t readI16(int16_t& i16);

  uint32_t readI32(int32_t& i32);

  uint32_t readI64(int64_t& i64);

  uint32_t readDouble(double& dub);

  uint32_t readString(std::string& str);

  uint32_t readBinary(std::string& str);

  uint32_t skip(TType type);

 private:
  TTransport* trans_;

  rapidjson::StringBuffer sb_;

  rapidjson::Writer<rapidjson::StringBuffer> writer_;

  rapidjson::Document document_;

  LookaheadStream lookahead_;

  std::stack<cxx::shared_ptr<TraverseContext> > contexts_stack_;

 private:
  inline uint32_t write_to_transport() {
    size_t sb_size = sb_.GetSize();
    trans_->write(reinterpret_cast<const uint8_t*>(sb_.GetString()), sb_size);
    sb_.Clear();
    return sb_size;
  }

  inline const rapidjson::Value &get_value(uint32_t &len, bool no_parse = false);
};

/**
 * Constructs input and output protocol objects given transports.
 */
class TRAPIDJSONProtocolFactory : public TProtocolFactory {
 public:
  TRAPIDJSONProtocolFactory() {}

  virtual ~TRAPIDJSONProtocolFactory() {}

  cxx::shared_ptr<TProtocol> getProtocol(cxx::shared_ptr<TTransport> trans) {
    return cxx::shared_ptr<TProtocol>(new TRAPIDJSONProtocol(trans));
  }
};

} // protocol
} // dr
} // pebble


#endif // PEBBLE_DR_PROTOCOL_RAPIDJSONPROTOCOL_H

