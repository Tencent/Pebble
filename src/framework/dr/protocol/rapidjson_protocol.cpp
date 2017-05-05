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

#define RAPIDJSON_HAS_STDSTRING 1

#include "framework/dr/protocol/rapidjson_protocol.h"
#include "framework/dr/transport/transport_exception.h"
#include <sstream>
#include <limits>
#include "framework/dr/protocol/base64_utils.h"

#include <iostream>


using namespace rapidjson;
using namespace pebble::dr::transport;

namespace pebble { namespace dr { namespace protocol {

static const uint32_t kThriftVersion1 = 1;

// static const std::string kTypeNameBool("tf");
// static const std::string kTypeNameByte("i8");
// static const std::string kTypeNameI16("i16");
// static const std::string kTypeNameI32("i32");
// static const std::string kTypeNameI64("i64");
// static const std::string kTypeNameDouble("dbl");
// static const std::string kTypeNameStruct("rec");
// static const std::string kTypeNameString("str");
// static const std::string kTypeNameMap("map");
// static const std::string kTypeNameList("lst");
// static const std::string kTypeNameSet("set");
// 
// static const std::string &getTypeNameForTypeID(TType typeID) {
//   switch (typeID) {
//   case T_BOOL:
//     return kTypeNameBool;
//   case T_BYTE:
//     return kTypeNameByte;
//   case T_I16:
//     return kTypeNameI16;
//   case T_I32:
//     return kTypeNameI32;
//   case T_I64:
//     return kTypeNameI64;
//   case T_DOUBLE:
//     return kTypeNameDouble;
//   case T_STRING:
//     return kTypeNameString;
//   case T_STRUCT:
//     return kTypeNameStruct;
//   case T_MAP:
//     return kTypeNameMap;
//   case T_SET:
//     return kTypeNameSet;
//   case T_LIST:
//     return kTypeNameList;
//   default:
//     throw TProtocolException(TProtocolException::NOT_IMPLEMENTED,
//                              "Unrecognized type");
//   }
// }
// 
// static TType getTypeIDForTypeName(const char *name, size_t name_len) {
//   TType result = T_STOP; // Sentinel value
//   if (name_len > 1) {
//     switch (name[0]) {
//     case 'd':
//       result = T_DOUBLE;
//       break;
//     case 'i':
//       switch (name[1]) {
//       case '8':
//         result = T_BYTE;
//         break;
//       case '1':
//         result = T_I16;
//         break;
//       case '3':
//         result = T_I32;
//         break;
//       case '6':
//         result = T_I64;
//         break;
//       }
//       break;
//     case 'l':
//       result = T_LIST;
//       break;
//     case 'm':
//       result = T_MAP;
//       break;
//     case 'r':
//       result = T_STRUCT;
//       break;
//     case 's':
//       if (name[1] == 't') {
//         result = T_STRING;
//       }
//       else if (name[1] == 'e') {
//         result = T_SET;
//       }
//       break;
//     case 't':
//       result = T_BOOL;
//       break;
//     }
//   }
//   if (result == T_STOP) {
//     throw TProtocolException(TProtocolException::NOT_IMPLEMENTED,
//                              "Unrecognized type");
//   }
//   return result;
// }

class TraverseContext {
  public:
    TraverseContext() {}

    virtual ~TraverseContext() {}

    virtual const Value &next() = 0;

    virtual bool has_more() = 0;
};


class ObjectTraverseContext : public TraverseContext {
  public:
    ObjectTraverseContext(Value::ConstMemberIterator itr, Value::ConstMemberIterator end) : itr_(itr), end_(end), value_(false) {}
    ObjectTraverseContext(const Value &v) : itr_(v.MemberBegin()), end_(v.MemberEnd()), value_(false) {}

    const Value &next() {
      const Value & ret = value_ ? itr_++->value : itr_->name;
      value_ = !value_;
      return ret;
    }

    bool has_more() {
      return itr_ != end_;
    }

  private:
    Value::ConstMemberIterator itr_;

    Value::ConstMemberIterator end_;

