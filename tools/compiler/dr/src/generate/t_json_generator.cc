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
 *
 * Contains some contributions under the Thrift Software License.
 * Please see doc/old-thrift-license.txt in the Thrift distribution for
 * details.
 */

#include <fstream>
#include <iostream>
#include <sstream>

#include <stdlib.h>
#include <sys/stat.h>
#include <sstream>
#include "t_generator.h"
#include "../platform.h"

using std::map;
using std::ofstream;
using std::ostream;
using std::ostringstream;
using std::string;
using std::stringstream;
using std::vector;
using std::stack;

static const string endl = "\n";
static const string quot = "\"";

class t_json_generator : public t_generator {
public:
  t_json_generator(
    t_program* program,
    const std::map<std::string, std::string>& parsed_options,
    const std::string& option_string)
    : t_generator(program)
  {
    // (void) parsed_options;
    (void) option_string;
    out_dir_base_ = "gen-json";

    std::map<std::string, std::string>::const_iterator iter =
        parsed_options.find("message");
    _gen_msg_template = (iter != parsed_options.end());
  }

  virtual ~t_json_generator() {}

  /**
  * Init and close methods
  */

  void init_generator();
  void close_generator();

  void generate_typedef(t_typedef* ttypedef);
  void generate_enum(t_enum* tenum);
  void generate_program();
  void generate_consts(vector<t_const*>);
  void generate_function(t_function * tfunc);
  void generate_field(t_field * field);

  void generate_service(t_service* tservice);
  void generate_struct(t_struct* tstruct);

  // 生成消息模版
  void gen_type_str(t_type* type);
  void gen_type_value(t_type* type, bool gen_quotes = false);
  void gen_field(t_field * field);
  void gen_message();

private:
  std::ofstream f_json_;
  std::stack<bool> _commaNeeded;
  bool _gen_msg_template;
  string get_type_name(t_type* type);
  string get_const_value(t_const_value* val);

  void start_object();
  void start_array();
  void write_key(string key, string val);
  void write_key_int(string key, int val);
  void end_object(bool newLine);
  void end_array(bool newLine);
  void write_comma_if_needed();
  void indicate_comma_needed();
  string escapeJsonString(const string& input);

  void merge_includes(t_program*);
};

void t_json_generator::init_generator() {
  MKDIR(get_out_dir().c_str());

  string suffix(".json");
  if (_gen_msg_template) {
    suffix = ".template";
  }
  string f_json_name = get_out_dir() + program_->get_name() + suffix;
  f_json_.open(f_json_name.c_str());

  //Merge all included programs into this one so we can output one big file.
  merge_includes(program_);
}

string t_json_generator::escapeJsonString(const string& input) {
  std::ostringstream ss;
  for (std::string::const_iterator iter = input.begin(); iter != input.end(); iter++) {
    switch (*iter) {
    case '\\': ss << "\\\\"; break;
    case '"': ss << "\\\""; break;
    case '/': ss << "\\/"; break;
    case '\b': ss << "\\b"; break;
    case '\f': ss << "\\f"; break;
    case '\n': ss << "\\n"; break;
    case '\r': ss << "\\r"; break;
    case '\t': ss << "\\t"; break;
    default: ss << *iter; break;
    }
  }
  return ss.str();
}

void t_json_generator::start_object(){
  f_json_ << "{";
  _commaNeeded.push(false);
}

void t_json_generator::start_array(){
  f_json_ << "[";
  _commaNeeded.push(false);
}

void t_json_generator::write_comma_if_needed(){
  if (_commaNeeded.top()) f_json_ << ",";
}

void t_json_generator::indicate_comma_needed(){
  _commaNeeded.pop();
  _commaNeeded.push(true);
}

void t_json_generator::write_key(string key, string val){
  write_comma_if_needed();
  f_json_ << quot << key << quot << ":" << quot << escapeJsonString(val) << quot;
  indicate_comma_needed();
}

void t_json_generator::write_key_int(string key, int val){
  write_comma_if_needed();
  f_json_ << quot << key << quot << ":" << quot << val << quot;
  indicate_comma_needed();
}

void t_json_generator::end_object(bool newLine){
  f_json_ << "}";
  if (newLine) f_json_ << endl;
  _commaNeeded.pop();
}

