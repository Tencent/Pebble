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


#ifndef T_ATTRIBUTES_H
#define T_ATTRIBUTES_H

#include <map>
#include <string>

class t_program;

class t_attributes : public t_type {
 public:
  t_attributes(t_program* program) :
    t_type(program) {}

  virtual std::string get_fingerprint_material() const {
    throw "BUG: Can't get fingerprint material for attributes.";
  }

  bool set_attribute(std::string name, std::map<std::string, std::string> attribute_pairs) {
    if (attributes_.find(name) != attributes_.end()) {
        return false;
    }
    attributes_[name] = attribute_pairs;
    return true;
  }

  std::map<std::string, std::map<std::string, std::string> > attributes_;
};

#endif
