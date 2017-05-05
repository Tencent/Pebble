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

#ifndef PEBBLE_DR_PROTOCOL_BINARYPROTOCOL_H
#define PEBBLE_DR_PROTOCOL_BINARYPROTOCOL_H

#include "framework/dr/protocol/protocol.h"
#include "framework/dr/protocol/virtual_protocol.h"
#include <stdlib.h>

namespace pebble { namespace dr { namespace protocol {

/**
 * The default binary protocol for thrift. Writes all data in a very basic
 * binary format, essentially just spitting out the raw bytes.
 *
 */
template <class Transport_>
class TBinaryProtocolT
  : public TVirtualProtocol< TBinaryProtocolT<Transport_> > {
 protected:
  static const int32_t VERSION_MASK   = ((int32_t)0xffff0000);
  static const int32_t VERSION_1      = ((int32_t)0x80010000);
  static const int MAX_STRING_SIZE    = (8 * 1024 * 1024); // 8M
  static const int MAX_CONTAINER_SIZE = (8 * 1024 * 1024); // 8M

 public:
  TBinaryProtocolT(cxx::shared_ptr<Transport_> trans) :
    TVirtualProtocol< TBinaryProtocolT<Transport_> >(trans),
    trans_(trans.get()),
    string_limit_(MAX_STRING_SIZE),
    container_limit_(MAX_CONTAINER_SIZE),
    strict_read_(false),
    strict_write_(false),
    string_buf_(NULL),
    string_buf_size_(0) {}

  TBinaryProtocolT(cxx::shared_ptr<Transport_> trans,
                   int32_t string_limit,
                   int32_t container_limit,
                   bool strict_read,
                   bool strict_write) :
    TVirtualProtocol< TBinaryProtocolT<Transport_> >(trans),
    trans_(trans.get()),
    string_limit_(string_limit),
    container_limit_(container_limit),
    strict_read_(strict_read),
    strict_write_(strict_write),
    string_buf_(NULL),
    string_buf_size_(0) {}

  ~TBinaryProtocolT() {
    if (string_buf_ != NULL) {
      free(string_buf_);
      string_buf_size_ = 0;
    }
  }

  void setStringSizeLimit(int32_t string_limit) {
    string_limit_ = string_limit;
  }

  void setContainerSizeLimit(int32_t container_limit) {
    container_limit_ = container_limit;
  }

  void setStrict(bool strict_read, bool strict_write) {
    strict_read_ = strict_read;
    strict_write_ = strict_write;
  }

  /**
   * Writing functions.
   */

  /*ol*/ uint32_t writeMessageBegin(const std::string& name,
                                    const TMessageType messageType,
                                    const int64_t seqid);

  /*ol*/ uint32_t writeMessageEnd();


  inline uint32_t writeStructBegin(const char* name);

  inline uint32_t writeStructEnd();

  inline uint32_t writeFieldBegin(const char* name,
                                  const TType fieldType,
                                  const int16_t fieldId);

  inline uint32_t writeFieldEnd();

  inline uint32_t writeFieldStop();

  inline uint32_t writeMapBegin(const TType keyType,
                                const TType valType,
                                const uint32_t size);

  inline uint32_t writeMapEnd();

  inline uint32_t writeListBegin(const TType elemType, const uint32_t size);

  inline uint32_t writeListEnd();

  inline uint32_t writeSetBegin(const TType elemType, const uint32_t size);

  inline uint32_t writeSetEnd();

  inline uint32_t writeBool(const bool value);

  inline uint32_t writeByte(const int8_t byte);

  inline uint32_t writeI16(const int16_t i16);

  inline uint32_t writeI32(const int32_t i32);

  inline uint32_t writeI64(const int64_t i64);

  inline uint32_t writeDouble(const double dub);

  template <typename StrType>
  inline uint32_t writeString(const StrType& str);

  inline uint32_t writeBinary(const std::string& str);

  /**
   * Reading functions
   */


  /*ol*/ uint32_t readMessageBegin(std::string& name,
                                   TMessageType& messageType,
                                   int64_t& seqid);

  /*ol*/ uint32_t readMessageEnd();

  inline uint32_t readStructBegin(std::string& name);

  inline uint32_t readStructEnd();

  inline uint32_t readFieldBegin(std::string& name,
                                 TType& fieldType,
                                 int16_t& fieldId);

  inline uint32_t readFieldEnd();

  inline uint32_t readMapBegin(TType& keyType,
                               TType& valType,
                               uint32_t& size);

  inline uint32_t readMapEnd();

  inline uint32_t readListBegin(TType& elemType, uint32_t& size);

  inline uint32_t readListEnd();

  inline uint32_t readSetBegin(TType& elemType, uint32_t& size);

  inline uint32_t readSetEnd();

  inline uint32_t readBool(bool& value);
  // Provide the default readBool() implementation for std::vector<bool>
  using TVirtualProtocol< TBinaryProtocolT<Transport_> >::readBool;

  inline uint32_t readByte(int8_t& byte);

  inline uint32_t readI16(int16_t& i16);

  inline uint32_t readI32(int32_t& i32);

  inline uint32_t readI64(int64_t& i64);

  inline uint32_t readDouble(double& dub);

  template<typename StrType>
  inline uint32_t readString(StrType& str);

  inline uint32_t readBinary(std::string& str);

 protected:
  template<typename StrType>
  uint32_t readStringBody(StrType& str, int32_t sz);

  Transport_* trans_;

  int32_t string_limit_;
  int32_t container_limit_;

  // Enforce presence of version identifier
  bool strict_read_;
  bool strict_write_;

  // Buffer for reading strings, save for the lifetime of the protocol to
  // avoid memory churn allocating memory on every string read
  uint8_t* string_buf_;
  int32_t string_buf_size_;

};

typedef TBinaryProtocolT<TTransport> TBinaryProtocol;

/**
 * Constructs binary protocol handlers
 */
template <class Transport_>
class TBinaryProtocolFactoryT : public TProtocolFactory {
 public:
  TBinaryProtocolFactoryT() :
    string_limit_(0),
    container_limit_(0),
    strict_read_(false),
    strict_write_(true) {}