void t_json_generator::end_array(bool newLine){
  f_json_ << "]";
  if (newLine) f_json_ << endl;
  _commaNeeded.pop();
}

void t_json_generator::close_generator() {
  f_json_.close();
}

void t_json_generator::merge_includes(t_program * program) {
  vector<t_program*> includes = program->get_includes();
  vector<t_program*>::iterator inc_iter;
  for (inc_iter = includes.begin(); inc_iter != includes.end(); ++inc_iter){
    t_program* include = *inc_iter;
    //recurse in case we get crazy
    merge_includes(include);
    //merge enums
    vector<t_enum*> enums = include->get_enums();
    vector<t_enum*>::iterator en_iter;
    for (en_iter = enums.begin(); en_iter != enums.end(); ++en_iter) {
      program->add_enum(*en_iter);
    }
    //merge typedefs
    vector<t_typedef*> typedefs = include->get_typedefs();
    vector<t_typedef*>::iterator td_iter;
    for (td_iter = typedefs.begin(); td_iter != typedefs.end(); ++td_iter) {
      program->add_typedef(*td_iter);
    }
    //merge structs
    vector<t_struct*> objects = include->get_objects();
    vector<t_struct*>::iterator o_iter;
    for (o_iter = objects.begin(); o_iter != objects.end(); ++o_iter) {
      program->add_struct(*o_iter);
    }
    //merge constants
    vector<t_const*> consts = include->get_consts();
    vector<t_const*>::iterator c_iter;
    for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter){
      program->add_const(*c_iter);
    }

    // Generate services
    vector<t_service*> services = include->get_services();
    vector<t_service*>::iterator sv_iter;
    for (sv_iter = services.begin(); sv_iter != services.end(); ++sv_iter) {
      program->add_service(*sv_iter);
    }
  }
}

void t_json_generator::generate_program() {
  if (_gen_msg_template) {
    gen_message();
    return;
  }

  // Initialize the generator
  init_generator();
  start_object();

  write_key("name", program_->get_name());
  if (program_->has_doc()) write_key("doc", program_->get_doc());

  // Generate enums
  vector<t_enum*> enums = program_->get_enums();
  vector<t_enum*>::iterator en_iter;
  f_json_ << ",\"enums\":";
  start_array();
  f_json_ << endl;
  for (en_iter = enums.begin(); en_iter != enums.end(); ++en_iter) {
    write_comma_if_needed();
    generate_enum(*en_iter);
    indicate_comma_needed();
  }
  end_array(true);

  // Generate typedefs
  vector<t_typedef*> typedefs = program_->get_typedefs();
  vector<t_typedef*>::iterator td_iter;
  f_json_ << ",\"typedefs\":";
  start_array();
  f_json_ << endl;
  for (td_iter = typedefs.begin(); td_iter != typedefs.end(); ++td_iter) {
    write_comma_if_needed();
    generate_typedef(*td_iter);
    indicate_comma_needed();
  }
  end_array(true);

  // Generate structs, exceptions, and unions in declared order
  vector<t_struct*> objects = program_->get_objects();
  vector<t_struct*>::iterator o_iter;
  f_json_ << ",\"structs\":";
  start_array();
  f_json_ << endl;
  for (o_iter = objects.begin(); o_iter != objects.end(); ++o_iter) {
    write_comma_if_needed();
    if ((*o_iter)->is_xception()) {
      generate_xception(*o_iter);
    }
    else {
      generate_struct(*o_iter);
    }
    indicate_comma_needed();
  }
  end_array(true);

  // Generate constants
  vector<t_const*> consts = program_->get_consts();
  generate_consts(consts);

  // Generate services
  vector<t_service*> services = program_->get_services();
  vector<t_service*>::iterator sv_iter;
  f_json_ << ",\"services\":";
  start_array();
  f_json_ << endl;
  for (sv_iter = services.begin(); sv_iter != services.end(); ++sv_iter) {
    write_comma_if_needed();
    service_name_ = get_service_name(*sv_iter);
    generate_service(*sv_iter);
    indicate_comma_needed();
  }
  end_array(false);
  end_object(true);
  // Close the generator
  close_generator();
}

void t_json_generator::generate_typedef(t_typedef* ttypedef){
  start_object();
  write_key("name", ttypedef->get_name());
  write_key("type", get_type_name(ttypedef->get_true_type()));
  if (ttypedef->has_doc()) write_key("doc", ttypedef->get_doc());
  end_object(true);
}

