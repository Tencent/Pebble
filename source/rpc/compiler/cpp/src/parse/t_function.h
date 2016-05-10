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

#ifndef T_FUNCTION_H
#define T_FUNCTION_H

#include <sstream>
#include <string>
#include "t_type.h"
#include "t_struct.h"
#include "t_doc.h"

/**
 * Representation of a function. Key parts are return type, function name,
 * optional modifiers, and an argument list, which is implemented as a thrift
 * struct.
 *
 */
class t_function : public t_doc {
 public:
  t_function(t_type* returntype,
             std::string name,
             t_struct* arglist,
             bool oneway=false) :
    returntype_(returntype),
    name_(name),
    arglist_(arglist),
    oneway_(oneway) {
    xceptions_ = new t_struct(NULL);
    if (oneway_ && (! returntype_->is_void())) {
      pwarning(1, "Oneway methods should return void.\n");
    }
  }

  t_function(t_type* returntype,
             std::string name,
             t_struct* arglist,
             t_struct* xceptions,
             bool oneway=false) :
    returntype_(returntype),
    name_(name),
    arglist_(arglist),
    xceptions_(xceptions),
    oneway_(oneway)
  {
    if (oneway_ && !xceptions_->get_members().empty()) {
      throw std::string("Oneway methods can't throw exceptions.");
    }
    if (oneway_ && (! returntype_->is_void())) {
      pwarning(1, "Oneway methods should return void.\n");
    }
  }

  ~t_function() {}

  t_type* get_returntype() const {
    return returntype_;
  }

  const std::string& get_name() const {
    return name_;
  }

  t_struct* get_arglist() const {
    return arglist_;
  }

  t_struct* get_xceptions() const {
    return xceptions_;
  }

  bool is_oneway() const {
    return oneway_;
  }

  std::string get_timeout_ms() {
    // timeout时间单位为s
    std::map<std::string, std::string>::iterator it = annotations_.find("timeout");
    if (annotations_.end() == it) {
      return "-1";
    }
    if ((it->second).empty()) {
      return "-1";
    }
    for (int i = 0; i < static_cast<int>((it->second).length()); i++) {
      if ((it->second)[i] < '0' || (it->second)[i] > '9') {
        return "-1";
      }
    }
    if ((it->second)[0] == '0' && (it->second).length() > 1) {
      return "-1";
    }
    std::ostringstream oss;
    oss << "(" << it->second << " * 1000)";
    return oss.str();
  }

  std::map<std::string, std::string> annotations_;

 private:
  t_type* returntype_;
  std::string name_;
  t_struct* arglist_;
  t_struct* xceptions_;
  bool oneway_;
};

#endif