  TBinaryProtocolFactoryT(int32_t string_limit, int32_t container_limit,
                          bool strict_read, bool strict_write) :
    string_limit_(string_limit),
    container_limit_(container_limit),
    strict_read_(strict_read),
    strict_write_(strict_write) {}

  virtual ~TBinaryProtocolFactoryT() {}

  void setStringSizeLimit(int32_t string_limit) {
    string_limit_ = string_limit;
  }

  void setContainerSizeLimit(int32_t container_limit) {
    container_limit_ = container_limit;
  }

  void setStrict(bool strict_read, bool strict_write) {
    strict_read_ = strict_read;
    strict_write_ = strict_write;
  }

  cxx::shared_ptr<TProtocol> getProtocol(cxx::shared_ptr<TTransport> trans) {
    cxx::shared_ptr<Transport_> specific_trans =
      cxx::dynamic_pointer_cast<Transport_>(trans);
    TProtocol* prot;
    if (specific_trans) {
      prot = new TBinaryProtocolT<Transport_>(specific_trans, string_limit_,
                                              container_limit_, strict_read_,
                                              strict_write_);
    } else {
      prot = new TBinaryProtocol(trans, string_limit_, container_limit_,
                                 strict_read_, strict_write_);
    }

    return cxx::shared_ptr<TProtocol>(prot);
  }

 private:
  int32_t string_limit_;
  int32_t container_limit_;
  bool strict_read_;
  bool strict_write_;

};

typedef TBinaryProtocolFactoryT<TTransport> TBinaryProtocolFactory;

}}} // pebble::dr::protocol

#include "framework/dr/protocol/binary_protocol.tcc"

#endif // PEBBLE_DR_PROTOCOL_BINARYPROTOCOL_H