void t_json_generator::generate_consts(vector<t_const*> consts){
  vector<t_const*>::iterator c_iter;
  f_json_ << ",\"constants\":";
  start_array();
  f_json_ << endl;
  for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter) {
    write_comma_if_needed();
    indicate_comma_needed();
    start_object();
    t_const* con = (*c_iter);
    write_key("name", con->get_name());
    write_key("type", get_type_name(con->get_type()));
    if (con->has_doc()) write_key("doc", con->get_doc());
    write_key("value", get_const_value(con->get_value()));
    end_object(true);
  }
  end_array(true);
}

void t_json_generator::generate_enum(t_enum* tenum) {
  start_object();
  write_key("name", tenum->get_name());
  if (tenum->has_doc()) write_key("doc", tenum->get_doc());
  f_json_ << ",\"members\":";
  start_array();
  vector<t_enum_value*> values = tenum->get_constants();
  vector<t_enum_value*>::iterator val_iter;
  for (val_iter = values.begin(); val_iter != values.end(); ++val_iter) {
    t_enum_value* val = (*val_iter);
    write_comma_if_needed();
    start_object();
    write_key("name", val->get_name());
    write_key_int("value", val->get_value());
    if (val->has_doc()) write_key("doc", val->get_doc());
    end_object(false);
    indicate_comma_needed();
  }
  end_array(false);
  end_object(true);
}

void t_json_generator::generate_struct(t_struct* tstruct){
  start_object();
  write_key("name", tstruct->get_name());
  if (tstruct->has_doc()) write_key("doc", tstruct->get_doc());
  if (tstruct->is_xception()) write_key("isException", "true");
  vector<t_field*> members = tstruct->get_members();
  vector<t_field*>::iterator mem_iter = members.begin();
  f_json_ << ",\"fields\":";
  start_array();
  for (; mem_iter != members.end(); mem_iter++) {
    generate_field((*mem_iter));
  }
  end_array(false);

  if (!tstruct->attributes_.empty()){
    f_json_ << ",\"attributes\":";
    start_object();
    typedef map<string, map<string, string> >::iterator AttributesIterator;
    AttributesIterator attr_iter;
    map<string, string>::iterator kv_iter;
    for (attr_iter = tstruct->attributes_.begin(); attr_iter != tstruct->attributes_.end(); ++attr_iter) {
      if (attr_iter->second.empty()) {
        continue;
      }
      write_comma_if_needed();
      f_json_ << "\"" << attr_iter->first << "\":";
      start_object();
      for (kv_iter = attr_iter->second.begin(); kv_iter != attr_iter->second.end(); ++kv_iter) {
          write_key(kv_iter->first, kv_iter->second);
      }
      end_object(false);
      indicate_comma_needed();
    }
    end_object(false);
  }

  end_object(true);
}

void t_json_generator::generate_service(t_service* tservice){
  start_object();
  write_key("name", tservice->get_name());
  if (tservice->get_extends()) write_key("extendsType", tservice->get_extends()->get_name());
  if (tservice->has_doc()) write_key("doc", tservice->get_doc());
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator fn_iter = functions.begin();
  f_json_ << ",\"functions\":";
  start_array();
  for (; fn_iter != functions.end(); fn_iter++) {
    t_function* func = (*fn_iter);
    write_comma_if_needed();
    indicate_comma_needed();
    generate_function(func);
  }
  end_array(false);
  end_object(true);
}

void t_json_generator::generate_function(t_function* tfunc){
  start_object();
  write_key("name", tfunc->get_name());
  write_key("returnType", get_type_name(tfunc->get_returntype()));
  if (tfunc->is_oneway()) write_key("oneWay", "true");
  if (tfunc->has_doc()) write_key("doc", tfunc->get_doc());
  vector<t_field*> members = tfunc->get_arglist()->get_members();
  vector<t_field*>::iterator mem_iter = members.begin();
  f_json_ << ",\"arguments\":";
  start_array();
  for (; mem_iter != members.end(); mem_iter++) {
    generate_field((*mem_iter));
  }
  end_array(false);

  vector<t_field*> excepts = tfunc->get_xceptions()->get_members();
  vector<t_field*>::iterator ex_iter = excepts.begin();
  f_json_ << ",\"exceptions\":";
  start_array();
  for (; ex_iter != excepts.end(); ex_iter++) {
    generate_field((*ex_iter));
  }
  end_array(false);
  end_object(false);
}

