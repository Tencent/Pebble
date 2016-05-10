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

#ifndef PEBBLE_RPC_COMMON_H
#define PEBBLE_RPC_COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <sys/types.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <string>
#include <map>
#include <list>
#include <set>
#include <vector>
#include <exception>
#include <typeinfo>
#include "source/common/pebble_common.h"
#include "source/rpc/common/rpc_define.h"
#include "source/rpc/common/fp_util.h"


struct D {
    void operator()(void *p) {
        // deleter
        // rpc server管理processor，processor需要用户传入创建的服务对象，这个服务对象由用户创建
        // 无需框架删除，避免出现重复free
        p = p;
    }
};


#define T_VIRTUAL_CALL()

#define T_GLOBAL_DEBUG_VIRTUAL 0

namespace pebble { namespace rpc {

typedef enum {
    ROUTE_RANDOM  = 0x30,
    ROUTE_MOD     = 0x31,
    // add other router...
    ROUTE_BUTT
} ROUTER_TYPE;


class TEnumIterator : public std::iterator<std::forward_iterator_tag,
    std::pair<int, const char*> > {
public:
    TEnumIterator(int n,
        int* enums,
        const char** names) :
        ii_(0), n_(n), enums_(enums), names_(names) {
    }

    int operator ++() {
        return ++ii_;
    }

    bool operator != (const TEnumIterator& end) {
        (void)end;  // avoid "unused" warning with NDEBUG
        assert(end.n_ == -1);
        return (ii_ != n_);
    }

    std::pair<int, const char*> operator*() const {
        return std::make_pair(enums_[ii_], names_[ii_]);
    }

private:
    int ii_;
    const int n_;
    int* enums_;
    const char** names_;
};

class TOutput {
public:
    TOutput() : f_(&errorTimeWrapper) {}

    inline void setOutputFunction(void (*function)(const char *)) {
        f_ = function;
    }

    inline void operator()(const char *message) {
        f_(message);
    }

    // It is important to have a const char* overload here instead of
    // just the string version, otherwise errno could be corrupted
    // if there is some problem allocating memory when constructing
    // the string.
    void perror(const char *message, int errno_copy);
    inline void perror(const std::string &message, int errno_copy) {
        perror(message.c_str(), errno_copy);
    }

    void printf(const char *message, ...);

    static void errorTimeWrapper(const char* msg);

    /** Just like strerror_r but returns a C++ string object. */
    static std::string strerror_s(int errno_copy);

private:
    void (*f_)(const char *);
};

extern TOutput GlobalOutput;

class TException : public std::exception {
public:
    TException():
        message_() {}

    explicit TException(const std::string& message) :
        message_(message) {}

    virtual ~TException() throw() {}

    virtual const char* what() const throw() {
        if (message_.empty()) {
            return "Default TException.";
        } else {
            return message_.c_str();
        }
    }

protected:
    std::string message_;
};


class TDelayedException {
public:
    template <class E> static TDelayedException* delayException(const E& e);
    virtual void throw_it() = 0;
    virtual ~TDelayedException() {}
};

template <class E> class TExceptionWrapper : public TDelayedException {
public:
    explicit TExceptionWrapper(const E& e) : e_(e) {}
    virtual void throw_it() {
        E temp(e_);
        delete this;
        throw temp;
    }
private:
    E e_;
};

template <class E>
TDelayedException* TDelayedException::delayException(const E& e) {
    return new TExceptionWrapper<E>(e);
}

namespace protocol {
    class TProtocol;
}

class TApplicationException : public TException {
public:
    /**
     * Error codes for the various types of exceptions.
     */
    enum TApplicationExceptionType {
        UNKNOWN = 0,
        UNKNOWN_METHOD = 1,
        INVALID_MESSAGE_TYPE = 2,
        WRONG_METHOD_NAME = 3,
        BAD_SEQUENCE_ID = 4,
        MISSING_RESULT = 5,
        INTERNAL_ERROR = 6,
        PROTOCOL_ERROR = 7,
        INVALID_TRANSFORM = 8,
        INVALID_PROTOCOL = 9,
        UNSUPPORTED_CLIENT_TYPE = 10,
        REQUEST_TIME_OUT = 11,
        UNKNOWN_SERVICE = 12,
        TRANSPORT_EXCEPTION = 13,
        PROTOCOL_EXCEPTION = 14,
        ACCESS_DENIED = 15,
    };

    TApplicationException() :
        TException(),
        type_(UNKNOWN) {}

    explicit TApplicationException(TApplicationExceptionType type) :
        TException(),
        type_(type) {}

    explicit TApplicationException(const std::string& message) :
        TException(message),
        type_(UNKNOWN) {}

    TApplicationException(TApplicationExceptionType type,
                        const std::string& message) :
        TException(message),
        type_(type) {}

    virtual ~TApplicationException() throw() {}

    /**
     * Returns an error code that provides information about the type of error
     * that has occurred.
     *
     * @return Error code
     */
    TApplicationExceptionType getType() {
        return type_;
    }

    virtual const char* what() const throw() {
        if (message_.empty()) {
            switch (type_) {
            case UNKNOWN                 : return
                "TApplicationException: Unknown application exception";
            case UNKNOWN_METHOD          : return "TApplicationException: Unknown method";
            case INVALID_MESSAGE_TYPE    : return "TApplicationException: Invalid message type";
            case WRONG_METHOD_NAME       : return "TApplicationException: Wrong method name";
            case BAD_SEQUENCE_ID         : return "TApplicationException: Bad sequence identifier";
            case MISSING_RESULT          : return "TApplicationException: Missing result";
            case INTERNAL_ERROR          : return "TApplicationException: Internal error";
            case PROTOCOL_ERROR          : return "TApplicationException: Protocol error";
            case INVALID_TRANSFORM       : return "TApplicationException: Invalid transform";
            case INVALID_PROTOCOL        : return "TApplicationException: Invalid protocol";
            case UNSUPPORTED_CLIENT_TYPE : return "TApplicationException: Unsupported client type";
            case REQUEST_TIME_OUT        : return "TApplicationException: Request time out";
            case UNKNOWN_SERVICE         : return "TApplicationException: Unknown service";
            case TRANSPORT_EXCEPTION     : return "TApplicationException: transport exception";
            case PROTOCOL_EXCEPTION      : return "TApplicationException: protocol exception";
            case ACCESS_DENIED           : return "TApplicationException: access denied";
            default                      : return "TApplicationException: (Invalid exception type)";
            };
        } else {
            return message_.c_str();
        }
    }

    uint32_t read(protocol::TProtocol* iprot);
    uint32_t write(protocol::TProtocol* oprot) const;

protected:
    /**
     * Error code
     */
    TApplicationExceptionType type_;

};

} // namespace rpc
} // namespace pebble

#endif // PEBBLE_RPC_COMMON_H