    bool value_;
};


class ArrayTraverseContext : public TraverseContext {
  public:
    ArrayTraverseContext(Value::ConstValueIterator itr, Value::ConstValueIterator end) : itr_(itr), end_(end) {}
    ArrayTraverseContext(const Value &v) : itr_(v.Begin()), end_(v.End()) {}

    const Value &next() {
      return *(itr_++);
    }

    bool has_more() {
      return itr_ != end_;
    }

  private:
    Value::ConstValueIterator itr_;

    Value::ConstValueIterator end_;
};

class ValueTraverseContext : public TraverseContext {
  public:
    ValueTraverseContext(const Value &value) : value_(value), consumed_(false) {}

    const Value &next() {
      consumed_ = true;
      return value_;
    }

    bool has_more() {
      return !consumed_;
    }

  private:
    const Value &value_;

    bool consumed_;
};

TRAPIDJSONProtocol::TRAPIDJSONProtocol(cxx::shared_ptr<TTransport> ptrans) :
  TVirtualProtocol<TRAPIDJSONProtocol>(ptrans),
  trans_(ptrans.get()),
  sb_(),
  writer_(sb_),
  document_(),
  lookahead_(ptrans) {
}

TRAPIDJSONProtocol::~TRAPIDJSONProtocol() {}

uint32_t TRAPIDJSONProtocol::writeMessageBegin(const std::string& name,
                                          const TMessageType messageType,
                                          const int64_t seqid) {
  sb_.Clear();
  writer_.Reset(sb_);

  writer_.StartArray();
  writer_.Int(kThriftVersion1);
  writer_.String(name);
  writer_.Int(messageType);
  writer_.Int64(seqid);

  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeMessageEnd() {
  writer_.EndArray();
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeStructBegin(const char* name) {
  writer_.StartObject();
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeStructEnd() {
  writer_.EndObject();
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeFieldBegin(const char* name,
                                        const TType fieldType,
                                        const int16_t fieldId) {
  writer_.String(name);
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeFieldEnd() {
  return 0;
}

uint32_t TRAPIDJSONProtocol::writeFieldStop() {
  return 0;
}

uint32_t TRAPIDJSONProtocol::writeMapBegin(const TType keyType,
                                      const TType valType,
                                      const uint32_t size) {
  writer_.StartArray();
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeMapEnd() {
  writer_.EndArray();
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeListBegin(const TType elemType,
                                       const uint32_t size) {
  writer_.StartArray();
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeListEnd() {
  writer_.EndArray();
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeSetBegin(const TType elemType,
                                      const uint32_t size) {
  writer_.StartArray();
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeSetEnd() {
  writer_.EndArray();
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeBool(const bool value) {
  writer_.Bool(value);
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeByte(const int8_t byte) {
  // writeByte() must be handled specially becuase boost::lexical cast sees
  // int8_t as a text type instead of an integer type
  writer_.Int(byte);
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeI16(const int16_t i16) {
  writer_.Int(i16);
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeI32(const int32_t i32) {
  writer_.Int(i32);
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeI64(const int64_t i64) {
  writer_.Int64(i64);
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeDouble(const double dub) {
  writer_.Double(dub);
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeString(const std::string& str) {
  writer_.String(str);
  return write_to_transport();
}

uint32_t TRAPIDJSONProtocol::writeBinary(const std::string& str) {
  std::stringstream ss;
  uint8_t b[4];
  const uint8_t *bytes = (const uint8_t *)str.c_str();
  if(str.length() > (std::numeric_limits<uint32_t>::max)())
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  uint32_t len = static_cast<uint32_t>(str.length());
  while (len >= 3) {
    // Encode 3 bytes at a time
    base64_encode(bytes, 3, b);
    ss.write(reinterpret_cast<char *>(b), 4);
    bytes += 3;
    len -=3;
  }
  if (len) { // Handle remainder
    base64_encode(bytes, len, b);
    ss.write(reinterpret_cast<char*>(b), len + 1);
  }
  writer_.String(ss.str());
  return write_to_transport();
}

inline const Value &TRAPIDJSONProtocol::get_value(uint32_t &readed, bool no_parse) {
  if (contexts_stack_.size() > 0) {
    if (contexts_stack_.top()->has_more()) {
      readed = 0;
      return contexts_stack_.top()->next();
    }
  } else if (!no_parse) {
    lookahead_.Reset();
    document_.ParseStream<kParseStopWhenDoneFlag>(lookahead_);
    if (document_.HasParseError()) {
      throw TProtocolException(TProtocolException::INVALID_DATA,
        "invalid json format.");
    }
    readed = lookahead_.Tell();
    return document_;
  }
  throw TProtocolException(TProtocolException::INVALID_DATA,
    "no more value.");
}

uint32_t TRAPIDJSONProtocol::readMessageBegin(std::string& name,
                                         TMessageType& messageType,
                                         int64_t& seqid) {
  while (!contexts_stack_.empty()) {
    contexts_stack_.pop();
  }

  uint32_t readed = 0;
  const Value &root = get_value(readed);

  if (!root.IsArray()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected Array for a message.");
  }

  Value::ConstValueIterator itr = root.Begin();

  if (!(*itr).IsUint() || (*itr).GetUint() != kThriftVersion1) {
    throw TProtocolException(TProtocolException::BAD_VERSION,
      "Message contained bad version.");
  }

  ++itr;

  if (!(*itr).IsString()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected String for a message name.");
  }
  name.assign((*itr).GetString(), (*itr).GetStringLength());

  ++itr;

  if (!(*itr).IsUint()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected Uint for a message type.");
  }
  messageType = (TMessageType)(*itr).GetUint();

  ++itr;

  if (!(*itr).IsInt64()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected Uint64 for a seqid.");
  }

  seqid = (*itr).GetInt64();

  ++itr;

  contexts_stack_.push(cxx::shared_ptr<TraverseContext>(new ArrayTraverseContext(itr, root.End())));

  return readed;
}

uint32_t TRAPIDJSONProtocol::readMessageEnd() {
  contexts_stack_.pop();
  return 0;
}

uint32_t TRAPIDJSONProtocol::readStructBegin(std::string& name) {
  uint32_t readed = 0;
  const Value &obj = get_value(readed);
  if (!obj.IsObject()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected Object for a struct.");

  }
  contexts_stack_.push(cxx::shared_ptr<TraverseContext>(new ObjectTraverseContext(obj)));
  return readed;
}

uint32_t TRAPIDJSONProtocol::readStructEnd() {
  contexts_stack_.pop();
  return 0;
}

uint32_t TRAPIDJSONProtocol::readFieldBegin(std::string& name,
                                       TType& fieldType,
                                       int16_t& fieldId) {
  uint32_t result = 0;
  uint32_t readed = 0;
  if (!contexts_stack_.top()->has_more()) {
    fieldType = pebble::dr::protocol::T_STOP;
  } else {
    const Value &field_name = get_value(readed, true);
    result += readed;
    if (!field_name.IsString()) {
      throw TProtocolException(TProtocolException::INVALID_DATA,
        "Expected String for field_name.");
    }
    name.assign(field_name.GetString(), field_name.GetStringLength());
    fieldType = T_NULL;
    fieldId = -1;
  }
  return readed;
}

uint32_t TRAPIDJSONProtocol::readFieldEnd() {
  return 0;
}

uint32_t TRAPIDJSONProtocol::readMapBegin(TType& keyType,
                                     TType& valType,
                                     uint32_t& size) {
  uint32_t result = 0;

  const Value& map = get_value(result);
  if (!map.IsArray()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected Array for map.");
  }
  keyType = T_NULL;
  valType = T_NULL;
  size = map.Size() / 2;
  contexts_stack_.push(cxx::shared_ptr<TraverseContext>(new ObjectTraverseContext(map)));
  return result;
}

uint32_t TRAPIDJSONProtocol::readMapEnd() {
  contexts_stack_.pop();
  return 0;
}

uint32_t TRAPIDJSONProtocol::readListBegin(TType& elemType,
                                      uint32_t& size) {
  uint32_t result = 0;
  const Value &list = get_value(result);
  if (!list.IsArray()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "invalid list format.");
  }

  elemType = T_NULL;
  size = list.Size();

  contexts_stack_.push(cxx::shared_ptr<TraverseContext>(new ArrayTraverseContext(list.Begin(), list.End())));

  return 0;
}

uint32_t TRAPIDJSONProtocol::readListEnd() {
  contexts_stack_.pop();
  return 0;
}

uint32_t TRAPIDJSONProtocol::readSetBegin(TType& elemType,
                                     uint32_t& size) {
  return readListBegin(elemType, size);
}

uint32_t TRAPIDJSONProtocol::readSetEnd() {
  contexts_stack_.pop();
  return 0;
}

uint32_t TRAPIDJSONProtocol::readBool(bool& value) {
  uint32_t readed = 0;
  const Value &data = get_value(readed);
  if (!data.IsBool()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected Bool.");
  }
  value = data.GetBool();
  return readed;
}

// readByte() must be handled properly becuase boost::lexical cast sees int8_t
// as a text type instead of an integer type
uint32_t TRAPIDJSONProtocol::readByte(int8_t& byte) {
  uint32_t readed = 0;
  const Value &data = get_value(readed);
  if (!data.IsInt()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected Int.");
  }
  byte = (int8_t)data.GetInt();
  return readed;
}

uint32_t TRAPIDJSONProtocol::readI16(int16_t& i16) {
  uint32_t readed = 0;
  const Value &data = get_value(readed);
  if (!data.IsInt()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected Int.");
  }
  i16 = (int16_t)data.GetInt();
  return readed;
}

uint32_t TRAPIDJSONProtocol::readI32(int32_t& i32) {
  uint32_t readed = 0;
  const Value &data = get_value(readed);
  if (!data.IsInt()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected Int.");
  }
  i32 = data.GetInt();
  return readed;
}

uint32_t TRAPIDJSONProtocol::readI64(int64_t& i64) {
  uint32_t readed = 0;
  const Value &data = get_value(readed);
  if (!data.IsInt64()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected Int64.");
  }
  i64 = data.GetInt64();
  return readed;
}

uint32_t TRAPIDJSONProtocol::readDouble(double& dub) {
  uint32_t readed = 0;
  const Value &data = get_value(readed);
  if (!data.IsDouble()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected Double.");
  }
  dub = data.GetDouble();
  return readed;
}

uint32_t TRAPIDJSONProtocol::readString(std::string &str) {
  uint32_t readed = 0;
  const Value &data = get_value(readed);
  if (!data.IsString()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected String.");
  }
  str.assign(data.GetString(), data.GetStringLength());
  return readed;
}

uint32_t TRAPIDJSONProtocol::readBinary(std::string &str) {
  uint32_t result = 0;
  const Value &tmp = get_value(result);
  if (!tmp.IsString()) {
    throw TProtocolException(TProtocolException::INVALID_DATA,
      "Expected String.");
  }
  uint8_t *b = (uint8_t *)tmp.GetString();
  if(tmp.GetStringLength() > (std::numeric_limits<uint32_t>::max)())
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  uint32_t len = static_cast<uint32_t>(tmp.GetStringLength());
  str.clear();
  while (len >= 4) {
    base64_decode(b, 4);
    str.append((const char *)b, 3);
    b += 4;
    len -= 4;
  }
  // Don't decode if we hit the end or got a single leftover byte (invalid
  // base64 but legal for skip of regular string type)
  if (len > 1) {
    base64_decode(b, len);
    str.append((const char *)b, len - 1);
  }
  return result;
}

uint32_t TRAPIDJSONProtocol::skip(TType type) {
  if (type == T_NULL) {
    uint32_t readed = 0;
    get_value(readed);
    return readed;
  } else {
    return pebble::dr::protocol::skip(*this, type);
  }
}

}}} // pebble::dr::protocol