void t_json_generator::generate_field(t_field * field){
  write_comma_if_needed();
  start_object();
  write_key_int("index", field->get_key());
  write_key("name", field->get_name());
  write_key("type", get_type_name(field->get_type()));
  if (field->has_doc()) write_key("doc", field->get_doc());
  switch (field->get_req()) {
        case t_field::T_REQUIRED:
            write_key("required", "true");
            break;
        default:
            write_key("required", "false");
            break;
  }
  if (field->get_value())
    write_key("default", get_const_value(field->get_value()));

  if (!field->attributes_.empty()){
    f_json_ << ",\"attributes\":";
    start_object();
    typedef map<string, map<string, string> >::iterator AttributesIterator;
    AttributesIterator attr_iter;
    map<string, string>::iterator kv_iter;
    for (attr_iter = field->attributes_.begin(); attr_iter != field->attributes_.end(); ++attr_iter) {
      if (attr_iter->second.empty()) {
        continue;
      }
      write_comma_if_needed();
      f_json_ << "\"" << attr_iter->first << "\":";
      start_object();
      for (kv_iter = attr_iter->second.begin(); kv_iter != attr_iter->second.end(); ++kv_iter) {
          write_key(kv_iter->first, kv_iter->second);
      }
      end_object(false);
      indicate_comma_needed();
    }
    end_object(false);
  }

  end_object(false);
  indicate_comma_needed();
}
string t_json_generator::get_const_value(t_const_value* tvalue){

  switch (tvalue->get_type()) {
  case t_const_value::CV_INTEGER:
    return tvalue->get_string();
  case t_const_value::CV_DOUBLE:
    return tvalue->get_string();
  case t_const_value::CV_STRING:
    return tvalue->get_string();
  case t_const_value::CV_LIST:
  {
    string list = "[";
    vector<t_const_value*> list_elems = tvalue->get_list();;
    vector<t_const_value*>::iterator list_iter;
    bool first = true;
    for (list_iter = list_elems.begin(); list_iter != list_elems.end(); list_iter++) {
      if (!first)list += ",";
      first = false;
      list += get_const_value(*list_iter);
    }
    return list + "]";
  }
  case t_const_value::CV_IDENTIFIER:
    return tvalue->get_identifier_name();
  case t_const_value::CV_MAP:
    map<t_const_value*, t_const_value*> map_elems = tvalue->get_map();
    map<t_const_value*, t_const_value*>::iterator map_iter;
    string map = "[";
    bool first = true;
    for (map_iter = map_elems.begin(); map_iter != map_elems.end(); map_iter++) {
      if (!first) map += ",";
      first = false;
      map += get_const_value(map_iter->first) + ":";
      map += get_const_value(map_iter->second);
    }
    return map + "]";
  }
  return "UNKNOWN";
}
string t_json_generator::get_type_name(t_type* ttype){
  if (ttype->is_container()) {
    if (ttype->is_list()) {
      return "list<" + get_type_name(((t_list*)ttype)->get_elem_type()) + ">";
    }
    else if (ttype->is_set()) {
      return "set<" + get_type_name(((t_set*)ttype)->get_elem_type()) + ">";
    }
    else if (ttype->is_map()) {
      return "map<" + get_type_name(((t_map*)ttype)->get_key_type()) +
        +"," + get_type_name(((t_map*)ttype)->get_val_type()) + ">";
    }
  }
  else if (ttype->is_base_type()) {
    return (((t_base_type*)ttype)->is_binary() ? "binary" : ttype->get_name());
  }
  return ttype->get_name();
}

//====================================================================================
// 生成请求消息模版 pebble --gen json:message filename.pebble
//====================================================================================
/*
类型覆盖示例:

struct Info {
    1: i64 id,
    2: string data,
}

struct TypeInfo {
    1: bool para1,
    2: byte para2,
    3: i16 para3,
    4: i32 para4,
    5: i64 para5,
    6: double para6,
    7: string para7,
    8: binary para8,
    9: map<i64,string> para9,
    10: list<i32> para10,
    11: set<string> para11,
    12: map<string, Info> para12,
    13: list<Info> para13,
    14: set<Info> para14,
    15: byte para15,
    16: MyType para16,
}

service MyService {
  Info type_test(1: string p1, 2: TypeInfo p2, 3: Info p3),
}

{
    "p1":{"str":"value_p1"},
    "p2":{
        "rec":{
            "para1":{"tf":1},
            "para2":{"i8":2},
            "para3":{"i16":3},
            "para4":{"i32":4},
            "para5":{"i64":5},
            "para6":{"dbl":6.6},
            "para7":{"str":"para7"},
            "para8":{"str":"/v7+/v7+/v4"},
            "para9":{"map":["i64","str",2,{"0":"value-0","1":"value-1"}]},
            "para10":{"lst":["i32",2,1,2]},
            "para11":{"set":["str",2,"11","12"]},
            "para12":{"map":["str","rec",2,{"0":{"id":{"i64":12},"data":{"str":"1200"}},"1":{"id":{"i64":122},"data":{"str":"12200"}}}]},
            "para13":{"lst":["rec",2,{"id":{"i64":13},"data":{"str":"1300"}},{"id":{"i64":133},"data":{"str":"13300"}}]},
            "para14":{"set":["rec",2,{"id":{"i64":144},"data":{"str":"14400"}},{"id":{"i64":14},"data":{"str":"1400"}}]},
            "para15":{"i8":15},
            "para16":{"i32":16}
        }
    },
    "p3":{
        "rec":{
            "id":{"i64":33},
            "data":{"str":"3333"}
        }
    }
}

*/

// 生成类型的字符串
void t_json_generator::gen_type_str(t_type* type)
{
    if (type->is_base_type()) {
        switch ((static_cast<t_base_type*>(type))->get_base()) {
        case t_base_type::TYPE_STRING:
            f_json_ << "\"str\"";
            break;
        case t_base_type::TYPE_BOOL:
            f_json_ << "\"tf\"";
            break;
        case t_base_type::TYPE_BYTE:
            f_json_ << "\"i8\"";
            break;
        case t_base_type::TYPE_I16:
            f_json_ << "\"i16\"";
            break;
        case t_base_type::TYPE_I32:
            f_json_ << "\"i32\"";
            break;
        case t_base_type::TYPE_I64:
            f_json_ << "\"i64\"";
            break;
        case t_base_type::TYPE_DOUBLE:
            f_json_ << "\"dbl\"";
            break;
        default:
            break;
        }
    } else if (type->is_list()) {
        f_json_ << "\"lst\"";
    } else if (type->is_set()) {
        f_json_ << "\"set\"";
    } else if (type->is_map()) {
        f_json_ << "\"map\"";
    } else if (type->is_struct()) {
        f_json_ << "\"rec\"";
    }
}

// 生成类型的模版值
void t_json_generator::gen_type_value(t_type* type, bool gen_quotes)
{
    if (type->is_base_type()) {
        switch ((static_cast<t_base_type*>(type))->get_base()) {
        case t_base_type::TYPE_STRING:
            f_json_ << "\"string_value\"";
            break;
        case t_base_type::TYPE_BOOL:
            if (gen_quotes)
                f_json_ << "\"0\"";
            else
                f_json_ << "0";
            break;
        case t_base_type::TYPE_BYTE:
            if (gen_quotes)
                f_json_ << "\"8\"";
            else
                f_json_ << "8";
            break;
        case t_base_type::TYPE_I16:
            if (gen_quotes)
                f_json_ << "\"16\"";
            else
                f_json_ << "16";
            break;
        case t_base_type::TYPE_I32:
            if (gen_quotes)
                f_json_ << "\"32\"";
            else
                f_json_ << "32";
            break;
        case t_base_type::TYPE_I64:
            if (gen_quotes)
                f_json_ << "\"64\"";
            else
                f_json_ << "64";
            break;
        case t_base_type::TYPE_DOUBLE:
            if (gen_quotes)
                f_json_ << "\"1.2\"";
            else
                f_json_ << "1.2";
            break;
        default:
            break;
        }
    } else if (type->is_list()) {
        t_list* list_type = static_cast<t_list*>(type);
        f_json_ << "["; // list start
        gen_type_str(list_type->get_elem_type());
        f_json_ << ",";
        f_json_ << "2,"; // elements
        gen_type_value(list_type->get_elem_type());
        f_json_ << ",";
        gen_type_value(list_type->get_elem_type());
        f_json_ << "]"; // list end
    } else if (type->is_set()) {
        t_set* set_type = static_cast<t_set*>(type);
        f_json_ << "["; // set start
        gen_type_str(set_type->get_elem_type());
        f_json_ << ",";
        f_json_ << "2,"; // elements
        gen_type_value(set_type->get_elem_type());
        f_json_ << ",";
        gen_type_value(set_type->get_elem_type());
        f_json_ << "]"; // set end
    } else if (type->is_map()) {
        t_map* map_type = static_cast<t_map*>(type);
        f_json_ << "["; // set start
        gen_type_str(map_type->get_key_type());
        f_json_ << ",";
        gen_type_str(map_type->get_val_type());
        f_json_ << ",";
        f_json_ << "2,"; // elements
        f_json_ << "{";
        gen_type_value(map_type->get_key_type(), true);
        f_json_ << ":";
        gen_type_value(map_type->get_val_type(), true);
        f_json_ << ",";
        gen_type_value(map_type->get_key_type(), true);
        f_json_ << ":";
        gen_type_value(map_type->get_val_type(), true);
        f_json_ << "}";
        f_json_ << "]"; // set stop
    } else if (type->is_struct()) {
        t_struct* rec_type = static_cast<t_struct*>(type);
        f_json_ << "{";
        std::vector<t_field*> fields = rec_type->get_members();
        for (std::vector<t_field*>::iterator it = fields.begin(); it != fields.end(); ++it) {
            if (it != fields.begin()) {
                f_json_ << ",";
            }
            gen_field(*it);
        }
        f_json_ << "}";
    }
}

// 生成字段，字段名、类型、值
void t_json_generator::gen_field(t_field * field) {
    f_json_ << "\"" << field->get_name() << "\"" // field name
        << ":"  // pair separator
        << "{"; // filed start
    gen_type_str(field->get_type());
    f_json_ << ":";
    gen_type_value(field->get_type());
    f_json_ << "}"; // filed end
}

void t_json_generator::gen_message()
{
    init_generator();

    vector<t_service*> services = program_->get_services();
    vector<t_service*>::iterator sv_iter;
    for (sv_iter = services.begin(); sv_iter != services.end(); ++sv_iter) {
        // gen service
        vector<t_function*> functions = (*sv_iter)->get_functions();
        vector<t_function*>::iterator fn_iter = functions.begin();
        for (; fn_iter != functions.end(); fn_iter++) {
            // gen function
            t_function* func = (*fn_iter);

            // gen request begin
            // gen rpc head
            f_json_ << "[" // rpc start
                  << "1,"  // version
                  << "\"" << (*sv_iter)->get_name() << ":" << func->get_name() << "\"," // service_name:func_name
                  << "1,"  // rpc type
                  << "1,"  // req seq
                  << "{";  // args start

            // gen args
            vector<t_field*> members = func->get_arglist()->get_members();
            vector<t_field*>::iterator mem_iter = members.begin();
            for (; mem_iter != members.end(); mem_iter++) {
                if (mem_iter != members.begin()) {
                    f_json_ << ",";
                }
                gen_field((*mem_iter));
            }

            f_json_ << "}" // args end
                  << "]"   // rpc end
                  << endl;
            // gen request end

            // gen response begin
            // gen rpc head
            f_json_ << "[" // rpc start
                  << "1,"  // version
                  << "\"" << (*sv_iter)->get_name() << ":" << func->get_name() << "\"," // service_name:func_name
                  << "2,"  // rpc type
                  << "1,"  // req seq
                  << "{";  // args start

            f_json_ << "\"success\""
                  << ":"  // pair separator
                  << "{"; // filed start
            gen_type_str(func->get_returntype());
            f_json_ << ":";
            gen_type_value(func->get_returntype());
            f_json_ << "}"; // filed end

            f_json_ << "}" // args end
                  << "]"   // rpc end
                  << endl;

            // TODO:  oneway/void返回值,这两种不要求返回消息生成，暂时不管
            
            // gen response end
        }
    }

    close_generator();
}

THRIFT_REGISTER_GENERATOR(json, "JSON",
    "   message: Generate request message template\n"
)
