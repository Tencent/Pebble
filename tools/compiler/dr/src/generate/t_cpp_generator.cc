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

/*
 * 修改描述
 * 1. 头文件路径及文件名
 * 2. 名字空间
 * 3. boost依赖
 * 4. client及processor处理部分(统一client接口，统一services接口)
 * 5. 暂时未用功能
 * 对thrift移植的要求:类名、名字空间路径、文件夹尽量保持一致
 */

#include <cassert>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/stat.h>

#include "../platform.h"
#include "t_oop_generator.h"

using std::map;
using std::ofstream;
using std::ostream;
using std::string;
using std::vector;

static const string endl = "\n";  // avoid ostream << std::endl flushes

/**
 * C++ code generator. This is legitimacy incarnate.
 *
 */
class t_cpp_generator : public t_oop_generator {
 public:
  t_cpp_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& option_string)
    : t_oop_generator(program)
  {
    gen_dense_ = false;
    use_include_prefix_ = false;
    gen_no_client_completion_ = false;
    gen_no_default_operators_ = false;
    gen_templates_ = false;
    gen_templates_only_ = false;

    gen_cob_style_ = true;
    gen_pure_enums_ = true;

    out_dir_base_ = "gen-cpp";
  }

  /**
   * Init and close methods
   */

  void init_generator();
  void close_generator();


  /**
   * Program-level generation functions
   */

  void generate_typedef(t_typedef* ttypedef);
  void generate_enum(t_enum* tenum);
  void generate_const(t_const* tconst);
  std::string get_const_value(t_type* type, t_const_value* value);
  void generate_forward_declaration(t_struct* tstruct);
  void generate_struct(t_struct* tstruct) {
    generate_cpp_struct(tstruct, false);
  }
  void generate_xception(t_struct* txception) {
    generate_cpp_struct(txception, true);
  }
  void generate_cpp_struct(t_struct* tstruct, bool is_exception);

  void generate_service(t_service* tservice);

  void print_const_value(std::ofstream& out, std::string name, t_type* type, t_const_value* value);
  std::string render_const_value(std::ofstream& out, std::string name, t_type* type, t_const_value* value);

  void generate_struct_declaration    (std::ofstream& out, t_struct* tstruct,
                                      bool is_exception=false,
                                      bool pointers=false,
                                      bool read=true,
                                      bool write=true,
                                      bool swap=false,
                                      bool reflection=false);
  void generate_struct_definition   (std::ofstream& out, std::ofstream& force_cpp_out, t_struct* tstruct, bool setters=true);
  void generate_copy_constructor     (std::ofstream& out, t_struct* tstruct, bool is_exception);
  void generate_assignment_operator  (std::ofstream& out, t_struct* tstruct);
  void generate_struct_fingerprint   (std::ofstream& out, t_struct* tstruct, bool is_definition);
  void generate_struct_reader        (std::ofstream& out, t_struct* tstruct, bool pointers=false);
  void generate_struct_writer        (std::ofstream& out, t_struct* tstruct, bool pointers=false);
  void generate_struct_result_writer (std::ofstream& out, t_struct* tstruct, bool pointers=false);
  void generate_struct_swap          (std::ofstream& out, t_struct* tstruct);
  void generate_struct_ostream_operator(std::ofstream& out, t_struct* tstruct);
  void generate_struct_reflection_info(std::ofstream& out, t_struct* tstruct);

  /**
   * Service-level generation functions
   */

  void generate_service_interface (t_service* tservice, string style);
  void generate_service_interface_factory (t_service* tservice, string style);
  void generate_service_null      (t_service* tservice, string style);
  void generate_service_multiface (t_service* tservice);
  void generate_service_helpers   (t_service* tservice);
  void generate_service_client    (t_service* tservice, string style);
  void generate_service_processor (t_service* tservice, string style);
  void generate_service_skeleton  (t_service* tservice);
  void generate_process_function  (t_service* tservice, t_function* tfunction,
                                   string style, bool specialized=false);
  void generate_return_function  (t_service* tservice, t_function* tfunction,
                                   string style, bool specialized=false);
  void generate_function_helpers  (t_service* tservice, t_function* tfunction);
  void generate_service_async_skeleton (t_service* tservice);

  /**
   * Serialization constructs
   */

  void generate_deserialize_field        (std::ofstream& out,
                                          t_field*    tfield,
                                          std::string iprot,
                                          std::string prefix="",
                                          std::string suffix="");

  void generate_deserialize_struct       (std::ofstream& out,
                                          t_struct*   tstruct,
                                          std::string iprot,
                                          std::string prefix="",
                                          bool pointer=false);

  void generate_deserialize_container    (std::ofstream& out,
                                          t_type*     ttype,
                                          std::string iprot,
                                          std::string prefix="");

  void generate_deserialize_set_element  (std::ofstream& out,
                                          t_set*      tset,
                                          std::string iprot,
                                          std::string prefix="");

  void generate_deserialize_map_element  (std::ofstream& out,
                                          t_map*      tmap,
                                          std::string iprot,
                                          std::string prefix="");

  void generate_deserialize_list_element (std::ofstream& out,
                                          t_list*     tlist,
                                          std::string iprot,
                                          std::string prefix,
                                          bool push_back,
                                          std::string index);

  void generate_serialize_field          (std::ofstream& out,
                                          t_field*    tfield,
                                          std::string oprot,
                                          std::string prefix="",
                                          std::string suffix="");

  void generate_serialize_struct         (std::ofstream& out,
                                          t_struct*   tstruct,
                                          std::string oprot,
                                          std::string prefix="",
                                          bool pointer=false);

  void generate_serialize_container      (std::ofstream& out,
                                          t_type*     ttype,
                                          std::string oprot,
                                          std::string prefix="");

  void generate_serialize_map_element    (std::ofstream& out,
                                          t_map*      tmap,
                                          std::string iter,
                                          std::string oprot);

  void generate_serialize_set_element    (std::ofstream& out,
                                          t_set*      tmap,
                                          std::string iter,
                                          std::string oprot);

  void generate_serialize_list_element   (std::ofstream& out,
                                          t_list*     tlist,
                                          std::string iter,
                                          std::string oprot);

  void generate_function_call            (ostream& out,
                                          t_function* tfunction,
                                          string target,
                                          string iface,
                                          string arg_prefix);
  /*
   * Helper rendering functions
   */

  std::string namespace_prefix(std::string ns);
  std::string namespace_open(std::string ns);
  std::string namespace_close(std::string ns);
  std::string type_name(t_type* ttype, bool in_typedef=false, bool arg=false);
  std::string base_type_name(t_base_type::t_base tbase);
  std::string declare_field(t_field* tfield, bool init=false, bool pointer=false, bool constant=false, bool reference=false, std::string alias="");
  std::string function_signature(t_function* tfunction, std::string style, std::string prefix="", bool name_params=true, std::string preargs="");
  std::string function_signature_if(t_function* tfunction, std::string style, std::string prefix="", bool name_params=true);
  std::string argument_list(t_struct* tstruct, bool name_params=true, bool start_comma=false);
  std::string type_to_enum(t_type* ttype);
  std::string local_reflection_name(const char*, t_type* ttype, bool external=false);

  void generate_enum_constant_list(std::ofstream& f,
                                   const vector<t_enum_value*>& constants,
                                   const char* prefix,
                                   const char* suffix,
                                   bool include_values);

  void generate_struct_ostream_operator_decl(std::ofstream& f, t_struct* tstruct);

  // These handles checking gen_dense_ and checking for duplicates.
  void generate_local_reflection(std::ofstream& out, t_type* ttype, bool is_definition);
  void generate_local_reflection_pointer(std::ofstream& out, t_type* ttype);

  bool is_reference(t_field* tfield) {
    return tfield->get_reference();
  }

  bool is_complex_type(t_type* ttype) {
    ttype = get_true_type(ttype);

    return
      ttype->is_container() ||
      ttype->is_struct() ||
      ttype->is_xception() ||
      (ttype->is_base_type() && (((t_base_type*)ttype)->get_base() == t_base_type::TYPE_STRING));
  }

  void set_use_include_prefix(bool use_include_prefix) {
    use_include_prefix_ = use_include_prefix;
  }

 private:
  /**
   * Returns the include prefix to use for a file generated by program, or the
   * empty string if no include prefix should be used.
   */
  std::string get_include_prefix(const t_program& program) const;

  /**
   * True if we should generate pure enums for Thrift enums, instead of wrapper classes.
   */
  bool gen_pure_enums_;

  /**
   * True if we should generate local reflection metadata for TDenseProtocol.
   */
  bool gen_dense_;

  /**
   * True if we should generate templatized reader/writer methods.
   */
  bool gen_templates_;

  /**
   * True iff we should generate process function pointers for only templatized
   * reader/writer methods.
   */
  bool gen_templates_only_;

  /**
   * True iff we should use a path prefix in our #include statements for other
   * thrift-generated header files.
   */
  bool use_include_prefix_;

  /**
   * True if we should generate "Continuation OBject"-style classes as well.
   */
  bool gen_cob_style_;

  /**
   * True if we should omit calls to completion__() in CobClient class.
   */
  bool gen_no_client_completion_;

  /**
   * True if we should omit generating the default opeartors ==, != and <.
   */
  bool gen_no_default_operators_;

  /**
   * Strings for namespace, computed once up front then used directly
   */

  std::string ns_open_;
  std::string ns_close_;

  /**
   * File streams, stored here to avoid passing them as parameters to every
   * function.
   */

  // 基本数据结构定义及实现
  std::ofstream f_types_h_;
  std::ofstream f_types_cpp_;
  // 每个服务独立的用户接口和实现
  std::ofstream f_service_h_;
  std::ofstream f_service_inh_;
  std::ofstream f_service_cpp_;

  /**
   * When generating local reflections, make sure we don't generate duplicates.
   */
  std::set<std::string> reflected_fingerprints_;

  // The ProcessorGenerator is used to generate parts of the code,
  // so it needs access to many of our protected members and methods.
  //
  // TODO: The code really should be cleaned up so that helper methods for
  // writing to the output files are separate from the generator classes
  // themselves.
  friend class ProcessorGenerator;
};

/**
 * Prepares for file generation by opening up the necessary file output
 * streams.
 * 生成文件命名规则:
 * filename.thrift
 *   filename.h
 *   filename.cpp
 *   filename_xxx.cpp  (xxx: service name)
 *   filename_xxx_if.h (xxx: service name)
 */
void t_cpp_generator::init_generator() {
  // Make output directory
  MKDIR(get_out_dir().c_str());

  // Make output file
  string f_types_name = get_out_dir()+program_name_+".h";
  f_types_h_.open(f_types_name.c_str());

  string f_types_impl_name = get_out_dir()+program_name_+".cpp";
  f_types_cpp_.open(f_types_impl_name.c_str());

  // Print header
  f_types_h_ <<
    autogen_comment();
  f_types_cpp_ <<
    autogen_comment();

  // Start ifndef
  f_types_h_ <<
    "#ifndef __" << program_name_ << "_h__" << endl <<
    "#define __" << program_name_ << "_h__" << endl <<
    endl;

  // Include base types
  f_types_h_ <<
    "#include <iosfwd>" << endl <<
    endl;

  f_types_h_ <<
      "#include \"framework/dr/common/common.h\"" << endl <<
      "#include \"framework/dr/common/field_pack.h\"" << endl <<
      "#include \"framework/dr/common/fp_util.h\"" << endl <<
      "#include \"framework/dr/common/reflection.h\"" <<
      endl;

  // Include other Thrift includes
  const vector<t_program*>& includes = program_->get_includes();
  for (size_t i = 0; i < includes.size(); ++i) {
    f_types_h_ <<
      "#include \"" << get_include_prefix(*(includes[i])) <<
      includes[i]->get_name() << ".h\"" << endl;
  }
  f_types_h_ << endl << endl;

  // Include custom headers
  const vector<string>& cpp_includes = program_->get_cpp_includes();
  for (size_t i = 0; i < cpp_includes.size(); ++i) {
    if (cpp_includes[i][0] == '<') {
      f_types_h_ <<
        "#include " << cpp_includes[i] << endl;
    } else {
      f_types_h_ <<
        "#include \"" << cpp_includes[i] << "\"" << endl;
    }
  }
  f_types_h_ <<
    endl;

  // The swap() code needs <algorithm> for std::swap()
  f_types_cpp_ << "#include <algorithm>" << endl;
  // for operator<<
  f_types_cpp_ << "#include <ostream>" << endl << endl;
  f_types_cpp_ << "#include \"framework/dr/common/to_string.h\"" << endl;
  f_types_cpp_ << "#include \"framework/dr/protocol/rapidjson_protocol.h\"" << endl;
  f_types_cpp_ << "#include \"framework/dr/serialize.h\"" << endl;
  f_types_cpp_ << "#include \"rapidjson/memorystream.h\"" << endl;
  f_types_cpp_ << "#include \"rapidjson/prettywriter.h\"" << endl;
  f_types_cpp_ << "#include \"rapidjson/reader.h\"" << endl <<
    endl;

  // Include the types file
  f_types_cpp_ <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ << ".h\"" << endl <<
    endl;

  // Open namespace
  ns_open_ = namespace_open(program_->get_namespace("cpp"));
  ns_close_ = namespace_close(program_->get_namespace("cpp"));

  f_types_h_ <<
    ns_open_ << endl <<
    endl;

  f_types_cpp_ <<
    ns_open_ << endl <<
    endl;
}

/**
 * Closes the output files.
 */
void t_cpp_generator::close_generator() {
  // Close namespace
  f_types_h_ << ns_close_ << endl << endl;
  f_types_cpp_ << ns_close_ << endl;

  // Close ifndef
  f_types_h_ << "#endif // __" << program_name_ << "_h__" << endl;

  // Close output file
  f_types_h_.close();
  f_types_cpp_.close();
}

/**
 * Generates a typedef. This is just a simple 1-liner in C++
 *
 * @param ttypedef The type definition
 */
void t_cpp_generator::generate_typedef(t_typedef* ttypedef) {
  f_types_h_ <<
    indent() << "typedef " << type_name(ttypedef->get_type(), true) << " " << ttypedef->get_symbolic() << ";" << endl <<
    endl;
}


void t_cpp_generator::generate_enum_constant_list(std::ofstream& f,
                                                  const vector<t_enum_value*>& constants,
                                                  const char* prefix,
                                                  const char* suffix,
                                                  bool include_values) {
  f << " {" << endl;
  indent_up();

  vector<t_enum_value*>::const_iterator c_iter;
  bool first = true;
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    if (first) {
      first = false;
    } else {
      f << "," << endl;
    }
    indent(f)
      << prefix << (*c_iter)->get_name() << suffix;
    if (include_values) {
      f << " = " << (*c_iter)->get_value();
    }
  }

  f << endl;
  indent_down();
  indent(f) << "};" << endl;
}

/**
 * Generates code for an enumerated type. In C++, this is essentially the same
 * as the thrift definition itself, using the enum keyword in C++.
 *
 * @param tenum The enumeration
 */
void t_cpp_generator::generate_enum(t_enum* tenum) {
  vector<t_enum_value*> constants = tenum->get_constants();

  std::string enum_name = tenum->get_name();
  if (!gen_pure_enums_) {
    enum_name = "type";
    f_types_h_ <<
      indent() << "struct " << tenum->get_name() << " {" << endl;
    indent_up();
  }
  f_types_h_ <<
    indent() << "enum " << enum_name;

  generate_enum_constant_list(f_types_h_, constants, "", "", true);

  if (!gen_pure_enums_) {
    indent_down();
    f_types_h_ << "};" << endl;
  }

  f_types_h_ << endl;

  /**
     Generate a character array of enum names for debugging purposes.
  */
  std::string prefix = "";
  if (!gen_pure_enums_) {
    prefix = tenum->get_name() + "::";
  }

  f_types_cpp_ <<
    indent() << "int _k" << tenum->get_name() << "Values[] =";
  generate_enum_constant_list(f_types_cpp_, constants, prefix.c_str(), "", false);

  f_types_cpp_ <<
    indent() << "const char* _k" << tenum->get_name() << "Names[] =";
  generate_enum_constant_list(f_types_cpp_, constants, "\"", "\"", false);

  f_types_h_ <<
    indent() << "extern const std::map<int, const char*> _" <<
    tenum->get_name() << "_VALUES_TO_NAMES;" << endl << endl;

  f_types_cpp_ <<
    indent() << "const std::map<int, const char*> _" << tenum->get_name() <<
    "_VALUES_TO_NAMES(::pebble::dr::TEnumIterator(" << constants.size() <<
    ", _k" << tenum->get_name() << "Values" <<
    ", _k" << tenum->get_name() << "Names), " <<
    "::pebble::dr::TEnumIterator(-1, NULL, NULL));" << endl << endl;

  generate_local_reflection(f_types_h_, tenum, false);
  generate_local_reflection(f_types_cpp_, tenum, true);
}

void t_cpp_generator::generate_const(t_const* tconst) {
    f_types_h_ << "static const " << type_name(tconst->get_type())
        << " " << tconst->get_name()
        << " = " << get_const_value(tconst->get_type(), tconst->get_value()) << ";"
        << endl << endl;
}

std::string t_cpp_generator::get_const_value(t_type* type, t_const_value* value) {
  std::ostringstream oss;
  std::string name;
  type = get_true_type(type);
  if (type->is_base_type()) {
    return render_const_value(f_types_h_, name, type, value);
  } else if (type->is_enum()) {
    oss << value->get_integer();
    return oss.str();
  } else {
    throw "INVALID TYPE for const define" + type->get_name();
  }
}


/**
 * Prints the value of a constant with the given type. Note that type checking
 * is NOT performed in this function as it is always run beforehand using the
 * validate_types method in main.cc
 */
void t_cpp_generator::print_const_value(ofstream& out, string name, t_type* type, t_const_value* value) {
  type = get_true_type(type);
  if (type->is_base_type()) {
    string v2 = render_const_value(out, name, type, value);
    indent(out) << name << " = " << v2 << ";" << endl <<
      endl;
  } else if (type->is_enum()) {
    indent(out) << name << " = (" << type_name(type) << ")" << value->get_integer() << ";" << endl <<
      endl;
  } else if (type->is_struct() || type->is_xception()) {
    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;
    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
    bool is_nonrequired_field = false;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      t_type* field_type = NULL;
      is_nonrequired_field = false;
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if ((*f_iter)->get_name() == v_iter->first->get_string()) {
          field_type = (*f_iter)->get_type();
          is_nonrequired_field = (*f_iter)->get_req() != t_field::T_REQUIRED;
        }
      }
      if (field_type == NULL) {
        throw "type error: " + type->get_name() + " has no field " + v_iter->first->get_string();
      }
      string val = render_const_value(out, name, field_type, v_iter->second);
      indent(out) << name << "." << v_iter->first->get_string() << " = " << val << ";" << endl;
      if(is_nonrequired_field) {
          indent(out) << name << ".__isset." << v_iter->first->get_string() << " = true;" << endl;
      }
    }
    out << endl;
  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();
    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      string key = render_const_value(out, name, ktype, v_iter->first);
      string val = render_const_value(out, name, vtype, v_iter->second);
      indent(out) << name << ".insert(std::make_pair(" << key << ", " << val << "));" << endl;
    }
    out << endl;
  } else if (type->is_list()) {
    t_type* etype = ((t_list*)type)->get_elem_type();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      string val = render_const_value(out, name, etype, *v_iter);
      indent(out) << name << ".push_back(" << val << ");" << endl;
    }
    out << endl;
  } else if (type->is_set()) {
    t_type* etype = ((t_set*)type)->get_elem_type();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      string val = render_const_value(out, name, etype, *v_iter);
      indent(out) << name << ".insert(" << val << ");" << endl;
    }
    out << endl;
  } else {
    throw "INVALID TYPE IN print_const_value: " + type->get_name();
  }
}

/**
 *
 */
string t_cpp_generator::render_const_value(ofstream& out, string name, t_type* type, t_const_value* value) {
  (void) name;
  std::ostringstream render;

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_STRING:
      render << '"' << get_escaped_string(value) << '"';
      break;
    case t_base_type::TYPE_BOOL:
      render << ((value->get_integer() > 0) ? "true" : "false");
      break;
    case t_base_type::TYPE_BYTE:
    case t_base_type::TYPE_I16:
    case t_base_type::TYPE_I32:
      render << value->get_integer();
      break;
    case t_base_type::TYPE_I64:
      render << value->get_integer() << "LL";
      break;
    case t_base_type::TYPE_DOUBLE:
      if (value->get_type() == t_const_value::CV_INTEGER) {
        render << value->get_integer();
      } else {
        render << value->get_double();
      }
      break;
    default:
      throw "compiler error: no const of base type " + t_base_type::t_base_name(tbase);
    }
  } else if (type->is_enum()) {
    render << "(" << type_name(type) << ")" << value->get_integer();
  } else {
    string t = tmp("tmp");
    indent(out) << type_name(type) << " " << t << ";" << endl;
    print_const_value(out, t, type, value);
    render << t;
  }

  return render.str();
}

void t_cpp_generator::generate_forward_declaration(t_struct* tstruct) {
  // Forward declare struct def
  f_types_h_ <<
    indent() << "class " << tstruct->get_name() << ";" << endl <<
    endl;
}

/**
 * Generates a struct definition for a thrift data type. This is a class
 * with data members and a read/write() function, plus a mirroring isset
 * inner class.
 *
 * @param tstruct The struct definition
 */
void t_cpp_generator::generate_cpp_struct(t_struct* tstruct, bool is_exception) {
  generate_struct_declaration(f_types_h_, tstruct, is_exception,
                             false, true, true, true, (!tstruct->is_union()) && (!tstruct->is_xception()));
  generate_struct_definition(f_types_cpp_, f_types_cpp_, tstruct);
  generate_struct_fingerprint(f_types_cpp_, tstruct, true);
  generate_local_reflection(f_types_h_, tstruct, false);
  generate_local_reflection(f_types_cpp_, tstruct, true);
  generate_local_reflection_pointer(f_types_cpp_, tstruct);

  std::ofstream& out = (f_types_cpp_);
  generate_struct_reader(out, tstruct);
  generate_struct_writer(out, tstruct);
  generate_struct_swap(f_types_cpp_, tstruct);
  generate_copy_constructor(f_types_cpp_, tstruct, is_exception);
  generate_assignment_operator(f_types_cpp_, tstruct);
  generate_struct_ostream_operator(f_types_cpp_, tstruct);
  generate_struct_reflection_info(f_types_cpp_, tstruct);
}

void t_cpp_generator::generate_copy_constructor(
  ofstream& out,
  t_struct* tstruct,
  bool is_exception) {
  std::string tmp_name = tmp("other");

  indent(out) << tstruct->get_name() << "::" <<
    tstruct->get_name() << "(const " << tstruct->get_name() <<
    "& " << tmp_name << ") ";
  if (is_exception)
    out << ": pebble::TException() ";
  out << "{" << endl;
  indent_up();

  const vector<t_field*>& members = tstruct->get_members();

  // eliminate compiler unused warning
  if (members.empty())
    indent(out) << "(void) " << tmp_name << ";" << endl;

  vector<t_field*>::const_iterator f_iter;
  bool has_nonrequired_fields = false;
  for (f_iter = members.begin(); f_iter != members.end(); ++f_iter) {
    if ((*f_iter)->get_req() != t_field::T_REQUIRED)
      has_nonrequired_fields = true;
    indent(out) << (*f_iter)->get_name() << " = " << tmp_name << "." <<
      (*f_iter)->get_name() << ";" << endl;
  }

  if (has_nonrequired_fields)
    indent(out) << "__isset = " << tmp_name << ".__isset;" << endl;

  indent_down();
  indent(out) << "}" << endl;
}

void t_cpp_generator::generate_assignment_operator(
  ofstream& out,
  t_struct* tstruct) {
  std::string tmp_name = tmp("other");

  indent(out) << tstruct->get_name() << "& " << tstruct->get_name() << "::"
    "operator=(const " << tstruct->get_name() <<
    "& " << tmp_name << ") {" << endl;
  indent_up();

  const vector<t_field*>& members = tstruct->get_members();

  // eliminate compiler unused warning
  if (members.empty())
    indent(out) << "(void) " << tmp_name << ";" << endl;

  vector<t_field*>::const_iterator f_iter;
  bool has_nonrequired_fields = false;
  for (f_iter = members.begin(); f_iter != members.end(); ++f_iter) {
    if ((*f_iter)->get_req() != t_field::T_REQUIRED)
      has_nonrequired_fields = true;
    indent(out) << (*f_iter)->get_name() << " = " << tmp_name << "." <<
      (*f_iter)->get_name() << ";" << endl;
  }
  if (has_nonrequired_fields)
    indent(out) << "__isset = " << tmp_name << ".__isset;" << endl;

  indent(out) << "return *this;" << endl;
  indent_down();
  indent(out) << "}" << endl;
}

/**
 * Writes the struct declaration into the header file
 *
 * @param out Output stream
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_declaration(ofstream& out,
                                                 t_struct* tstruct,
                                                 bool is_exception,
                                                 bool pointers,
                                                 bool read,
                                                 bool write,
                                                 bool swap,
                                                 bool reflection) {
  string extends = "";
  if (is_exception) {
    extends = " : public ::pebble::TException";
  } else {
    extends = "";
  }

  // Get members
  vector<t_field*>::const_iterator m_iter;
  const vector<t_field*>& members = tstruct->get_members();

  // Write the isset structure declaration outside the class. This makes
  // the generated code amenable to processing by SWIG.
  // We only declare the struct if it gets used in the class.

  // Isset struct has boolean fields, but only for non-required fields.
  bool has_nonrequired_fields = false;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if ((*m_iter)->get_req() != t_field::T_REQUIRED)
      has_nonrequired_fields = true;
  }

  if (has_nonrequired_fields && (!pointers || read)) {

    out <<
      indent() << "typedef struct _" << tstruct->get_name() << "__isset {" << endl;
    indent_up();

    indent(out) <<
      "_" << tstruct->get_name() << "__isset() ";
    bool first = true;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      if ((*m_iter)->get_req() == t_field::T_REQUIRED) {
        continue;
      }
      string isSet = ((*m_iter)->get_value() != NULL) ? "true" : "false";
      if (first) {
        first = false;
        out <<
          ": " << (*m_iter)->get_name() << "(" << isSet << ")";
      } else {
        out <<
          ", " << (*m_iter)->get_name() << "(" << isSet << ")";
      }
    }
    out << " {}" << endl;

    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      if ((*m_iter)->get_req() != t_field::T_REQUIRED) {
        indent(out) <<
          "bool " << (*m_iter)->get_name() << " :1;" << endl;
        }
      }

      indent_down();
      indent(out) <<
        "} _" << tstruct->get_name() << "__isset;" << endl;
    }

  out << endl;

  // Open struct def
  out <<
    indent() << "class " << tstruct->get_name() << extends << " {" << endl <<
    indent() << "public:" << endl;
  indent_up();

  // Put the fingerprint up top for all to see.
  generate_struct_fingerprint(out, tstruct, false);

  if (!pointers) {
    // Copy constructor
    indent(out) <<
      tstruct->get_name() << "(const " << tstruct->get_name() << "&);" << endl;

    // Assignment Operator
    indent(out) << tstruct->get_name() << "& operator=(const " << tstruct->get_name() << "&);" << endl;

    // Default constructor
    indent(out) <<
      tstruct->get_name() << "()";


    bool init_ctor = false;

    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      t_type* t = get_true_type((*m_iter)->get_type());
      if (t->is_base_type() || t->is_enum() || is_reference(*m_iter)) {
        string dval;
        if (t->is_enum()) {
          dval += "(" + type_name(t) + ")";
        }
        dval += (t->is_string() || is_reference(*m_iter)) ? "" : "0";
        t_const_value* cv = (*m_iter)->get_value();
        if (cv != NULL) {
          dval = render_const_value(out, (*m_iter)->get_name(), t, cv);
        }
        if (!init_ctor) {
          init_ctor = true;
          out << " : ";
          out << (*m_iter)->get_name() << "(" << dval << ")";
        } else {
          out << ", " << (*m_iter)->get_name() << "(" << dval << ")";
        }
      }
    }
    out << " {" << endl;
    indent_up();
    // TODO(dreiss): When everything else in Thrift is perfect,
    // do more of these in the initializer list.
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      t_type* t = get_true_type((*m_iter)->get_type());

      if (!t->is_base_type()) {
        t_const_value* cv = (*m_iter)->get_value();
        if (cv != NULL) {
          print_const_value(out, (*m_iter)->get_name(), t, cv);
        }
      }
    }
    scope_down(out);
  }

  if (tstruct->annotations_.find("final") == tstruct->annotations_.end()) {
    out <<
      endl <<
      indent() << "virtual ~" << tstruct->get_name() << "() throw();" << endl;
  }

  // Pointer to this structure's reflection local typespec.
  if (gen_dense_) {
    indent(out) <<
      "static ::pebble::dr::reflection::local::TypeSpec* local_reflection;" <<
      endl << endl;
  }

  // Declare all fields
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    indent(out) <<
      declare_field(*m_iter, false, (pointers && !(*m_iter)->get_type()->is_xception()), !read) << endl;
  }

  // Add the __isset data member if we need it, using the definition from above
  if (has_nonrequired_fields && (!pointers || read)) {
    out <<
      endl <<
      indent() << "_" << tstruct->get_name() << "__isset __isset;" << endl;
  }

  // Create a setter function for each field
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if (pointers) {
      continue;
    }
    if (is_reference((*m_iter))) {
      out <<
        endl <<
        indent() << "void __set_" << (*m_iter)->get_name() <<
        "(cxx::shared_ptr<" << type_name((*m_iter)->get_type(), false, false) << ">";
      out << " val);" << endl;
    } else {
      out <<
        endl <<
        indent() << "void __set_" << (*m_iter)->get_name() <<
        "(" << type_name((*m_iter)->get_type(), false, true);
      out << " val);" << endl;
    }
  }
  out << endl;

  if (!pointers) {
    // Should we generate default operators?
    if (!gen_no_default_operators_) {
      // Generate an equality testing operator.  Make it inline since the compiler
      // will do a better job than we would when deciding whether to inline it.
      out <<
        indent() << "bool operator == (const " << tstruct->get_name() << " & " <<
        (members.size() > 0 ? "rhs" : "/* rhs */") << ") const" << endl;
      scope_up(out);
      for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        // Most existing Thrift code does not use isset or optional/required,
        // so we treat "default" fields as required.
        t_type* t = get_true_type((*m_iter)->get_type());
        if (t->is_base_type() && t_base_type::TYPE_DOUBLE == ((t_base_type*)t)->get_base())
        {
            if ((*m_iter)->get_req() != t_field::T_OPTIONAL) {
                out <<
                    indent() << "if (!::pebble::dr::floating_point::AlmostEquals(" << (*m_iter)->get_name()
                    << " , rhs." << (*m_iter)->get_name() << "))" << endl <<
                    indent(1) << "return false;" << endl;
            } else {
                out <<
                    indent() << "if (__isset." << (*m_iter)->get_name()
                    << " != rhs.__isset." << (*m_iter)->get_name() << ")" << endl <<
                    indent(1) << "return false;" << endl <<
                    indent() << "else if (__isset." << (*m_iter)->get_name() << " && !::pebble::dr::floating_point::AlmostEquals("
                    << (*m_iter)->get_name() << " , rhs." << (*m_iter)->get_name()
                    << "))" << endl <<
                    indent(1) << "return false;" << endl;
            }
        }else if ((*m_iter)->get_req() != t_field::T_OPTIONAL) {
          out <<
            indent() << "if (!(" << (*m_iter)->get_name()
                     << " == rhs." << (*m_iter)->get_name() << "))" << endl <<
            indent(1) << "return false;" << endl;
        } else {
          out <<
            indent() << "if (__isset." << (*m_iter)->get_name()
                     << " != rhs.__isset." << (*m_iter)->get_name() << ")" << endl <<
            indent(1) << "return false;" << endl <<
            indent() << "else if (__isset." << (*m_iter)->get_name() << " && !("
                     << (*m_iter)->get_name() << " == rhs." << (*m_iter)->get_name()
                     << "))" << endl <<
            indent(1) << "return false;" << endl;
        }
      }
      indent(out) << "return true;" << endl;
      scope_down(out);
      out <<
        indent() << "bool operator != (const " << tstruct->get_name() << " &rhs) const {" << endl <<
        indent(1) << "return !(*this == rhs);" << endl <<
        indent() << "}" << endl << endl;

      // Generate the declaration of a less-than operator.  This must be
      // implemented by the application developer if they wish to use it.  (They
      // will get a link error if they try to use it without an implementation.)
      out <<
        indent() << "// This must be implemented by the developer " <<
        "if they wish to use it." << endl;
      out <<
        indent() << "bool operator < (const "
                 << tstruct->get_name() << " &) const;" << endl << endl;
    }
  }

  if (read) {
      out <<
        indent() << "uint32_t read(" <<
        "::pebble::dr::protocol::TProtocol* iprot);" << endl;
  }
  if (write) {
      out <<
        indent() << "uint32_t write(" <<
        "::pebble::dr::protocol::TProtocol* oprot) const;" << endl;
  }
  out << endl;

  if (read) {
    out <<
        indent() << "// 从binary码流和json字符串反序列化，返回<0时表示失败，成功时返回处理的长度" << endl <<
        indent() << "int read(char *buff, size_t len);" << endl <<
        indent() << "int fromJson(const std::string &json);" << endl;
  }
  if (write) {
    out <<
        indent() << "// 序列化到binary码流和json字符串，返回<0时表示失败，成功时返回处理的长度" << endl <<
        indent() << "int write(char *buff, size_t len) const;" << endl <<
        indent() << "int toJson(std::string *str, bool pretty = false) const;" << endl;
  }
  out << endl;

  // ostream operator<<
  out << indent() << "friend ";
  generate_struct_ostream_operator_decl(out, tstruct);
  out << ";" << endl;

  if (reflection) {
    out << indent() << "static const pebble::dr::reflection::TypeInfo* S_GetTypeInfo();" << endl << endl;
    out << indent() << "virtual const pebble::dr::reflection::TypeInfo* GetTypeInfo();" << endl << endl;
    out << "private:" << endl << endl;
    out << indent() << "static pebble::dr::reflection::TypeInfo* s_type_info;" << endl << endl;

    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        out << indent() << "class FieldInfo_" << (*m_iter)->get_name() << " : public ::pebble::dr::reflection::FieldInfo {" << endl;
        out << indent() << "public:" << endl;
        indent_up();
        out << indent() << "FieldInfo_" << (*m_iter)->get_name() << "(std::string field_name, int16_t field_id,"
            << "pebble::dr::protocol::TType field_type,"
            << "bool is_optional, ::pebble::dr::reflection::AttributesType attributes) " << endl;
        out << indent(1) << ": FieldInfo(field_name, field_id, field_type, is_optional, attributes) {}" << endl << endl;
        out << indent() << "virtual int Pack(void *obj, uint8_t *buff, uint32_t buff_len) const;" << endl << endl;
        out << indent() << "virtual int UnPack(void *obj, uint8_t *buff, uint32_t buff_len) const;" << endl << endl;
        indent_down();
        out << indent() << "};" << endl << endl;;
    }
  }

  indent_down();
  indent(out) <<
    "};" << endl <<
    endl;

  if (swap) {
    // Generate a namespace-scope swap() function
    out <<
      indent() << "void swap(" << tstruct->get_name() << " &a, " <<
      tstruct->get_name() << " &b);" << endl <<
      endl;
  }
}

void t_cpp_generator::generate_struct_definition(ofstream& out,
                                                 ofstream& force_cpp_out,
                                                 t_struct* tstruct,
                                                 bool setters) {
  // Get members
  vector<t_field*>::const_iterator m_iter;
  const vector<t_field*>& members = tstruct->get_members();


  // Destructor
  if (tstruct->annotations_.find("final") == tstruct->annotations_.end()) {
    force_cpp_out <<
      endl <<
      indent() << tstruct->get_name() << "::~" << tstruct->get_name() << "() throw() {" << endl;
    indent_up();

    indent_down();
    force_cpp_out << indent() << "}" << endl << endl;
  }

  // Create a setter function for each field
  if (setters) {
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      if (is_reference((*m_iter))) {
        std::string type = type_name((*m_iter)->get_type());
        out <<
          endl <<
          indent() << "void " << tstruct->get_name() << "::__set_" << (*m_iter)->get_name() <<
          "(cxx::shared_ptr<" << type_name((*m_iter)->get_type(), false, false) << ">";
        out << " val) {" << endl;
      } else {
        out <<
          endl <<
          indent() << "void " << tstruct->get_name() << "::__set_" << (*m_iter)->get_name() <<
          "(" << type_name((*m_iter)->get_type(), false, true);
        out << " val) {" << endl;
      }
      indent_up();
      out << indent() << "this->" << (*m_iter)->get_name() << " = val;" << endl;
      indent_down();

      // assume all fields are required except optional fields.
      // for optional fields change __isset.name to true
      bool is_optional = (*m_iter)->get_req() == t_field::T_OPTIONAL;
      if (is_optional) {
        out <<
          indent() <<
          indent() << "__isset." << (*m_iter)->get_name() << " = true;" << endl;
      }
      out <<
        indent()<< "}" << endl;
    }
  }
  out << endl;
}
/**
 * Writes the fingerprint of a struct to either the header or implementation.
 *
 * @param out Output stream
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_fingerprint(ofstream& out,
                                                  t_struct* tstruct,
                                                  bool is_definition) {
  string stat, nspace, comment;
  if (is_definition) {
    stat = "";
    nspace = tstruct->get_name() + "::";
    comment = " ";
  } else {
    stat = "static ";
    nspace = "";
    comment = "; // ";
  }

  if (! tstruct->has_fingerprint()) {
    tstruct->generate_fingerprint();  // lazy fingerprint generation
  }
  if (tstruct->has_fingerprint()) {
    out <<
      indent() << stat << "const char* " << nspace
        << "ascii_fingerprint" << comment << "= \"" <<
        tstruct->get_ascii_fingerprint() << "\";" << endl <<
      indent() << stat << "const uint8_t " << nspace <<
        "binary_fingerprint[" << t_type::fingerprint_len << "]" << comment << "= {";
    const char* comma = "";
    for (int i = 0; i < t_type::fingerprint_len; i++) {
      out << comma << "0x" << t_struct::byte_to_hex(tstruct->get_binary_fingerprint()[i]);
      comma = ", ";
    }
    out << "};" << endl << endl;
  }
}

/**
 * Writes the local reflection of a type (either declaration or definition).
 */
void t_cpp_generator::generate_local_reflection(std::ofstream& out,
                                                t_type* ttype,
                                                bool is_definition) {
  if (!gen_dense_) {
    return;
  }
  ttype = get_true_type(ttype);
  string key = ttype->get_ascii_fingerprint() + (is_definition ? "-defn" : "-decl");
  assert(ttype->has_fingerprint());  // test AFTER get due to lazy fingerprint generation

  // Note that we have generated this fingerprint.  If we already did, bail out.
  if (!reflected_fingerprints_.insert(key).second) {
    return;
  }
  // Let each program handle its own structures.
  if (ttype->get_program() != NULL && ttype->get_program() != program_) {
    return;
  }

  // Do dependencies.
  if (ttype->is_list()) {
    generate_local_reflection(out, ((t_list*)ttype)->get_elem_type(), is_definition);
  } else if (ttype->is_set()) {
    generate_local_reflection(out, ((t_set*)ttype)->get_elem_type(), is_definition);
  } else if (ttype->is_map()) {
    generate_local_reflection(out, ((t_map*)ttype)->get_key_type(), is_definition);
    generate_local_reflection(out, ((t_map*)ttype)->get_val_type(), is_definition);
  } else if (ttype->is_struct() || ttype->is_xception()) {
    // Hacky hacky.  For efficiency and convenience, we need a dummy "T_STOP"
    // type at the end of our typespec array.  Unfortunately, there is no
    // T_STOP type, so we use the global void type, and special case it when
    // generating its typespec.

    const vector<t_field*>& members = ((t_struct*)ttype)->get_sorted_members();
    vector<t_field*>::const_iterator m_iter;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      generate_local_reflection(out, (**m_iter).get_type(), is_definition);
    }
    generate_local_reflection(out, g_type_void, is_definition);

    // For definitions of structures, do the arrays of metas and field specs also.
    if (is_definition) {
      out <<
        indent() << "::pebble::dr::reflection::local::FieldMeta" << endl <<
        indent() << local_reflection_name("metas", ttype) <<"[] = {" << endl;
      indent_up();
      for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        indent(out) << "{ " << (*m_iter)->get_key() << ", " <<
          (((*m_iter)->get_req() == t_field::T_OPTIONAL) ? "true" : "false") <<
          " }," << endl;
      }
      // Zero for the T_STOP marker.
      indent(out) << "{ 0, false }" << endl << "};" << endl;
      indent_down();

      out <<
        indent() << "::pebble::dr::reflection::local::TypeSpec*" << endl <<
        indent() << local_reflection_name("specs", ttype) <<"[] = {" << endl;
      indent_up();
      for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        indent(out) << "&" <<
          local_reflection_name("typespec", (*m_iter)->get_type(), true) << "," << endl;
      }
      indent(out) << "&" <<
        local_reflection_name("typespec", g_type_void) << "," << endl;
      indent_down();
      indent(out) << "};" << endl;
    }
  }

  out <<
    indent() << "// " << ttype->get_fingerprint_material() << endl <<
    indent() << (is_definition ? "" : "extern ") <<
      "::pebble::dr::reflection::local::TypeSpec" << endl <<
      local_reflection_name("typespec", ttype) <<
      (is_definition ? "(" : ";") << endl;

  if (!is_definition) {
    out << endl;
    return;
  }

  indent_up();

  if (ttype->is_void()) {
    indent(out) << "::pebble::dr::protocol::T_STOP";
  } else {
    indent(out) << type_to_enum(ttype);
  }

  if (ttype->is_struct()) {
    out << "," << endl <<
      indent() << type_name(ttype) << "::binary_fingerprint," << endl <<
      indent() << local_reflection_name("metas", ttype) << "," << endl <<
      indent() << local_reflection_name("specs", ttype);
  } else if (ttype->is_list()) {
    out << "," << endl <<
      indent() << "&" << local_reflection_name("typespec", ((t_list*)ttype)->get_elem_type(), true) << "," << endl <<
      indent() << "NULL";
  } else if (ttype->is_set()) {
    out << "," << endl <<
      indent() << "&" << local_reflection_name("typespec", ((t_set*)ttype)->get_elem_type(), true) << "," << endl <<
      indent() << "NULL";
  } else if (ttype->is_map()) {
    out << "," << endl <<
      indent() << "&" << local_reflection_name("typespec", ((t_map*)ttype)->get_key_type(), true) << "," << endl <<
      indent() << "&" << local_reflection_name("typespec", ((t_map*)ttype)->get_val_type(), true);
  }

  out << ");" << endl << endl;

  indent_down();
}

/**
 * Writes the structure's static pointer to its local reflection typespec
 * into the implementation file.
 */
void t_cpp_generator::generate_local_reflection_pointer(std::ofstream& out,
                                                        t_type* ttype) {
  if (!gen_dense_) {
    return;
  }
  indent(out) <<
    "::pebble::dr::reflection::local::TypeSpec* " <<
      ttype->get_name() << "::local_reflection = " << endl <<
    indent(1) << "&" << local_reflection_name("typespec", ttype) << ";" <<
    endl << endl;
}

/**
 * Makes a helper function to gen a struct reader.
 *
 * @param out Stream to write to
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_reader(ofstream& out,
                                             t_struct* tstruct,
                                             bool pointers) {
  indent(out) <<
    "int " << tstruct->get_name() <<
    "::read(char *buff, size_t buff_len) {" << endl;
  indent_up();

  indent(out) << "return pebble::dr::UnPack<"<< tstruct->get_name() <<
    ", pebble::dr::protocol::TBinaryProtocol>(this, reinterpret_cast<uint8_t*>(buff), static_cast<uint32_t>(buff_len));" <<
    endl;

  indent_down();
  indent(out) <<
    "}" << endl << endl;

  indent(out) <<
    "int " << tstruct->get_name() <<
    "::fromJson(const std::string &json) {" << endl;
  indent_up();

  indent(out) << "return pebble::dr::UnPack<"<< tstruct->get_name() <<
    ", pebble::dr::protocol::TRAPIDJSONProtocol>(this, json);" <<
    endl;

  indent_down();
  indent(out) <<
    "}" << endl << endl;

  if (gen_templates_) {
    out <<
      indent() << "template <class Protocol_>" << endl <<
      indent() << "uint32_t " << tstruct->get_name() <<
      "::read(Protocol_* iprot) {" << endl;
  } else {
    indent(out) <<
      "uint32_t " << tstruct->get_name() <<
      "::read(::pebble::dr::protocol::TProtocol* iprot) {" << endl;
  }
  indent_up();

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  // Declare stack tmp variables
  out <<
    endl <<
    indent() << "uint32_t xfer = 0;" << endl <<
    indent() << "std::string fname;" << endl <<
    indent() << "::pebble::dr::protocol::TType ftype;" << endl <<
    indent() << "int16_t fid;" << endl <<
    endl <<
    indent() << "xfer += iprot->readStructBegin(fname);" << endl <<
    endl <<
    indent() << "using ::pebble::dr::protocol::TProtocolException;" << endl <<
    endl;

  // Required variables aren't in __isset, so we need tmp vars to check them.
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if ((*f_iter)->get_req() == t_field::T_REQUIRED)
      indent(out) << "bool isset_" << (*f_iter)->get_name() << " = false;" << endl;
  }
  out << endl;


  // Loop over reading in fields
  indent(out) <<
    "while (true)" << endl;
    scope_up(out);

    // Read beginning field marker
    indent(out) <<
      "xfer += iprot->readFieldBegin(fname, ftype, fid);" << endl;

    // Check for field STOP marker
    out <<
      indent() << "if (ftype == ::pebble::dr::protocol::T_STOP) {" << endl <<
      indent() << indent() << "break;" << endl <<
      indent() << "}" << endl;

    if(fields.empty()) {
      out <<
        indent() << "xfer += iprot->skip(ftype);" << endl;
    }
    else {
      // Switch statement on the field we are reading
      out << indent() << "if (fid == -1) {" << endl;
      indent_up();
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
          out << indent() <<(f_iter == fields.begin() ? "" : "else ") <<
              "if (fname == \"" << (*f_iter)->get_name() << "\") {" << endl;
          out << indent(1) << "fid = " << (*f_iter)->get_key() << ";" << endl;
          out << indent() << "}" << endl;
      }
      indent_down();
      out << indent() << "}" << endl;

      indent(out) <<
        "switch (fid)" << endl;

        scope_up(out);

        // Generate deserialization code for known cases
        for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
          indent(out) <<
            "case " << (*f_iter)->get_key() << ":" << endl;
          indent_up();
          indent(out) <<
            "if (ftype == ::pebble::dr::protocol::T_NULL || ftype == " << type_to_enum((*f_iter)->get_type()) << ") {" << endl;
          indent_up();

          const char *isset_prefix =
            ((*f_iter)->get_req() != t_field::T_REQUIRED) ? "this->__isset." : "isset_";

#if 0
          // This code throws an exception if the same field is encountered twice.
          // We've decided to leave it out for performance reasons.
          // TODO(dreiss): Generate this code and "if" it out to make it easier
          // for people recompiling thrift to include it.
          out <<
            indent() << "if (" << isset_prefix << (*f_iter)->get_name() << ")" << endl <<
            indent() << "  throw TProtocolException(TProtocolException::INVALID_DATA);" << endl;
#endif

          if (pointers && !(*f_iter)->get_type()->is_xception()) {
            generate_deserialize_field(out, *f_iter, "iprot", "(*(this->", "))");
          } else {
            generate_deserialize_field(out, *f_iter, "iprot", "this->");
          }
          out <<
            indent() << isset_prefix << (*f_iter)->get_name() << " = true;" << endl;
          indent_down();
          out <<
            indent() << "} else {" << endl <<
            indent(1) << "xfer += iprot->skip(ftype);" << endl <<
            // TODO(dreiss): Make this an option when thrift structs
            // have a common base class.
            // indent() << "  throw TProtocolException(TProtocolException::INVALID_DATA);" << endl <<
            indent() << "}" << endl <<
            indent() << "break;" << endl;
          indent_down();
      }

      // In the default case we skip the field
      out <<
        indent() << "default:" << endl <<
        indent(1) << "xfer += iprot->skip(ftype);" << endl <<
        indent(1) << "break;" << endl;

      scope_down(out);
    } //!fields.empty()
    // Read field end marker
    indent(out) <<
      "xfer += iprot->readFieldEnd();" << endl;

    scope_down(out);

  out <<
    endl <<
    indent() << "xfer += iprot->readStructEnd();" << endl;

  // Throw if any required fields are missing.
  // We do this after reading the struct end so that
  // there might possibly be a chance of continuing.
  out << endl;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if ((*f_iter)->get_req() == t_field::T_REQUIRED)
      out <<
        indent() << "if (!isset_" << (*f_iter)->get_name() << ')' << endl <<
        indent(1) << "throw TProtocolException(TProtocolException::INVALID_DATA);" << endl;
  }

  indent(out) << "return xfer;" << endl;

  indent_down();
  indent(out) <<
    "}" << endl << endl;
}

/**
 * Generates the write function.
 *
 * @param out Stream to write to
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_writer(ofstream& out,
                                             t_struct* tstruct,
                                             bool pointers) {
  string name = tstruct->get_name();
  const vector<t_field*>& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;

  indent(out) <<
    "int " << tstruct->get_name() <<
    "::write(char *buff, size_t buff_len) const {" << endl;
  indent_up();

  indent(out) << "return pebble::dr::Pack<"<< tstruct->get_name() <<
    ", pebble::dr::protocol::TBinaryProtocol>(this, reinterpret_cast<uint8_t*>(buff), static_cast<uint32_t>(buff_len));" <<
    endl;

  indent_down();

  indent(out) <<
    "}" << endl << endl;


  indent(out) <<
    "int " << tstruct->get_name() <<
    "::toJson(std::string *str, bool pretty) const {" << endl;
  indent_up();

  indent(out) << "int ret = pebble::dr::Pack<"<< tstruct->get_name() <<
    ", pebble::dr::protocol::TRAPIDJSONProtocol>(this, str);" <<
    endl;

  indent(out) << "if (ret > 0 && pretty) {" << endl;
  indent_up();
  indent(out) << "rapidjson::Reader reader;" << endl;
  indent(out) << "rapidjson::MemoryStream ms(str->c_str(), str->size());" << endl;
  indent(out) << "rapidjson::StringBuffer sb;" << endl;
  indent(out) << "rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);" << endl;
  indent(out) << "if (!reader.Parse(ms, writer)) return pebble::dr::kUNKNOW;" << endl;
  indent(out) << "str->assign(sb.GetString(), sb.GetSize());" << endl;
  indent_down();
  indent(out) << "}" << endl;

  indent(out) << "return ret;" << endl;

  indent_down();

  indent(out) <<
    "}" << endl << endl;


  if (gen_templates_) {
    out <<
      indent() << "template <class Protocol_>" << endl <<
      indent() << "uint32_t " << tstruct->get_name() <<
      "::write(Protocol_* oprot) const {" << endl;
  } else {
    indent(out) <<
      "uint32_t " << tstruct->get_name() <<
      "::write(::pebble::dr::protocol::TProtocol* oprot) const {" << endl;
  }
  indent_up();

  out <<
    indent() << "uint32_t xfer = 0;" << endl;

  indent(out) << "oprot->incrementRecursionDepth();" << endl;
  indent(out) <<
    "xfer += oprot->writeStructBegin(\"" << name << "\");" << endl;

  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    bool check_if_set = (*f_iter)->get_req() == t_field::T_OPTIONAL ||
                        (*f_iter)->get_type()->is_xception();
    if (check_if_set) {
      out << endl << indent() << "if (this->__isset." << (*f_iter)->get_name() << ") {" << endl;
      indent_up();
    } else {
      out << endl;
    }

    // Write field header
    out <<
      indent() << "xfer += oprot->writeFieldBegin(" <<
      "\"" << (*f_iter)->get_name() << "\", " <<
      type_to_enum((*f_iter)->get_type()) << ", " <<
      (*f_iter)->get_key() << ");" << endl;
    // Write field contents
    if (pointers && !(*f_iter)->get_type()->is_xception()) {
      generate_serialize_field(out, *f_iter, "oprot", "(*(this->", "))");
    } else {
      generate_serialize_field(out, *f_iter, "oprot", "this->");
    }
    // Write field closer
    indent(out) <<
      "xfer += oprot->writeFieldEnd();" << endl;
    if (check_if_set) {
      indent_down();
      indent(out) << '}';
    }
  }

  out << endl;

  // Write the struct map
  out <<
    indent() << "xfer += oprot->writeFieldStop();" << endl <<
    indent() << "xfer += oprot->writeStructEnd();" << endl <<
    indent() << "oprot->decrementRecursionDepth();" << endl <<
    indent() << "return xfer;" << endl;

  indent_down();
  indent(out) <<
    "}" << endl <<
    endl;
}

/**
 * Struct writer for result of a function, which can have only one of its
 * fields set and does a conditional if else look up into the __isset field
 * of the struct.
 *
 * @param out Output stream
 * @param tstruct The result struct
 */
void t_cpp_generator::generate_struct_result_writer(ofstream& out,
                                                    t_struct* tstruct,
                                                    bool pointers) {
  string name = tstruct->get_name();
  const vector<t_field*>& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;

  if (gen_templates_) {
    out <<
      indent() << "template <class Protocol_>" << endl <<
      indent() << "uint32_t " << tstruct->get_name() <<
      "::write(Protocol_* oprot) const {" << endl;
  } else {
    indent(out) <<
      "uint32_t " << tstruct->get_name() <<
      "::write(::pebble::dr::protocol::TProtocol* oprot) const {" << endl;
  }
  indent_up();

  out <<
    endl <<
    indent() << "uint32_t xfer = 0;" << endl <<
    endl;

  indent(out) <<
    "xfer += oprot->writeStructBegin(\"" << name << "\");" << endl;

  bool first = true;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
      out <<
        endl <<
        indent() << "if ";
    } else {
      out <<
        " else if ";
    }

    out << "(this->__isset." << (*f_iter)->get_name() << ") {" << endl;

    indent_up();

    // Write field header
    out <<
      indent() << "xfer += oprot->writeFieldBegin(" <<
      "\"" << (*f_iter)->get_name() << "\", " <<
      type_to_enum((*f_iter)->get_type()) << ", " <<
      (*f_iter)->get_key() << ");" << endl;
    // Write field contents
    if (pointers) {
      generate_serialize_field(out, *f_iter, "oprot", "(*(this->", "))");
    } else {
      generate_serialize_field(out, *f_iter, "oprot", "this->");
    }
    // Write field closer
    indent(out) << "xfer += oprot->writeFieldEnd();" << endl;

    indent_down();
    indent(out) << "}";
  }

  // Write the struct map
  out <<
    endl <<
    indent() << "xfer += oprot->writeFieldStop();" << endl <<
    indent() << "xfer += oprot->writeStructEnd();" << endl <<
    indent() << "return xfer;" << endl;

  indent_down();
  indent(out) <<
    "}" << endl <<
    endl;
}

/**
 * Generates the swap function.
 *
 * @param out Stream to write to
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_swap(ofstream& out, t_struct* tstruct) {
  out <<
    indent() << "void swap(" << tstruct->get_name() << " &a, " <<
    tstruct->get_name() << " &b) {" << endl;
  indent_up();

  // Let argument-dependent name lookup find the correct swap() function to
  // use based on the argument types.  If none is found in the arguments'
  // namespaces, fall back to ::std::swap().
  out <<
    indent() << "using ::std::swap;" << endl;

  bool has_nonrequired_fields = false;
  const vector<t_field*>& fields = tstruct->get_members();
  for (vector<t_field*>::const_iterator f_iter = fields.begin();
       f_iter != fields.end();
       ++f_iter) {
    t_field *tfield = *f_iter;

    if (tfield->get_req() != t_field::T_REQUIRED) {
      has_nonrequired_fields = true;
    }

    out <<
      indent() << "swap(a." << tfield->get_name() <<
      ", b." << tfield->get_name() << ");" << endl;
  }

  if (has_nonrequired_fields) {
    out <<
      indent() << "swap(a.__isset, b.__isset);" << endl;
  }

  // handle empty structs
  if (fields.size() == 0) {
    out <<
      indent() << "(void) a;" << endl;
    out <<
      indent() << "(void) b;" << endl;
  }

  scope_down(out);
  out << endl;
}

void t_cpp_generator::generate_struct_ostream_operator_decl(std::ofstream& out,
                                                            t_struct* tstruct) {
  out << "std::ostream& operator<<(std::ostream& out, const "
      << tstruct->get_name() << "& obj)";
}

namespace struct_ostream_operator_generator
{
void generate_required_field_value(std::ofstream& out, const t_field* field)
{
  if (field->get_type()->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)(field->get_type()))->get_base();
    if (t_base_type::TYPE_BYTE == tbase) {
      // 对byte类型特殊处理
      out << " << to_string((uint32_t)(obj." << field->get_name() << "))";
      return;
    }
  }

  out << " << to_string(obj." << field->get_name() << ")";
}

void generate_optional_field_value(std::ofstream& out, const t_field* field)
{
  out << ";" << endl;
  out << "(obj.__isset." << field->get_name() << " ? (out";
  generate_required_field_value(out, field);
  out << ") : (out << \"<null>\"))";
}

void generate_field_value(std::ofstream& out, const t_field* field)
{
  if (field->get_req() == t_field::T_OPTIONAL)
    generate_optional_field_value(out, field);
  else
    generate_required_field_value(out, field);
}

void generate_field_name(std::ofstream& out, const t_field* field)
{
  out << "\"" << field->get_name() << "=\"";
}

void generate_field(std::ofstream& out, const t_field* field)
{
  generate_field_name(out, field);
  generate_field_value(out, field);
}

void generate_fields(std::ofstream& out,
                     const vector<t_field*>& fields,
                     const std::string& indent)
{
  const vector<t_field*>::const_iterator beg = fields.begin();
  const vector<t_field*>::const_iterator end = fields.end();

  for (vector<t_field*>::const_iterator it = beg; it != end; ++it) {
    out << indent << "out << ";

    if (it != beg) {
      out << "\", \" << ";
    }

    generate_field(out, *it);
    out << ";" << endl;
  }
}


}

/**
 * Generates operator<<
 */
void t_cpp_generator::generate_struct_ostream_operator(std::ofstream& out,
                                                       t_struct* tstruct) {
  out << indent();
  generate_struct_ostream_operator_decl(out, tstruct);
  out << " {" << endl;

  indent_up();

  out <<
    indent() << "using pebble::dr::to_string;" << endl;

  // eliminate compiler unused warning
  const vector<t_field*>& fields = tstruct->get_members();
  if (fields.empty())
    out << indent() << "(void) obj;" << endl;

  out <<
    indent() << "out << \"" << tstruct->get_name() << "(\";" << endl;

  struct_ostream_operator_generator::generate_fields(out,
                                                     fields,
                                                     indent());

  out <<
    indent() << "out << \")\";" << endl <<
    indent() << "return out;" << endl;

  indent_down();
  out << "}" << endl << endl;
}

void t_cpp_generator::generate_struct_reflection_info(std::ofstream& out, t_struct* tstruct) {
  if (tstruct->is_xception() || tstruct->is_union()) return;
  // Get members
  vector<t_field*>::const_iterator m_iter;
  const vector<t_field*>& members = tstruct->get_members();

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      bool check_if_set = (*m_iter)->get_req() == t_field::T_OPTIONAL ||
          (*m_iter)->get_type()->is_xception();

      out << indent() << "int " << tstruct->get_name() << "::FieldInfo_" <<
          (*m_iter)->get_name() << "::Pack(void *obj, uint8_t *buff, uint32_t buff_len) const {" << endl;
      indent_up();
      out << indent() << "if (obj == NULL || buff == NULL) return pebble::dr::reflection::kINVALIDPARAMETER;" << endl;
      out << indent() << "try {" << endl;
      indent_up();
      out << indent() << "pebble::dr::internal::FieldPackGlobal::reset(buff, buff_len);" << endl;
      out << indent() << tstruct->get_name() << " *pstruct = static_cast<" << tstruct->get_name() << "*>(obj);" << endl;
      out << indent() << "if (pstruct == NULL) return pebble::dr::reflection::kINVALIDOBJECT;" << endl;
      out << indent() << "uint32_t xfer = 0;" << endl << endl;
      if (check_if_set) {
          out << endl << indent() << "if (pstruct->__isset." << (*m_iter)->get_name() << ") {" << endl;
          indent_up();
      }
      generate_serialize_field(out, *m_iter, "pebble::dr::internal::FieldPackGlobal::protocol.get()", "pstruct->");
      if (check_if_set) {
          indent_down();
          out << indent() << "}" << endl;
      }
      out << indent() << "return pebble::dr::internal::FieldPackGlobal::used();" << endl;
      indent_down();
      out << indent() << "} catch (pebble::dr::internal::ArrayOutOfBoundsException) {" << endl;
      out << indent(1) << "return pebble::dr::reflection::kINSUFFICIENTBUFFER;" << endl;
      out << indent() << "} catch (...) {" << endl;
      out << indent(1) << "return pebble::dr::reflection::kUNKNOW;" << endl;
      out << indent() << "}" << endl;
      indent_down();
      out << indent() << "};" << endl << endl;;

      out << indent() << "int " << tstruct->get_name() << "::FieldInfo_" <<
          (*m_iter)->get_name() << "::UnPack(void *obj, uint8_t *buff, uint32_t buff_len) const {" << endl;
      indent_up();
      out << indent() << "if (obj == NULL || buff == NULL) return pebble::dr::reflection::kINVALIDPARAMETER;" << endl;
      out << indent() << "try {" << endl;
      indent_up();
      out << indent() << "pebble::dr::internal::FieldPackGlobal::reset(buff, buff_len);" << endl;
      out << indent() << tstruct->get_name() << " *pstruct = static_cast<" << tstruct->get_name() << "*>(obj);" << endl;
      out << indent() << "if (pstruct == NULL) return pebble::dr::reflection::kINVALIDOBJECT;" << endl;
      out << indent() << "uint32_t xfer = 0;" << endl << endl;
      generate_deserialize_field(out, *m_iter, "pebble::dr::internal::FieldPackGlobal::protocol.get()", "pstruct->");
      if ((*m_iter)->get_req() != t_field::T_REQUIRED) {
          out << indent() << "pstruct->__isset." << (*m_iter)->get_name() << " = true;" << endl;
      }
      out << indent() << "return pebble::dr::internal::FieldPackGlobal::used();" << endl;
      indent_down();
      out << indent() << "} catch (pebble::dr::internal::ArrayOutOfBoundsException) {" << endl;
      out << indent(1) << "return pebble::dr::reflection::kINVALIDBUFFER;" << endl;
      out << indent() << "} catch (...) {" << endl;
      out << indent(1) << "return pebble::dr::reflection::kUNKNOW;" << endl;
      out << indent() << "}" << endl;
      indent_down();
      out << indent() << "};" << endl << endl;;
  }

  out << indent() << "const pebble::dr::reflection::TypeInfo* "<< tstruct->get_name() << "::GetTypeInfo() {" << endl;
  out << indent(1) << "return S_GetTypeInfo();" << endl;
  out << "}" << endl << endl;

  out << indent() << "const pebble::dr::reflection::TypeInfo* "<< tstruct->get_name() << "::S_GetTypeInfo() {" << endl;

  indent_up();

  out << indent() << "if (s_type_info == NULL) {" << endl;
  indent_up();
  out << indent() << "std::tr1::unordered_map<std::string, pebble::dr::reflection::FieldInfo *> field_infos;" << endl;
  out << indent() << "pebble::dr::reflection::AttributesType struct_attrs;" << endl << endl;

  typedef map<string, map<string, string> >::iterator AttributesIterator;
  AttributesIterator attr_iter;
  map<string, string>::iterator kv_iter;

  if (!tstruct->attributes_.empty()) {
    scope_up(out);
    for (attr_iter = tstruct->attributes_.begin(); attr_iter != tstruct->attributes_.end(); ++attr_iter) {
      if (attr_iter->second.empty()) {
        continue;
      }
      out << indent() << "std::map<std::string, std::string> "<< attr_iter->first << ";" << endl;
      for (kv_iter = attr_iter->second.begin(); kv_iter != attr_iter->second.end(); ++kv_iter) {
          out << indent() << attr_iter->first << "[\""<< kv_iter->first << "\"] = \"" << escape_string(kv_iter->second) << "\";" << endl;
      }
      out << indent() << "struct_attrs[\""<< attr_iter->first << "\"] = " << attr_iter->first << ";" << endl << endl;
    }
    scope_down(out);
  }

  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    scope_up(out);
    out << indent() << "pebble::dr::reflection::AttributesType field_attrs;" << endl << endl;
    for (attr_iter = (*m_iter)->attributes_.begin(); attr_iter != (*m_iter)->attributes_.end(); ++attr_iter) {
      if (attr_iter->second.empty()) {
        continue;
      }
      out << indent() << "std::map<std::string, std::string> "<< attr_iter->first << ";" << endl;
      for (kv_iter = attr_iter->second.begin(); kv_iter != attr_iter->second.end(); ++kv_iter) {
          out << indent() << attr_iter->first << "[\""<< kv_iter->first << "\"] = \"" << escape_string(kv_iter->second) << "\";" << endl;
      }
      out << indent() << "field_attrs[\""<< attr_iter->first << "\"] = " << attr_iter->first << ";" << endl << endl;
    }
    out << indent() << "field_infos[\"" << (*m_iter)->get_name() <<  "\"] = new FieldInfo_"
        << (*m_iter)->get_name() << "(" << endl;
    out << indent(1) << "\"" << (*m_iter)->get_name() << "\", " << (*m_iter)->get_key()
        << ", " << type_to_enum((*m_iter)->get_type()) << ", "<< ((*m_iter)->get_req() == t_field::T_OPTIONAL ? "true" : "false") << ", field_attrs);" << endl;
    scope_down(out);
  }

  out << indent() << "s_type_info = new pebble::dr::reflection::TypeInfo(field_infos, struct_attrs);" << endl;
  indent_down();
  out << indent() << "}" << endl;

  out << indent() << "return s_type_info;" << endl;
  indent_down();
  out << "}" << endl << endl;

  out << "pebble::dr::reflection::TypeInfo* " << tstruct->get_name() << "::s_type_info = NULL;" << endl << endl;
}


/**
 * Generates a thrift service. In C++, this comprises an entirely separate
 * header and source file. The header file defines the methods and includes
 * the data types defined in the main header file, and the implementation
 * file contains implementations of the basic printer and default interfaces.
 *
 * @param tservice The service definition
 */
void t_cpp_generator::generate_service(t_service* tservice) {
  string svcname = tservice->get_name();

  // Make output files
  string f_header_h_name = get_out_dir()+program_name_+"_"+svcname+".h";
  string f_header_inh_name = get_out_dir()+program_name_+"_"+svcname+".inh";
  f_service_h_.open(f_header_h_name.c_str());
  f_service_inh_.open(f_header_inh_name.c_str());

  // Print header file includes
  f_service_h_ <<
    autogen_comment();
  f_service_h_ <<
    "#ifndef __" << program_name_ << "_" << svcname << "_h__" << endl <<
    "#define __" << program_name_ << "_" << svcname << "_h__" << endl <<
    endl;

  f_service_h_ <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    "_" << svcname << ".inh\"" <<
    endl;

  f_service_h_ <<
    ns_open_ << endl <<
    endl;

  // Print inheader file includes
  f_service_inh_ <<
    autogen_comment();
  f_service_inh_ <<
    "#ifndef __" << program_name_ << "_" << svcname << "_inh__" << endl <<
    "#define __" << program_name_ << "_" << svcname << "_inh__" << endl <<
    endl;

  f_service_inh_ << "#include \"framework/pebble_rpc.h\"" << endl;

  f_service_inh_ <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    ".h\"" << endl;

  t_service* extends_service = tservice->get_extends();
  if (extends_service != NULL) {
    f_service_inh_ <<
      "#include \"" << get_include_prefix(*(extends_service->get_program())) <<
      extends_service->get_program()->get_name() << "_" <<
      extends_service->get_name() << ".h\"" << endl;
  }

  f_service_inh_ <<
    ns_open_ << endl <<
    endl;

  // Service implementation file includes
  string f_service_name = get_out_dir()+program_name_+"_"+svcname+".cpp";
  f_service_cpp_.open(f_service_name.c_str());
  f_service_cpp_ <<
    autogen_comment();

  // The swap() code needs <algorithm> for std::swap()
  f_service_cpp_ << "#include <algorithm>" << endl;
  // for operator<<
  f_service_cpp_ << "#include <ostream>" << endl << endl;
  f_service_cpp_ << "#include \"framework/dr/common/to_string.h\"" << endl;
  f_service_cpp_ << "#include \"framework/dr/protocol/rapidjson_protocol.h\"" << endl;
  f_service_cpp_ << "#include \"framework/dr/serialize.h\"" << endl;
  f_service_cpp_ << "#include \"rapidjson/memorystream.h\"" << endl;
  f_service_cpp_ << "#include \"rapidjson/prettywriter.h\"" << endl;
  f_service_cpp_ << "#include \"rapidjson/reader.h\"" << endl <<
    endl;

  f_service_cpp_ << "#include \"framework/dr/common/dr_define.h\"" << endl;
  f_service_cpp_ << "#include \"framework/dr/transport/buffer_transport.h\"" << endl;

  f_service_cpp_ <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ << "_" << svcname << ".h\"" << endl << endl;

  f_service_cpp_ <<
    endl << ns_open_ << endl << endl;

  // Generate all the components
  generate_service_interface(tservice, "");
  generate_service_helpers(tservice);
  generate_service_client(tservice, "");

  generate_service_interface(tservice, "CobSv");
  generate_service_processor(tservice, "Cob");

  // Close the namespace
  f_service_cpp_ <<
    ns_close_ << endl <<
    endl;
  f_service_h_ <<
    ns_close_ << endl <<
    endl;
  f_service_inh_ <<
    ns_close_ << endl <<
    endl;

  f_service_h_ << endl << "// 内部使用，用户无需关注" << endl
    << "namespace pebble {" << endl;
  f_service_h_ << indent() << "class PebbleRpc;" << endl;
  f_service_h_ << indent() << "template<typename Class> class GenServiceHandler;" << endl << endl;

  string ns = namespace_prefix(tservice->get_program()->get_namespace("cpp"));

  f_service_h_
      << indent() << "template<>" << endl
      << indent() << "class GenServiceHandler<" << ns << service_name_ << "CobSvIf> {" << endl
      << indent() << "public:" << endl
      << indent(1) << "cxx::shared_ptr<pebble::IPebbleRpcService> operator() (pebble::PebbleRpc* rpc," << ns << service_name_ << "CobSvIf *iface) {" << endl
      << indent(2) << "cxx::shared_ptr<pebble::IPebbleRpcService> service_handler(" << endl
      << indent(3) << "new " << ns << service_name_ << "Handler(rpc, iface));" << endl
      << indent(2) << "return service_handler;" << endl
      << indent(1) << "}" << endl
      << indent() << "};" << endl;

  f_service_h_ <<
    "} // namespace pebble" << endl << endl;

  f_service_h_ <<
    "#endif // __" << program_name_ << "_" << svcname << "_h__" << endl;

  f_service_inh_ <<
    "#endif // __" << program_name_ << "_" << svcname << "_inh__" << endl;

  // Close the files
  f_service_cpp_.close();
  f_service_h_.close();
  f_service_inh_.close();
}

/**
 * Generates helper functions for a service. Basically, this generates types
 * for all the arguments and results to functions.
 *
 * @param tservice The service to generate a header definition for
 */
void t_cpp_generator::generate_service_helpers(t_service* tservice) {
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  // std::ofstream& out = (gen_templates_ ? f_service_tcc_ : f_service_cpp_);
  std::ofstream& out = f_service_cpp_;

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* ts = (*f_iter)->get_arglist();
    string name_orig = ts->get_name();

    // TODO(dreiss): Why is this stuff not in generate_function_helpers?
    ts->set_name(tservice->get_name() + "_" + (*f_iter)->get_name() + "_args");
    generate_struct_declaration(f_service_inh_, ts, false);
    generate_struct_definition(out, out, ts, false);
    generate_struct_reader(out, ts);
    generate_struct_writer(out, ts);
    ts->set_name(tservice->get_name() + "_" + (*f_iter)->get_name() + "_pargs");
    generate_struct_declaration(f_service_inh_, ts, false, true, false, true);
    generate_struct_definition(out, out, ts, false);
    generate_struct_writer(out, ts, true);
    ts->set_name(name_orig);

    generate_function_helpers(tservice, *f_iter);
  }
}

/**
 * Generates a service interface definition.
 *
 * @param tservice The service to generate a header definition for
 */
void t_cpp_generator::generate_service_interface(t_service* tservice, string style) {

  std::ofstream *out = &f_service_inh_;
  if ("CobSv" == style) {
    out = &f_service_h_;
  }

  string service_if_name = service_name_ + style + "If";

  string extends = "";
  if (tservice->get_extends() != NULL) {
    extends = " : virtual public " + type_name(tservice->get_extends()) +
      style + "If";
  }

  *out <<
    "class " << service_if_name << extends << " {" << endl <<
    "public:" << endl;
  indent_up();
  *out <<
    indent() << "virtual ~" << service_if_name << "() {}" << endl << endl;

  if ("CobSv" == style) {
      *out << indent() << "// IDL定义接口部分 - begin" << endl;
      *out << indent() << "// IDL定义接口说明 : " << endl <<
        indent() << "// 1. rsp为响应回调函数(ONEWAY接口没有rsp)，调用此函数将向RPC调用方发送响应消息" << endl <<
        indent() << "// 2. rsp第一个参数为业务处理的返回码，默认0为成功，第二个参数(如果有的话)为响应数据" << endl;
  }

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    if ((*f_iter)->has_doc()) *out << endl;
    generate_java_doc(*out, *f_iter);
    *out <<
      indent() << "virtual " << function_signature_if(*f_iter, style) << " = 0;" << endl;
    // 同步异步接口统一到一起
    if (style == "" && !((*f_iter)->is_oneway())) {
      *out <<
        indent() << "virtual " << function_signature_if(*f_iter, "CobCl") << " = 0;" << endl;
    }
  }

  // add get processor function
  if ("CobSv" == style) {
    *out << indent() << "// IDL定义接口部分 - end" << endl << endl;
    *out << indent() << "typedef " << service_name_ << "CobSvIf __InterfaceType;" << endl;
  }

  indent_down();
  *out <<
    "};" << endl << endl;
}

/**
 * Generates a service interface factory.
 *
 * @param tservice The service to generate an interface factory for.
 */
void t_cpp_generator::generate_service_interface_factory(t_service* tservice,
                                                         string style) {
  string service_if_name = service_name_ + style + "If";

  // Figure out the name of the upper-most parent class.
  // Getting everything to work out properly with inheritance is annoying.
  // Here's what we're doing for now:
  //
  // - All handlers implement getHandler(), but subclasses use covariant return
  //   types to return their specific service interface class type.  We have to
  //   use raw pointers because of this; shared_ptr<> can't be used for
  //   covariant return types.
  //
  // - Since we're not using shared_ptr<>, we also provide a releaseHandler()
  //   function that must be called to release a pointer to a handler obtained
  //   via getHandler().
  //
  //   releaseHandler() always accepts a pointer to the upper-most parent class
  //   type.  This is necessary since the parent versions of releaseHandler()
  //   may accept any of the parent types, not just the most specific subclass
  //   type.  Implementations can use dynamic_cast to cast the pointer to the
  //   subclass type if desired.
  t_service* base_service = tservice;
  while (base_service->get_extends() != NULL) {
    base_service = base_service->get_extends();
  }
  string base_if_name = type_name(base_service) + style + "If";

  // Generate the abstract factory class
  string factory_name = service_if_name + "Factory";
  string extends;
  if (tservice->get_extends() != NULL) {
    extends = " : virtual public " + type_name(tservice->get_extends()) +
      style + "IfFactory";
  }

  f_service_h_ <<
    "class " << factory_name << extends << " {" << endl <<
    "public:" << endl;
  indent_up();
  f_service_h_ <<
    indent() << "typedef " << service_if_name << " Handler;" << endl <<
    endl <<
    indent() << "virtual ~" << factory_name << "() {}" << endl <<
    endl <<
    indent() << "virtual " << service_if_name << "* getHandler(" <<
      "const ::pebble::rpc::processor::TConnectionInfo& connInfo) = 0;" <<
    endl <<
    indent() << "virtual void releaseHandler(" << base_if_name <<
    "* /* handler */) = 0;" << endl;

  indent_down();
  f_service_h_ <<
    "};" << endl << endl;

  // Generate the singleton factory class
  string singleton_factory_name = service_if_name + "SingletonFactory";
  f_service_h_ <<
    "class " << singleton_factory_name <<
    " : virtual public " << factory_name << " {" << endl <<
    "public:" << endl;
  indent_up();
  f_service_h_ <<
    indent() << singleton_factory_name << "(const cxx::shared_ptr<" <<
    service_if_name << ">& iface) : iface_(iface) {}" << endl <<
    indent() << "virtual ~" << singleton_factory_name << "() {}" << endl <<
    endl <<
    indent() << "virtual " << service_if_name << "* getHandler(" <<
      "const ::pebble::rpc::processor::TConnectionInfo&) {" << endl <<
    indent(1) << "return iface_.get();" << endl <<
    indent() << "}" << endl <<
    indent() << "virtual void releaseHandler(" << base_if_name <<
    "* /* handler */) {}" << endl;

  f_service_h_ <<
    endl <<
    "protected:" << endl <<
    indent() << "cxx::shared_ptr<" << service_if_name << "> iface_;" << endl;

  indent_down();
  f_service_h_ <<
    "};" << endl << endl;
}

/**
 * Generates a null implementation of the service.
 *
 * @param tservice The service to generate a header definition for
 */
void t_cpp_generator::generate_service_null(t_service* tservice, string style) {
  string extends = "";
  if (tservice->get_extends() != NULL) {
    extends = " , virtual public " + type_name(tservice->get_extends()) + style + "Null";
  }
  f_service_h_ <<
    "class " << service_name_ << style << "Null : virtual public " << service_name_ << style << "If" << extends << " {" << endl <<
    "public:" << endl;
  indent_up();
  f_service_h_ <<
    indent() << "virtual ~" << service_name_ << style << "Null() {}" << endl;
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_service_h_ <<
      indent() << function_signature(*f_iter, style, "", false) << " {" << endl;
    indent_up();

    t_type* returntype = (*f_iter)->get_returntype();
    t_field returnfield(returntype, "response");

    if (style == "") {
      if (returntype->is_void() || is_complex_type(returntype)) {
        f_service_h_ << indent() << "return;" << endl;
      } else {
        f_service_h_ <<
          indent() << declare_field(&returnfield, true) << endl <<
          indent() << "return response;" << endl;
      }
    } else if (style == "CobSv") {
      if (returntype->is_void()) {
        f_service_h_ << indent() << "return cb();" << endl;
    } else {
      t_field returnfield(returntype, "response");
      f_service_h_ <<
        indent() << declare_field(&returnfield, true) << endl <<
        indent() << "return cb(response);" << endl;
    }

    } else {
      throw "UNKNOWN STYLE";
    }

    indent_down();
    f_service_h_ <<
      indent() << "}" << endl;

    // 增加异步接口定义
    if (style == "" && !((*f_iter)->is_oneway())) {
      f_service_h_ <<
        indent() << "virtual " << function_signature(*f_iter, "CobCl", "", false) << " {" << endl;
      indent_up();

      f_service_h_ << indent() << "return;" << endl;

      indent_down();
      f_service_h_ <<
        indent() << "}" << endl;
    }
  }
  indent_down();
  f_service_h_ <<
    "};" << endl << endl;
}

void t_cpp_generator::generate_function_call(ostream& out, t_function* tfunction, string target, string iface, string arg_prefix) {
  bool first = true;
  t_type* ret_type = get_true_type(tfunction->get_returntype());
  out << indent();
  if (!tfunction->is_oneway() && !ret_type->is_void()) {
    if (is_complex_type(ret_type)) {
      first = false;
      out << iface << "->" << tfunction->get_name() << "(" << target;
    } else {
      out << target << " = " << iface << "->" << tfunction->get_name() << "(";
    }
  } else {
    out << iface << "->" << tfunction->get_name() << "(";
  }
  const std::vector<t_field*>& fields = tfunction->get_arglist()->get_members();
  vector<t_field*>::const_iterator f_iter;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      out << ", ";
    }
    out << arg_prefix << (*f_iter)->get_name();
  }
  out << ");" << endl;
}

void t_cpp_generator::generate_service_async_skeleton(t_service* tservice) {
  string svcname = tservice->get_name();

  // Service implementation file includes
  string f_skeleton_name = get_out_dir()+svcname+"_async_server.skeleton.cpp";

  string ns = namespace_prefix(tservice->get_program()->get_namespace("cpp"));

  ofstream f_skeleton;
  f_skeleton.open(f_skeleton_name.c_str());
  f_skeleton <<
    "// This autogenerated skeleton file illustrates one way to adapt a synchronous" << endl <<
    "// interface into an asynchronous interface. You should copy it to another" << endl <<
    "// filename to avoid overwriting it and rewrite as asynchronous any functions" << endl <<
    "// that would otherwise introduce unwanted latency." << endl <<
    endl <<
    "#include \"" << get_include_prefix(*get_program()) << svcname << ".h\"" << endl <<
    "#include <source/rpc/protocol/binary_protocol.h>" << endl <<
    endl <<
    "using namespace ::pebble::rpc;" << endl <<
    "using namespace ::pebble::dr::protocol;" << endl <<
    "using namespace ::pebble::dr::transport;" << endl <<
    "using namespace ::pebble::rpc::async;" << endl <<
    endl <<
    "using cxx::shared_ptr;" << endl <<
    endl;

  // the following code would not compile:
  // using namespace ;
  // using namespace ::;
  if ( (!ns.empty()) && (ns.compare(" ::") != 0)) {
    f_skeleton <<
      "using namespace " << string(ns, 0, ns.size()-2) << ";" << endl <<
      endl;
  }

  f_skeleton <<
    "class " << svcname << "AsyncHandler : " <<
    "public " << svcname << "CobSvIf {" << endl <<
    "public:" << endl;
  indent_up();
  f_skeleton <<
    indent() << svcname << "AsyncHandler() {" << endl <<
    indent(1) << "syncHandler_ = std::auto_ptr<" << svcname <<
                "Handler>(new " << svcname << "Handler);" << endl <<
    indent(1) << "// Your initialization goes here" << endl <<
    indent() << "}" << endl;
  f_skeleton <<
    indent() << "virtual ~" << service_name_ << "AsyncHandler();" << endl;

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_skeleton <<
      endl <<
      indent() << function_signature(*f_iter, "CobSv", "", true) << " {" << endl;
    indent_up();

    t_type* returntype = (*f_iter)->get_returntype();
    t_field returnfield(returntype, "response");

    string target = returntype->is_void() ? "" : "response";
    if (!returntype->is_void()) {
      f_skeleton <<
        indent() << declare_field(&returnfield, true) << endl;
    }
    generate_function_call(f_skeleton, *f_iter, target, "syncHandler_", "");
    f_skeleton << indent() << "return cb(" << target << ");" << endl;

    scope_down(f_skeleton);
  }
  f_skeleton << endl <<
    "protected:" << endl <<
    indent() << "std::auto_ptr<" << svcname << "Handler> syncHandler_;" << endl;
  indent_down();
  f_skeleton <<
    "};" << endl << endl;
}

/**
 * Generates a multiface, which is a single server that just takes a set
 * of objects implementing the interface and calls them all, returning the
 * value of the last one to be called.
 *
 * @param tservice The service to generate a multiserver for.
 */
void t_cpp_generator::generate_service_multiface(t_service* tservice) {
  // Generate the dispatch methods
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;

  string extends = "";
  string extends_multiface = "";
  if (tservice->get_extends() != NULL) {
    extends = type_name(tservice->get_extends());
    extends_multiface = ", public " + extends + "Multiface";
  }

  string list_type = string("std::vector<cxx::shared_ptr<") + service_name_ + "If> >";

  // Generate the header portion
  f_service_h_ <<
    "class " << service_name_ << "Multiface : " <<
    "virtual public " << service_name_ << "If" <<
    extends_multiface << " {" << endl <<
    "public:" << endl;
  indent_up();
  f_service_h_ <<
    indent() << service_name_ << "Multiface(" << list_type << "& ifaces) : ifaces_(ifaces) {" << endl;
  if (!extends.empty()) {
    f_service_h_ <<
      indent(1) << "std::vector<cxx::shared_ptr<" + service_name_ + "If> >::iterator iter;" << endl <<
      indent(1) << "for (iter = ifaces.begin(); iter != ifaces.end(); ++iter) {" << endl <<
      indent(2) << "" << extends << "Multiface::add(*iter);" << endl <<
      indent(1) << "}" << endl;
  }
  f_service_h_ <<
    indent() << "}" << endl <<
    indent() << "virtual ~" << service_name_ << "Multiface() {}" << endl;
  indent_down();

  // Protected data members
  f_service_h_ <<
    "protected:" << endl;
  indent_up();
  f_service_h_ <<
    indent() << list_type << " ifaces_;" << endl <<
    indent() << service_name_ << "Multiface() {}" << endl <<
    indent() << "void add(cxx::shared_ptr<" << service_name_ << "If> iface) {" << endl;
  if (!extends.empty()) {
    f_service_h_ <<
      indent(1) << extends << "Multiface::add(iface);" << endl;
  }
  f_service_h_ <<
    indent(1) << "ifaces_.push_back(iface);" << endl <<
    indent() << "}" << endl;
  indent_down();

  f_service_h_ <<
    indent() << "public:" << endl;
  indent_up();

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* arglist = (*f_iter)->get_arglist();
    const vector<t_field*>& args = arglist->get_members();
    vector<t_field*>::const_iterator a_iter;

    string call = string("ifaces_[i]->") + (*f_iter)->get_name() + "(";
    bool first = true;
    if (is_complex_type((*f_iter)->get_returntype())) {
      call += "response";
      first = false;
    }
    for (a_iter = args.begin(); a_iter != args.end(); ++a_iter) {
      if (first) {
        first = false;
      } else {
        call += ", ";
      }
      call += (*a_iter)->get_name();
    }
    call += ")";

    f_service_h_ <<
      indent() << function_signature(*f_iter, "") << " {" << endl;
    indent_up();
    f_service_h_ <<
      indent() << "size_t sz = ifaces_.size();" << endl <<
      indent() << "size_t i = 0;" << endl <<
      indent() << "for (; i < (sz - 1); ++i) {" << endl;
    indent_up();
    f_service_h_ <<
      indent() << call << ";" << endl;
    indent_down();
    f_service_h_ <<
      indent() << "}" << endl;

    if (!(*f_iter)->get_returntype()->is_void()) {
      if (is_complex_type((*f_iter)->get_returntype())) {
        f_service_h_ <<
          indent() << call << ";" << endl <<
          indent() << "return;" << endl;
      } else {
        f_service_h_ <<
          indent() << "return " << call << ";" << endl;
      }
    } else {
      f_service_h_ <<
        indent() << call << ";" << endl;
    }

    indent_down();
    f_service_h_ <<
      indent() << "}" << endl <<
      endl;
  }

  indent_down();
  f_service_h_ <<
    indent() << "};" << endl <<
    endl;
}

/**
 * Generates a service client definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_cpp_generator::generate_service_client(t_service* tservice, string style) {
  std::ofstream& out = f_service_cpp_;
  string if_suffix = "If";

  // Generate the header portion
  string client_suffix = "Client";
  string extends_client = "";
  if (tservice->get_extends() != NULL) {
    extends_client = ", public " + type_name(tservice->get_extends()) + client_suffix;
  }

  f_service_h_ <<
    "class " << service_name_ << client_suffix << " : " <<
    "virtual public " << service_name_ << if_suffix <<
    extends_client << " {" << endl <<
    "public:" << endl;

  indent_up();

  f_service_h_ <<
    indent() << service_name_ << client_suffix << "(pebble::PebbleRpc* rpc);" << endl <<
    indent() << "virtual ~" << service_name_ << client_suffix << "();" << endl;

  if (tservice->get_extends() == NULL) {
    f_service_h_ <<
      indent() << "// 设置连接句柄，RPC请求通过此句柄对应的连接发送" << endl <<
      indent() << "void SetHandle(int64_t handle);" << endl << endl <<
      indent() << "// 设置路由函数，连接的选择交给路由回调函数处理，RPC请求通过路由回调函数选择的连接发送" << endl <<
      indent() << "// 设置路由回调函数后，不再使用SetHandle设置的连接句柄" << endl <<
      indent() << "void SetRouteFunction(const cxx::function<int64_t(uint64_t key)>& route_callback);" << endl << endl <<
      indent() << "// 设置路由key，如使用取模或哈希路由策略时使用" << endl <<
      indent() << "void SetRouteKey(uint64_t route_key);" << endl << endl <<
      indent() << "// 设置广播的频道名字，设置了频道后Client将所有的RPC请求按广播处理，广播至channel_name" << endl <<
      indent() << "void SetBroadcast(const std::string& channel_name);" << endl <<
      endl;
  }

  f_service_h_ << indent() << "// IDL定义接口部分 - begin" << endl;
  f_service_h_ << indent() << "// IDL定义接口说明 : " << endl <<
    indent() << "// 1. 所有IDL定义的接口将生成同步和异步两个函数，函数的参数和IDL定义保持一致" << endl <<
    indent() << "// 2. 同步调用函数有返回值，返回值表示RPC调用的结果，当RPC调用成功时RPC响应的结果值通过最后一个输出参数返回" << endl <<
    indent() << "// 3. 异步调用函数最后一个参数为callback函数，callback的第一个参数表示RPC调用的结果，第二个参数(如果有的话)为RPC响应结果" << endl;
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::const_iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    indent(f_service_h_) << function_signature_if(*f_iter, "") << ";" << endl;
    if (!((*f_iter)->is_oneway())) {
      indent(f_service_h_) << function_signature_if(*f_iter, "Parallel", "Parallel") << ";" << endl;
      indent(f_service_h_) << function_signature_if(*f_iter, "CobCl") << ";" << endl;
    }
  }

  f_service_h_ << indent() << "// IDL定义接口部分 - end" << endl;

  // 响应接收处理函数 recv_functionname & recv_functionname_sync
  f_service_h_ << endl << "private:" << endl;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    if (!(*f_iter)->is_oneway()) {
      // 参数定义
      std::string args = "int32_t ret, const uint8_t* buff, uint32_t buff_len";
      std::string ret_sync;
      std::string ret_parallel = ", int32_t* ret_code";
      std::string ret_async(", cxx::function<void(int ret_code)>& cb");
      if (!(*f_iter)->get_returntype()->is_void()) {
        std::string type_ref;
        std::string type_const;
        if (is_complex_type((*f_iter)->get_returntype())) {
            type_ref   = "&";
            type_const = "const ";
        }
        ret_sync  = ", " + type_name((*f_iter)->get_returntype()) + "* response";
        ret_parallel = ", int32_t* ret_code, " + type_name((*f_iter)->get_returntype()) + "* response";
        ret_async = ", cxx::function<void(int ret_code, " + type_const + type_name((*f_iter)->get_returntype())
            + type_ref + " response)>" + "& cb";
      }

      // sync
      f_service_h_ << indent() <<
        "int32_t recv_" << (*f_iter)->get_name() << "_sync(" << args << ret_sync << ");" <<
        endl;

      // parallel
      f_service_h_ << indent() <<
        "int32_t recv_" << (*f_iter)->get_name() << "_parallel(" << args << ret_parallel << ");" <<
        endl;

      // async
      f_service_h_ << indent() <<
        "int32_t recv_" << (*f_iter)->get_name() << "(" << args << ret_async << ");" <<
        endl;
    }
  }
  indent_down();

  if (tservice->get_extends() == NULL) {
    f_service_h_ << endl << "protected:" << endl;
    f_service_h_ << indent(1) << "int64_t GetHandle();" << endl;
    f_service_h_ << endl << "protected:" << endl;
    f_service_h_ << indent(1) << "::pebble::PebbleRpc* m_client;" << endl;
    f_service_h_ << indent(1) << "int64_t m_handle;" << endl;
    f_service_h_ << indent(1) << "cxx::function<int64_t(uint64_t)> m_route_func;" << endl;
    f_service_h_ << indent(1) << "uint64_t m_route_key;" << endl;
    f_service_h_ << indent(1) << "std::string m_channel_name;" << endl;
  }

  f_service_h_ <<
    "};" << endl <<
    endl;

  string scope = service_name_ + client_suffix + "::";

  // Generate client method implementations
  out <<
    scope << service_name_ << client_suffix << "(pebble::PebbleRpc* rpc)";

  if (tservice->get_extends() == NULL) {
    out <<  " {" << endl;
    out << indent(1) << "m_client = rpc;" << endl;
    out << indent(1) << "m_route_key = 0;" << endl;
    out << indent() << "}" << endl;
  } else {
    out <<  ":" << endl;
    out <<
      indent(1) << type_name(tservice->get_extends()) << client_suffix <<
      "(rpc) {" << endl <<
      "}" << endl;
  }
  out << endl;


  out <<
    scope << "~" << service_name_ << client_suffix << "() {}" << endl <<
    endl;

  if (tservice->get_extends() == NULL) {
    out << "void " << scope << "SetHandle(int64_t handle) {" << endl <<
      indent(1) << "m_handle = handle;" << endl <<
      "}" << endl <<
      endl;

    out << "void " << scope << "SetRouteFunction(const cxx::function<int64_t(uint64_t key)>& route_func) {" << endl <<
      indent(1) << "m_route_func = route_func;" << endl <<
      "}" << endl <<
      endl;

    out << "void " << scope << "SetRouteKey(uint64_t route_key) {" << endl <<
      indent(1) << "m_route_key = route_key;" << endl <<
      "}" << endl <<
      endl;

    out << "void " << scope << "SetBroadcast(const std::string& channel_name) {" << endl <<
      indent(1) << "m_channel_name = channel_name;" << endl <<
      "}" << endl <<
      endl;

    out << "int64_t " << scope << "GetHandle() {" << endl <<
      indent(1) << "if (m_route_func) {" << endl <<
      indent(2) << "return m_route_func(m_route_key);" << endl <<
      indent(1) << "}" << endl <<
      endl <<
      indent(1) << "return m_handle;" << endl <<
      "}" << endl <<
      endl;
  }

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    string funname = (*f_iter)->get_name();

    // 同步接口实现
    indent(out) << function_signature_if(*f_iter, "", scope) << endl;
    scope_up(out);

    out << indent() << "::pebble::dr::protocol::TProtocol* encoder =" << endl << indent(1) <<
        "m_client->GetCodec(pebble::PebbleRpc::kMALLOC);" << endl << indent() <<
        "if (!encoder) {" << endl << indent(1) <<
        "return ::pebble::kPEBBLE_RPC_UNKNOWN_CODEC_TYPE;" << endl << indent() <<
        "}" << endl <<
        endl;


    out << indent() <<
        "::pebble::RpcHead head;" << endl << indent() <<
        "head.m_function_name.assign(\"" << service_name_ << ":" << funname << "\");" << endl << indent();
    if (!(*f_iter)->is_oneway()) {
        out << "head.m_message_type = pebble::dr::protocol::T_CALL;" << endl << indent();
    } else {
        out << "head.m_message_type = pebble::dr::protocol::T_ONEWAY;" << endl << indent();
    }
    out << "head.m_session_id = m_client->GenSessionId();" << endl << endl;

    const vector<t_field*>& fields = (*f_iter)->get_arglist()->get_members();
    vector<t_field*>::const_iterator fld_iter;

    string argsname = tservice->get_name() + "_" + (*f_iter)->get_name() + "_pargs";
    out << indent() << argsname << " args;" << endl;
    for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
      out <<
        indent() << "args." << (*fld_iter)->get_name() << " = &" << (*fld_iter)->get_name() << ";" << endl;
    }
    out << endl;

    out << indent() <<
      "try {" << endl << indent(1) <<
      "args.write(encoder);" << endl << indent(1) <<
      "encoder->writeMessageEnd();" << endl << indent(1) <<
      "encoder->getTransport()->writeEnd();" << endl << indent() <<
      "} catch (pebble::TException ex) {" << endl << indent(1) <<
      "return pebble::kRPC_ENCODE_FAILED;" << endl << indent() <<
      "}" << endl <<
      endl;

    out << indent() <<
      "uint8_t* buff = NULL;" << endl << indent() <<
      "uint32_t buff_len = 0;" << endl << indent() <<
      "(static_cast<pebble::dr::transport::TMemoryBuffer*>(encoder->getTransport().get()))->" << endl << indent(1) <<
      "getBuffer(&buff, &buff_len);" << endl <<
      endl;

    if (!(*f_iter)->is_oneway()) {
      std::string ret_sync;
      if (!(*f_iter)->get_returntype()->is_void()) {
        ret_sync = ", response";
      }
      out << indent() <<
        "if (m_channel_name.empty()) {" <<
        endl;
      out << indent(1) <<
        "pebble::OnRpcResponse on_rsp = cxx::bind(&" << scope << "recv_" << funname << "_sync, this," << endl << indent(2) <<
        "cxx::placeholders::_1, cxx::placeholders::_2, cxx::placeholders::_3" << ret_sync << ");" << endl << indent(1) <<
        "return m_client->SendRequestSync(GetHandle(), head, buff, buff_len, on_rsp, " << (*f_iter)->get_timeout_ms() << ");" <<
        endl;
      out << indent() << "} else {" << endl << indent(1) <<
        "return m_client->BroadcastRequest(m_channel_name, head, buff, buff_len);" << endl << indent() <<
        "}" <<
        endl;
    } else {
      out << indent() <<
        "if (m_channel_name.empty()) {" <<
        endl;
      out << indent(1) <<
        "pebble::OnRpcResponse on_rsp;" << endl << indent(1) <<
        "int32_t ret = m_client->SendRequest(GetHandle(), head, buff, buff_len, on_rsp, " << (*f_iter)->get_timeout_ms() << ");" << endl << indent(1) <<
        "if (ret != pebble::kRPC_SUCCESS) {" << endl << indent(2) <<
        "return ret;" << endl << indent(1) <<
        "}" << endl <<
        endl;

      out << indent(1) <<
        "return pebble::kRPC_SUCCESS;" <<
        endl;

      out << indent() << "} else {" << endl << indent(1) <<
        "return m_client->BroadcastRequest(m_channel_name, head, buff, buff_len);" << endl << indent() <<
        "}" <<
        endl;
    }

    scope_down(out);
    out <<
      endl;

    // parallel function
    if (!(*f_iter)->is_oneway()) {
      indent(out) << function_signature_if(*f_iter, "Parallel", scope + "Parallel") << endl;
      scope_up(out);

      out << indent() << "::pebble::dr::protocol::TProtocol* encoder =" << endl << indent(1) <<
          "m_client->GetCodec(pebble::PebbleRpc::kMALLOC);" << endl << indent() <<
          "if (!encoder) {" << endl << indent(1) <<
          "*ret_code = ::pebble::kPEBBLE_RPC_UNKNOWN_CODEC_TYPE;" << endl << indent(1) <<
          "return *ret_code;" << endl << indent() <<
          "}" << endl <<
          endl;


      out << indent() <<
          "::pebble::RpcHead head;" << endl << indent() <<
          "head.m_function_name.assign(\"" << service_name_ << ":" << funname << "\");" << endl << indent();
      if (!(*f_iter)->is_oneway()) {
          out << "head.m_message_type = pebble::dr::protocol::T_CALL;" << endl << indent();
      } else {
          out << "head.m_message_type = pebble::dr::protocol::T_ONEWAY;" << endl << indent();
      }
      out << "head.m_session_id = m_client->GenSessionId();" << endl << endl;

      const vector<t_field*>& fields = (*f_iter)->get_arglist()->get_members();
      vector<t_field*>::const_iterator fld_iter;

      string argsname = tservice->get_name() + "_" + (*f_iter)->get_name() + "_pargs";
      out << indent() << argsname << " args;" << endl;
      for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
        out <<
          indent() << "args." << (*fld_iter)->get_name() << " = &" << (*fld_iter)->get_name() << ";" << endl;
      }
      out << endl;

      out << indent() <<
        "try {" << endl << indent(1) <<
        "args.write(encoder);" << endl << indent(1) <<
        "encoder->writeMessageEnd();" << endl << indent(1) <<
        "encoder->getTransport()->writeEnd();" << endl << indent() <<
        "} catch (pebble::TException ex) {" << endl << indent(1) <<
        "*ret_code = pebble::kRPC_ENCODE_FAILED;" << endl << indent(1) <<
        "return *ret_code;" << endl << indent() <<
        "}" << endl <<
        endl;

      out << indent() <<
        "uint8_t* buff = NULL;" << endl << indent() <<
        "uint32_t buff_len = 0;" << endl << indent() <<
        "(static_cast<pebble::dr::transport::TMemoryBuffer*>(encoder->getTransport().get()))->" << endl << indent(1) <<
        "getBuffer(&buff, &buff_len);" << endl <<
        endl;

      if (!(*f_iter)->is_oneway()) {
        std::string ret_sync;
        if (!(*f_iter)->get_returntype()->is_void()) {
          ret_sync = ", response";
        }
        out << indent() <<
          "if (m_channel_name.empty()) {" <<
          endl;
        out << indent(1) <<
          "pebble::OnRpcResponse on_rsp = cxx::bind(&" << scope << "recv_" << funname << "_parallel, this," << endl << indent(2) <<
          "cxx::placeholders::_1, cxx::placeholders::_2, cxx::placeholders::_3, ret_code" << ret_sync << ");" << endl << indent(1) <<
          "m_client->SendRequestParallel(GetHandle(), head, buff, buff_len, on_rsp, " << (*f_iter)->get_timeout_ms() << ", ret_code, num_called, num_parallel" << ");" <<
          endl;
        out << indent() << "} else {" << endl << indent(1) <<
          "*ret_code = m_client->BroadcastRequest(m_channel_name, head, buff, buff_len);" << endl << indent(1) <<
          "--(*num_called);" << endl << indent(1) <<
          "--(*num_parallel);" << endl << indent() <<
          "}" << endl << indent() <<
          "return *ret_code;" <<
          endl;
      } else {
        out << indent() <<
          "if (m_channel_name.empty()) {" <<
          endl;
          out << indent() <<
              "--(*num_called);" << endl << indent() <<
              "--(*num_parallel);" << endl << indent() <<
        out << indent(1) <<
          "pebble::OnRpcResponse on_rsp;" << endl << indent(1) <<
          "int32_t ret = m_client->SendRequest(GetHandle(), head, buff, buff_len, on_rsp, " << (*f_iter)->get_timeout_ms() << ");" << endl << indent(1) <<
          "if (ret != pebble::kRPC_SUCCESS) {" << endl << indent(2) <<
          "*ret_code = ret;" << endl << indent(1) <<
          "return *ret_code;" << endl << indent(1) <<
          "}" << endl <<
          endl;

        out << indent(1) <<
          "*ret_code = pebble::kRPC_SUCCESS;" << endl << indent(1) <<
          "return *ret_code;" << endl << indent(1) <<
          endl;

        out << indent() << "} else {" << endl << indent(1) <<
          "*ret_code = m_client->BroadcastRequest(m_channel_name, head, buff, buff_len);" << endl << indent() <<
          "return *ret_code;" << endl << indent() <<
          "}" <<
          endl;
      }

      scope_down(out);
      out <<
        endl;
      }

    // async function
    if (!(*f_iter)->is_oneway()) {
      indent(out) << function_signature_if(*f_iter, "CobCl", scope) << endl;
      scope_up(out);

      std::string ret_str;
      if (!(*f_iter)->get_returntype()->is_void()) {
        t_field returnfield((*f_iter)->get_returntype(), "response");
        out << indent() << declare_field(&returnfield, true) << endl;
        ret_str = ", response";
      }

      out << indent() << "::pebble::dr::protocol::TProtocol* encoder =" << endl << indent(1) <<
          "m_client->GetCodec(pebble::PebbleRpc::kMALLOC);" << endl << indent() <<
          "if (!encoder) {" << endl << indent(1) <<
          "cb(::pebble::kPEBBLE_RPC_UNKNOWN_CODEC_TYPE" << ret_str << ");" << endl << indent(1) <<
          "return;" << endl << indent() <<
          "}" << endl <<
          endl;

      out << indent() <<
          "::pebble::RpcHead head;" << endl << indent() <<
          "head.m_function_name.assign(\"" << service_name_ << ":" << funname << "\");" << endl << indent();
      if (!(*f_iter)->is_oneway()) {
          out << "head.m_message_type = pebble::dr::protocol::T_CALL;" << endl << indent();
      } else {
          out << "head.m_message_type = pebble::dr::protocol::T_ONEWAY;" << endl << indent();
      }
      out << "head.m_session_id = m_client->GenSessionId();" << endl << endl;

      out << indent() << argsname << " args;" << endl;
      for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
        out <<
          indent() << "args." << (*fld_iter)->get_name() << " = &" << (*fld_iter)->get_name() << ";" << endl;
      }
      out << endl;

      out << indent() <<
        "try {" << endl << indent(1) <<
        "args.write(encoder);" << endl << indent(1) <<
        "encoder->writeMessageEnd();" << endl << indent(1) <<
        "encoder->getTransport()->writeEnd();" << endl << indent() <<
        "} catch (pebble::TException ex) {" << endl << indent(1) <<
        "cb(pebble::kRPC_ENCODE_FAILED" << ret_str << ");" << endl << indent(1) <<
        "return;" << endl << indent() <<
        "}" << endl <<
        endl;

      out << indent() <<
        "uint8_t* buff = NULL;" << endl << indent() <<
        "uint32_t buff_len = 0;" << endl << indent() <<
        "(static_cast<pebble::dr::transport::TMemoryBuffer*>(encoder->getTransport().get()))->" << endl << indent(1) <<
        "getBuffer(&buff, &buff_len);" << endl <<
        endl;

      out << indent() <<
        "if (m_channel_name.empty()) {" <<
        endl;

      out << indent(1) <<
        "pebble::OnRpcResponse on_rsp = cxx::bind(&" << scope << "recv_" << funname << ", this," << endl << indent(2) <<
        "cxx::placeholders::_1, cxx::placeholders::_2, cxx::placeholders::_3, cb);" << endl << indent(1) <<
        "int32_t ret = m_client->SendRequest(GetHandle(), head, buff, buff_len, on_rsp, " << (*f_iter)->get_timeout_ms() << ");" << endl << indent(1) <<
        "if (ret != pebble::kRPC_SUCCESS) {" << endl << indent(2) <<
        "cb(ret" << ret_str << ");" << endl << indent(2) <<
        "return;" << endl << indent(1) <<
        "}" << endl;

      out << indent() << "} else {" << endl << indent(1) <<
        "int32_t ret = m_client->BroadcastRequest(m_channel_name, head, buff, buff_len);" << endl << indent(1) <<
        "if (ret != pebble::kRPC_SUCCESS) {" << endl << indent(2) <<
        "cb(ret" << ret_str << ");" << endl << indent(2) <<
        "return;" << endl << indent(1) <<
        "}" << endl << indent() <<
        "}" <<
        endl;

        scope_down(out);
        out << endl;
    }

    // 响应函数实现 recv_functionname_parallel
    {
    if (!(*f_iter)->is_oneway()) {
      std::string ret_str;
      std::string sync_para_str;
      if (!(*f_iter)->get_returntype()->is_void()) {
        ret_str = ", " + type_name((*f_iter)->get_returntype()) + "* response";
        sync_para_str = ", response";
      }
      out << indent() <<
        "int32_t " << scope << "recv_" << (*f_iter)->get_name() <<
        "_parallel(int32_t ret, const uint8_t* buff, uint32_t buff_len, int32_t* ret_code" <<
        ret_str << ")" <<
        endl;

      scope_up(out);

      out << indent() <<
          "*ret_code = recv_" << (*f_iter)->get_name() << "_sync(ret, buff, buff_len" << sync_para_str << ");" << endl << indent() <<
          "return *ret_code;" << endl;

      scope_down(out);
      out <<
        endl;
    }
    }
    // 响应函数实现 recv_functionname_sync
    if (!(*f_iter)->is_oneway()) {
      std::string ret_str;
      if (!(*f_iter)->get_returntype()->is_void()) {
        ret_str = ", " + type_name((*f_iter)->get_returntype()) + "* response";
      }
      out << indent() <<
        "int32_t " << scope << "recv_" << (*f_iter)->get_name() <<
        "_sync(int32_t ret, const uint8_t* buff, uint32_t buff_len" <<
        ret_str << ")" <<
        endl;

      scope_up(out);

      out << indent() <<
        "if (ret != pebble::kRPC_SUCCESS) {" << endl << indent(1) <<
        "if (0 == buff_len) {" << endl << indent(2) <<
        "return ret;" << endl << indent(1) <<
        "}" << endl << indent() <<
        "}" << endl <<
        endl;

      out << indent() <<
        "::pebble::dr::protocol::TProtocol* decoder =" << endl << indent(1) <<
        "m_client->GetCodec(pebble::PebbleRpc::kBORROW);" << endl << indent() <<
        "if (!decoder) {" << endl << indent(1) <<
        "return ::pebble::kPEBBLE_RPC_UNKNOWN_CODEC_TYPE;" << endl << indent() <<
        "}" << endl <<
        endl;

      out << indent() <<
        "(static_cast<pebble::dr::transport::TMemoryBuffer*>(decoder->getTransport().get()))->" << endl << indent(1) <<
        "resetBuffer(const_cast<uint8_t*>(buff), buff_len, ::pebble::dr::transport::TMemoryBuffer::OBSERVE);" << endl <<
        endl;

      out << indent() <<
        tservice->get_name() << "_" << (*f_iter)->get_name() << "_presult result;" << endl;
      if (!ret_str.empty()) {
        out << indent() <<
          "result.success = response;" << endl;
      }
      out << indent() <<
        "try {" << endl << indent(1) <<
        "result.read(decoder);" << endl << indent(1) <<
        "decoder->readMessageEnd();" << endl << indent(1) <<
        "decoder->getTransport()->readEnd();" << endl << indent() <<
        "} catch (pebble::TException ex) {" << endl << indent(1) <<
        "return pebble::kRPC_DECODE_FAILED;" << endl << indent() <<
        "}" << endl <<
        endl;

      if (!ret_str.empty()) {
        out << indent() <<
          "if (!result.__isset.success) {" << endl << indent(1) <<
          "return pebble::kPEBBLE_RPC_MISS_RESULT;" << endl << indent() <<
          "}" << endl <<
          endl;
      }

      out << indent() <<
        "return ret != pebble::kRPC_SUCCESS ? ret : pebble::kRPC_SUCCESS;" << endl;

      scope_down(out);
      out <<
        endl;

      // 响应函数实现 recv_functionname
      std::string ret_async = ", cxx::function<void(int ret_code)>& cb";
      std::string ret_value;
      if (!(*f_iter)->get_returntype()->is_void()) {
        std::string type_const;
        std::string type_ref;
        if (is_complex_type((*f_iter)->get_returntype())) {
            type_const = "const ";
            type_ref   = "&";
        }
        ret_async = ", cxx::function<void(int ret_code, " + type_const + type_name((*f_iter)->get_returntype()) + type_ref + " response)>& cb";
        ret_value = ", response";
      }
      out << indent() <<
        "int32_t " << scope << "recv_" << (*f_iter)->get_name() <<
        "(int32_t ret, const uint8_t* buff, uint32_t buff_len" <<
        ret_async << ")" <<
        endl;

      scope_up(out);

      if (!(*f_iter)->get_returntype()->is_void()) {
        t_field returnfield((*f_iter)->get_returntype(), "response");
        out << indent() << declare_field(&returnfield, true) << endl;
      }

      out << indent() <<
        "if (ret != pebble::kRPC_SUCCESS) {" << endl << indent(1) <<
        "if (0 == buff_len) {" << endl << indent(2) <<
        "cb(ret" << ret_value << ");" << endl << indent(2) <<
        "return ret;" << endl << indent(1) <<
        "}" << endl << indent() <<
        "}" << endl <<
        endl;

      out << indent() <<
        "::pebble::dr::protocol::TProtocol* decoder =" << endl << indent(1) <<
        "m_client->GetCodec(pebble::PebbleRpc::kBORROW);" << endl << indent() <<
        "if (!decoder) {" << endl << indent(1) <<
        "cb(::pebble::kPEBBLE_RPC_UNKNOWN_CODEC_TYPE" << ret_value << ");" << endl << indent(1) <<
        "return ::pebble::kPEBBLE_RPC_UNKNOWN_CODEC_TYPE;" << endl << indent() <<
        "}" << endl <<
        endl;

      out << indent() <<
        "(static_cast<pebble::dr::transport::TMemoryBuffer*>(decoder->getTransport().get()))->" << endl << indent(1) <<
        "resetBuffer(const_cast<uint8_t*>(buff), buff_len, ::pebble::dr::transport::TMemoryBuffer::OBSERVE);" << endl << indent(1) <<
        endl;

      out << indent() <<
        tservice->get_name() << "_" << (*f_iter)->get_name() << "_presult result;" << endl;
      if (!(*f_iter)->get_returntype()->is_void()) {
        out << indent() <<
          "result.success = &response;" << endl;
      }
      out << indent() <<
        "try {" << endl << indent(1) <<
        "result.read(decoder);" << endl << indent(1) <<
        "decoder->readMessageEnd();" << endl << indent(1) <<
        "decoder->getTransport()->readEnd();" << endl << indent() <<
        "} catch (pebble::TException ex) {" << endl << indent(1) <<
        "cb(pebble::kRPC_DECODE_FAILED" << ret_value << ");" << endl << indent(1) <<
        "return pebble::kRPC_DECODE_FAILED;" << endl << indent() <<
        "}" << endl <<
        endl;

      if (!(*f_iter)->get_returntype()->is_void()) {
        out << indent() <<
          "if (!result.__isset.success) {" << endl << indent(1) <<
          "cb(pebble::kPEBBLE_RPC_MISS_RESULT" << ret_value << ");" << endl << indent(1) <<
          "return pebble::kPEBBLE_RPC_MISS_RESULT;" << endl << indent() <<
          "}" << endl <<
          endl;
      }

      out << indent() <<
        "cb(ret != pebble::kRPC_SUCCESS ? ret : pebble::kRPC_SUCCESS" << ret_value << ");" << endl << indent() <<
        "return ret != pebble::kRPC_SUCCESS ? ret : pebble::kRPC_SUCCESS;" << endl;

      scope_down(out);
      out << endl;
    }
  }
}

class ProcessorGenerator {
 public:
  ProcessorGenerator(t_cpp_generator* generator, t_service* service,
                     const string& style);

  void run() {
    generate_class_definition();

    generate_construct_function();

    generate_register_service_function();

    generate_process_functions();
  }

  void generate_class_definition();
  void generate_construct_function();
  void generate_register_service_function();
  void generate_process_functions();
  void generate_factory();

 protected:
  std::string type_name(t_type* ttype, bool in_typedef=false, bool arg=false) {
    return generator_->type_name(ttype, in_typedef, arg);
  }

  std::string indent(int inc = 0) {
    return generator_->indent(inc);
  }
  std::ostream& indent(std::ostream &os) {
    return generator_->indent(os);
  }

  void indent_up() {
    generator_->indent_up();
  }
  void indent_down() {
    generator_->indent_down();
  }

  t_cpp_generator* generator_;
  t_service* service_;
  std::ofstream& f_service_h_;
  std::ofstream& f_out_;
  string service_name_;
  string style_;
  string pstyle_;
  string class_name_;
  string if_name_;
  string factory_class_name_;
  string finish_cob_;
  string finish_cob_decl_;
  string ret_type_;
  string call_context_;
  string cob_arg_;
  string call_context_arg_;
  string call_context_decl_;
  string template_header_;
  string template_suffix_;
  string typename_str_;
  string class_suffix_;
  string extends_;
};

ProcessorGenerator::ProcessorGenerator(t_cpp_generator* generator,
                                       t_service* service,
                                       const string& style)
  : generator_(generator),
    service_(service),
    f_service_h_(generator->f_service_h_),
    f_out_(generator->f_service_cpp_),
    service_name_(generator->service_name_),
    style_(style) {
  if (style_ == "Cob") {
    pstyle_ = "Async";
    class_name_ = service_name_ + "Handler";
    if_name_ = service_name_ + "CobSvIf";

    finish_cob_ = "cxx::function<void(bool ok)> cob, ";
    finish_cob_decl_ = "cxx::function<void(bool ok)>, ";
    cob_arg_ = "cob, ";
    ret_type_ = "void ";
  } else {
    class_name_ = service_name_ + "Processor";
    if_name_ = service_name_ + "If";

    ret_type_ = "bool ";
    // TODO(edhall) callContext should eventually be added to TAsyncProcessor
    call_context_ = ", void* callContext";
    call_context_arg_ = ", callContext";
    call_context_decl_ = ", void*";
  }

  factory_class_name_ = class_name_ + "Factory";

  if (generator->gen_templates_) {
    template_header_ = "template <class Protocol_>\n";
    template_suffix_ = "<Protocol_>";
    typename_str_ = "typename ";
    class_name_ += "T";
    factory_class_name_ += "T";
  }

  if (service_->get_extends() != NULL) {
    extends_ = type_name(service_->get_extends()) + "Handler";
    if (generator_->gen_templates_) {
      // TODO(simpkins): If gen_templates_ is enabled, we currently assume all
      // parent services were also generated with templates enabled.
      extends_ += "T<Protocol_>";
    }
  }
}

void ProcessorGenerator::generate_class_definition() {
  vector<t_function*> functions = service_->get_functions();
  vector<t_function*>::iterator f_iter;

  string parent_class;
  if (service_->get_extends() != NULL) {
    parent_class = extends_;
  } else {
      parent_class = "::pebble::IPebbleRpcService";
  }

  // Generate the header portion
  // generate processor definition to types file
  std::ofstream& out = generator_->f_service_inh_;

  // extern interface class
  out << "class " << if_name_ << ";" << endl;

  out <<
    template_header_ <<
    "class " << class_name_ << " : public " << parent_class << " {" << endl;

  // private data members
  if (service_->get_extends() == NULL) {
    out << "protected:" << endl << indent(1) <<
      "pebble::PebbleRpc* m_server;" << endl <<
      endl;
  }
  out <<
    "private:" << endl << indent(1) <<
    if_name_ << "* m_iface;" << endl <<
    endl;

  // Process function declarations
  out <<
    "private:" << endl;
  indent_up();

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    indent(out) <<
      "int32_t process_" << (*f_iter)->get_name() <<
      "(const uint8_t* buff, uint32_t buff_len, " << endl << indent(1) <<
      "cxx::function<int32_t(int32_t ret, const uint8_t* buff, uint32_t buff_len)>& rsp);" << endl;

    if (!(*f_iter)->is_oneway()) {
      string ret_arg = ", int32_t ret_code";
      if (!(*f_iter)->get_returntype()->is_void()) {
        std::string type_const = ", ";
        std::string type_ref;
        if (generator_->is_complex_type((*f_iter)->get_returntype())) {
          type_const = ", const ";
          type_ref   = "&";
        }
        ret_arg += type_const + type_name((*f_iter)->get_returntype()) + type_ref + " response";
      }
      indent(out) << "void return_" << (*f_iter)->get_name() <<
        "(cxx::function<int32_t(int32_t ret, const uint8_t* buff, uint32_t buff_len)>& rsp" << endl << indent(1) <<
        ret_arg << ");" << endl;
    }
  }

  out << endl <<
    "public:" << endl <<
    indent() << class_name_ <<
    "(pebble::PebbleRpc* rpc, " << if_name_ << "* iface);" << endl;

  out <<
    indent() << "virtual ~" << class_name_ << "() {}" << endl;

  out <<
    indent() << "virtual int32_t RegisterServiceFunction();" << endl;

  out <<
    indent() << "virtual std::string Name() { return \"" << service_name_ << "\"; }" <<  endl;

  indent_down();

  out <<
    "};" << endl << endl;
}

void ProcessorGenerator::generate_construct_function() {
  vector<t_function*> functions = service_->get_functions();
  vector<t_function*>::iterator f_iter;

  f_out_ <<
    class_name_ << "::" << class_name_ <<
    "(pebble::PebbleRpc* rpc, " << if_name_ << "* iface) ";
  if (!extends_.empty()) {
    f_out_ << endl <<
      indent(1) << ":" << extends_ << "(rpc, iface) {" << endl << indent(1) <<
      "m_iface = iface;" << endl;
  } else {
    f_out_ << "{" << endl << indent(1) << "m_server = rpc;" << endl <<
        indent(1) << "m_iface = iface;" << endl;
  }

  f_out_ <<
    indent() << "}" << endl << endl;
}

void ProcessorGenerator::generate_register_service_function() {
  vector<t_function*> functions = service_->get_functions();
  vector<t_function*>::iterator f_iter;

  f_out_ <<
    "int32_t " << class_name_ << "::RegisterServiceFunction() {" <<
    endl;
  indent_up();

  f_out_ << indent() <<
    "if (!m_server) {" << endl << indent(1) <<
    "return pebble::kRPC_INVALID_PARAM;" << endl << indent() <<
    "}" << endl <<
    endl;

  f_out_ << indent() <<
    "pebble::OnRpcRequest cb;" << endl << indent() <<
    "int32_t ret = 0;" << endl <<
    endl;

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_out_ <<
      indent() << "cb = cxx::bind(&" << class_name_ <<
      "::process_" << (*f_iter)->get_name() << ", this, " << endl << indent(1) <<
      "cxx::placeholders::_1, cxx::placeholders::_2, cxx::placeholders::_3);" << endl;

    f_out_ <<
      indent() << "ret = m_server->AddOnRequestFunction(\"" << service_name_ <<
      ":" << (*f_iter)->get_name() << "\", cb);" << endl;

    f_out_ <<
      indent() << "if (ret != pebble::kRPC_SUCCESS) {" << endl <<
      indent(1) << "return ret;" << endl <<
      indent() << "}" << endl <<
      endl;
  }

  if (!extends_.empty()) {
    f_out_ <<
      indent() << "ret = " << extends_ << "::RegisterServiceFunction();" << endl <<
      indent() << "if (ret != pebble::kRPC_SUCCESS && ret != pebble::kRPC_FUNCTION_NAME_EXISTED) {" << endl <<
      indent(1) << "return ret;" << endl <<
      indent() << "}" << endl <<
      endl;
  }
  f_out_ <<
    indent() << "return pebble::kRPC_SUCCESS;" << endl;

  indent_down();
  f_out_ <<
    indent() << "}" << endl << endl;
}

void ProcessorGenerator::generate_process_functions() {
  vector<t_function*> functions = service_->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
      generator_->generate_process_function(service_, *f_iter, style_, false);
      if (!(*f_iter)->is_oneway()) {
        generator_->generate_return_function(service_, *f_iter, style_, false);
      }
  }
}

void ProcessorGenerator::generate_factory() {
  string if_factory_name = if_name_ + "Factory";

  // Generate the factory class definition
  f_service_h_ <<
    template_header_ <<
    "class " << factory_class_name_ <<
      " : public ::pebble::rpc::processor::" <<
        (style_ == "Cob" ? "TAsyncProcessorFactory" :  "TProcessorFactory") <<
        " {" << endl <<
    "public:" << endl;
  indent_up();

  f_service_h_ <<
    indent() << factory_class_name_ << "(const ::cxx::shared_ptr< " <<
      if_factory_name << " >& handlerFactory) :" << endl <<
    indent(2) << "handlerFactory_(handlerFactory) {}" << endl <<
    endl <<
    indent() << "::cxx::shared_ptr< ::pebble::rpc::processor::" <<
      (style_ == "Cob" ? "TAsyncProcessor" :  "TProcessor") << " > " <<
      "getProcessor(const ::pebble::rpc::processor::TConnectionInfo& connInfo);" <<
      endl;

  f_service_h_ <<
    endl <<
    "protected:" << endl <<
    indent() << "::cxx::shared_ptr< " << if_factory_name <<
      " > handlerFactory_;" << endl;

  indent_down();
  f_service_h_ <<
    "};" << endl << endl;

  // If we are generating templates, output a typedef for the plain
  // factory name.
  if (generator_->gen_templates_) {
    f_service_h_ <<
      "typedef " << factory_class_name_ <<
      "< ::pebble::dr::protocol::TDummyProtocol > " <<
      service_name_ << pstyle_ << "ProcessorFactory;" << endl << endl;
  }

  // Generate the getProcessor() method
  f_out_ <<
    template_header_ <<
    indent() << "::cxx::shared_ptr< ::pebble::rpc::processor::" <<
      (style_ == "Cob" ? "TAsyncProcessor" :  "TProcessor") << " > " <<
      factory_class_name_ << template_suffix_ << "::getProcessor(" <<
      "const ::pebble::rpc::processor::TConnectionInfo& connInfo) {" << endl;
  indent_up();

  f_out_ <<
    indent() << "::pebble::rpc::processor::ReleaseHandler< " << if_factory_name <<
      " > cleanup(handlerFactory_);" << endl <<
    indent() << "::cxx::shared_ptr< " << if_name_ << " > handler(" <<
      "handlerFactory_->getHandler(connInfo), cleanup);" << endl <<
    indent() << "::cxx::shared_ptr< ::pebble::rpc::processor::" <<
      (style_ == "Cob" ? "TAsyncProcessor" :  "TProcessor") << " > " <<
      "processor(new " << class_name_ << template_suffix_ <<
      "(handler));" << endl <<
    indent() << "return processor;" << endl;

  indent_down();
  f_out_ <<
    indent() << "}" << endl;
}

/**
 * Generates a service processor definition.
 *
 * @param tservice The service to generate a processor for.
 */
void t_cpp_generator::generate_service_processor(t_service* tservice,
                                                 string style) {
  ProcessorGenerator generator(this, tservice, style);
  generator.run();
}

/**
 * Generates a struct and helpers for a function.
 *
 * @param tfunction The function
 */
void t_cpp_generator::generate_function_helpers(t_service* tservice,
                                                t_function* tfunction) {
  if (tfunction->is_oneway()) {
    return;
  }

  // std::ofstream& out = (gen_templates_ ? f_service_tcc_ : f_service_cpp_);
  std::ofstream& out = f_service_cpp_;

  t_struct result(program_, tservice->get_name() + "_" + tfunction->get_name() + "_result");
  t_field success(tfunction->get_returntype(), "success", 0);
  if (!tfunction->get_returntype()->is_void()) {
    result.append(&success);
  }

  t_struct* xs = tfunction->get_xceptions();
  const vector<t_field*>& fields = xs->get_members();
  vector<t_field*>::const_iterator f_iter;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    result.append(*f_iter);
  }

  generate_struct_declaration(f_service_inh_, &result, false);
  generate_struct_definition(out, out, &result, false);
  generate_struct_reader(out, &result);
  generate_struct_result_writer(out, &result);

  result.set_name(tservice->get_name() + "_" + tfunction->get_name() + "_presult");
  generate_struct_declaration(f_service_inh_, &result, false, true, true, gen_cob_style_);
  generate_struct_definition(out, out, &result, false);
  generate_struct_reader(out, &result, true);
  if (gen_cob_style_) {
    generate_struct_writer(out, &result, true);
  }

}

/**
 * Generates a process function definition.
 *
 * @param tfunction The function to write a dispatcher for
 */
void t_cpp_generator::generate_process_function(t_service* tservice,
                                                t_function* tfunction,
                                                string style,
                                                bool specialized) {
  t_struct* arg_struct = tfunction->get_arglist();
  const std::vector<t_field*>& fields = arg_struct->get_members();
  vector<t_field*>::const_iterator f_iter;

  string service_func_name = "\"" + tservice->get_name() + "." +
    tfunction->get_name() + "\"";

  std::ofstream& out = f_service_cpp_;

  // Processor entry point.
  // TODO(edhall) update for callContext when TEventServer is ready

  out <<
    "int32_t " << tservice->get_name() << "Handler" <<
    "::process_" << tfunction->get_name() <<
    "(" << endl << indent(1) <<
    "const uint8_t* buff, uint32_t buff_len," << endl << indent(1) <<
    "cxx::function<int32_t(int32_t ret, const uint8_t* buff, uint32_t buff_len)>& rsp)" <<
    endl;
  scope_up(out);

  out << indent() <<
    "::pebble::dr::protocol::TProtocol* decoder = m_server->GetCodec(pebble::PebbleRpc::kBORROW);" << endl << indent(1) <<
    "if (!decoder) {" << endl << indent(1) <<
    "rsp(pebble::kPEBBLE_RPC_UNKNOWN_CODEC_TYPE, NULL, 0);" << endl << indent(1) <<
    "return pebble::kPEBBLE_RPC_UNKNOWN_CODEC_TYPE;" << endl << indent() <<
    "}" << endl << endl << indent() <<
    "static_cast<pebble::dr::transport::TMemoryBuffer*>(decoder->getTransport().get())->" << endl << indent(1) <<
    "resetBuffer(const_cast<uint8_t*>(buff), buff_len, ::pebble::dr::transport::TMemoryBuffer::OBSERVE);" << endl <<
    endl;

  out <<
    indent() << tservice->get_name() + "_" + tfunction->get_name() << "_args args;" << endl << indent() <<
      "try {" << endl << indent(1) <<
      "args.read(decoder);" << endl << indent(1) <<
      "decoder->readMessageEnd();" << endl << indent(1) <<
      "decoder->getTransport()->readEnd();" << endl << indent() <<
      "} catch (pebble::TException ex) {" << endl << indent(1) <<
      "rsp(pebble::kPEBBLE_RPC_DECODE_BODY_FAILED, NULL, 0);" << endl << indent(1) <<
      "return pebble::kPEBBLE_RPC_DECODE_BODY_FAILED;" << endl << indent() <<
      "}" << endl << endl;

  std::string cb_func;
  if (!tfunction->is_oneway()) {
    if (!tfunction->get_returntype()->is_void()) {
      std::string type_const;
      std::string type_ref;
      if (is_complex_type(tfunction->get_returntype())) {
        type_const = "const ";
        type_ref   = "&";
      }
      out << indent() <<
        "cxx::function<void(int32_t ret_code, " << type_const << type_name(tfunction->get_returntype()) << type_ref << " response)> tmp =" << endl << indent(1) <<
        "cxx::bind(&" << tservice->get_name() << "Handler::return_" << tfunction->get_name() << ", this," << endl << indent(2) <<
        "rsp, cxx::placeholders::_1, cxx::placeholders::_2);" << endl <<
        endl;
    } else {
      out << indent() <<
        "cxx::function<void(int32_t ret_code)> tmp =" << endl << indent(1) <<
        "cxx::bind(&" << tservice->get_name() << "Handler::return_" << tfunction->get_name() << ", this," << endl << indent(2) <<
        "rsp, cxx::placeholders::_1);" << endl <<
        endl;
    }
    cb_func = "tmp";
  }

  out << indent() << "m_iface->" << tfunction->get_name() << "(";

  std::string para_sp;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (f_iter != fields.begin()) {
        out << ",";
    }
    out << endl << indent(1) << "args." << (*f_iter)->get_name();
    para_sp = ",";
  }

  if (!tfunction->is_oneway()) {
    if (!para_sp.empty() && !cb_func.empty()) {
      out << para_sp << endl << indent(1);
    }
    out << cb_func <<
      endl;
  }

  out << indent() << ");" << endl << endl;

  out << indent() << "return pebble::kRPC_SUCCESS;" << endl;

  scope_down(out);
  out << endl;
}

void t_cpp_generator::generate_return_function  (t_service* tservice, t_function* tfunction,
                                   string style, bool specialized) {
  string service_func_name = "\"" + tservice->get_name() + "." +
    tfunction->get_name() + "\"";

  std::ofstream& out = f_service_cpp_;

  // Processor entry point.
  // TODO(edhall) update for callContext when TEventServer is ready

  std::string ret_value = ", int32_t ret_code";
  if (!tfunction->get_returntype()->is_void()) {
    std::string type_ref;
    std::string type_const;
    if (is_complex_type(tfunction->get_returntype())) {
        type_ref   = "&";
        type_const = "const ";
    }
    ret_value += ", " + type_const + type_name(tfunction->get_returntype()) + type_ref + " response";
  }

  out <<
    "void " << tservice->get_name() << "Handler" <<
    "::return_" << tfunction->get_name() << "(" << endl << indent(1) <<
    "cxx::function<int32_t(int32_t ret, const uint8_t* buff, uint32_t buff_len)>& rsp" << endl << indent(1) <<
    ret_value << ")" <<
    endl;
  scope_up(out);

  #if 0
  out << indent() <<
    "if (ret_code != 0) {" << endl << indent(1) <<
    "rsp(ret_code, NULL, 0);" << endl << indent(1) <<
    "return;" << endl << indent() <<
    "}" << endl <<
    endl;
  #endif

  out << indent() <<
    "::pebble::dr::protocol::TProtocol* encoder = m_server->GetCodec(pebble::PebbleRpc::kMALLOC);" << endl << indent(1) <<
    "if (!encoder) {" << endl << indent(1) <<
    "rsp(pebble::kPEBBLE_RPC_UNKNOWN_CODEC_TYPE, NULL, 0);" << endl << indent(1) <<
    "return;" << endl << indent() <<
    "}" << endl <<
    endl;

  out <<
    indent() << tservice->get_name() + "_" + tfunction->get_name() << "_presult result;" << endl;

  if (!tfunction->get_returntype()->is_void()) {
    // The const_cast here is unfortunate, but it would be a pain to avoid,
    // and we only do a write with this struct, which is const-safe.
    out <<
      indent() << "result.success = const_cast<" <<
        type_name(tfunction->get_returntype()) << "*>(&response);" <<
        endl <<
      indent() << "result.__isset.success = true;" << endl;
  }

  out << endl << indent() <<
    "try {" << endl << indent(1) <<
    "result.write(encoder);" << endl << indent(1) <<
    "encoder->writeMessageEnd();" << endl << indent(1) <<
    "encoder->getTransport()->writeEnd();" << endl << indent() <<
    "} catch (pebble::TException ex) {" << endl << indent(1) <<
    "rsp(pebble::kPEBBLE_RPC_ENCODE_BODY_FAILED, NULL, 0);" << endl << indent(1) <<
    "return;" << endl << indent() <<
    "}" << endl << endl;

  out << indent() <<
    "uint8_t* buff = NULL;" << endl << indent() <<
    "uint32_t buff_len = 0;" << endl << indent() <<
    "(static_cast<pebble::dr::transport::TMemoryBuffer*>(encoder->getTransport().get()))->" << endl << indent(1) <<
    "getBuffer(&buff, &buff_len);" << endl << endl << indent() <<
    "rsp(ret_code, buff, buff_len);" << endl;

  scope_down(out);
  out << endl;
}

/**
 * Generates a skeleton file of a server
 *
 * @param tservice The service to generate a server for.
 */
void t_cpp_generator::generate_service_skeleton(t_service* tservice) {
  string svcname = tservice->get_name();

  // Service implementation file includes
  string f_skeleton_name = get_out_dir()+svcname+"_server.skeleton.cpp";

  string ns = namespace_prefix(tservice->get_program()->get_namespace("cpp"));

  ofstream f_skeleton;
  f_skeleton.open(f_skeleton_name.c_str());
  f_skeleton <<
    "// This autogenerated skeleton file illustrates how to build a server." << endl <<
    "// You should copy it to another filename to avoid overwriting it." << endl <<
    endl <<
    "#include \"" << get_include_prefix(*get_program()) << svcname << ".h\"" << endl <<
    "#include <source/rpc/protocol/binary_protocol.h>" << endl <<
    "#include <source/rpc/server/simple_server.h>" << endl <<
    "#include <source/rpc/transport/server_socket.h>" << endl <<
    "#include <source/rpc/transport/buffer_transport.h>" << endl <<
    endl <<
    "using namespace ::pebble::rpc;" << endl <<
    "using namespace ::pebble::dr::protocol;" << endl <<
    "using namespace ::pebble::dr::transport;" << endl <<
    "using namespace ::pebble::rpc::server;" << endl <<
    endl <<
    "using cxx::shared_ptr;" << endl <<
    endl;

  // the following code would not compile:
  // using namespace ;
  // using namespace ::;
  if ( (!ns.empty()) && (ns.compare(" ::") != 0)) {
    f_skeleton <<
      "using namespace " << string(ns, 0, ns.size()-2) << ";" << endl <<
      endl;
  }

  f_skeleton <<
    "class " << svcname << "Handler : virtual public " << svcname << "If {" << endl <<
    "public:" << endl;
  indent_up();
  f_skeleton <<
    indent() << svcname << "Handler() {" << endl <<
    indent(1) << "// Your initialization goes here" << endl <<
    indent() << "}" << endl <<
    endl;

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    generate_java_doc(f_skeleton, *f_iter);
    f_skeleton <<
      indent() << function_signature(*f_iter, "") << " {" << endl <<
      indent(1) << "// Your implementation goes here" << endl <<
      indent(1) << "printf(\"" << (*f_iter)->get_name() << "\\n\");" << endl <<
      indent() << "}" << endl <<
      endl;
  }

  indent_down();
  f_skeleton <<
    "};" << endl <<
    endl;

  f_skeleton <<
    indent() << "int main(int argc, char **argv) {" << endl;
  indent_up();
  f_skeleton <<
    indent() << "int port = 9090;" << endl <<
    indent() << "shared_ptr<" << svcname << "Handler> handler(new " << svcname << "Handler());" << endl <<
    indent() << "shared_ptr<TProcessor> processor(new " << svcname << "Processor(handler));" << endl <<
    indent() << "shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));" << endl <<
    indent() << "shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());" << endl <<
    indent() << "shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());" << endl <<
    endl <<
    indent() << "TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);" << endl <<
    indent() << "server.serve();" << endl <<
    indent() << "return 0;" << endl;
  indent_down();
  f_skeleton <<
    "}" << endl <<
    endl;

  // Close the files
  f_skeleton.close();
}

/**
 * Deserializes a field of any type.
 */
void t_cpp_generator::generate_deserialize_field(ofstream& out,
                                                 t_field* tfield,
                                                 string iprot,
                                                 string prefix,
                                                 string suffix) {
  t_type* type = get_true_type(tfield->get_type());

  if (type->is_void()) {
    throw "CANNOT GENERATE DESERIALIZE CODE FOR void TYPE: " +
      prefix + tfield->get_name();
  }

  string name = prefix + tfield->get_name() + suffix;

  if (type->is_struct() || type->is_xception()) {
    generate_deserialize_struct(out, (t_struct*)type, iprot, name, is_reference(tfield));
  } else if (type->is_container()) {
    generate_deserialize_container(out, type, iprot, name);
  } else if (type->is_base_type()) {
    indent(out) <<
      "xfer += " << iprot << "->";
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "compiler error: cannot serialize void field in a struct: " + name;
      break;
    case t_base_type::TYPE_STRING:
      if (((t_base_type*)type)->is_binary()) {
        out << "readBinary(" << name << ");";
      }
      else {
        out << "readString(" << name << ");";
      }
      break;
    case t_base_type::TYPE_BOOL:
      out << "readBool(" << name << ");";
      break;
    case t_base_type::TYPE_BYTE:
      out << "readByte(reinterpret_cast<int8_t&>(" << name << "));";
      break;
    case t_base_type::TYPE_I16:
      out << "readI16(reinterpret_cast<int16_t&>(" << name << "));";
      break;
    case t_base_type::TYPE_I32:
      out << "readI32(reinterpret_cast<int32_t&>(" << name << "));";
      break;
    case t_base_type::TYPE_I64:
      out << "readI64(reinterpret_cast<int64_t&>(" << name << "));";
      break;
    case t_base_type::TYPE_DOUBLE:
      out << "readDouble(" << name << ");";
      break;
    default:
      throw "compiler error: no C++ reader for base type " + t_base_type::t_base_name(tbase) + name;
    }
    out <<
      endl;
  } else if (type->is_enum()) {
    string t = tmp("ecast");
    out <<
      indent() << "int32_t " << t << ";" << endl <<
      indent() << "xfer += " << iprot << "->readI32(" << t << ");" << endl <<
      indent() << name << " = (" << type_name(type) << ")" << t << ";" << endl;
  } else {
    printf("DO NOT KNOW HOW TO DESERIALIZE FIELD '%s' TYPE '%s'\n",
           tfield->get_name().c_str(), type_name(type).c_str());
  }
}

/**
 * Generates an unserializer for a variable. This makes two key assumptions,
 * first that there is a const char* variable named data that points to the
 * buffer for deserialization, and that there is a variable protocol which
 * is a reference to a TProtocol serialization object.
 */
void t_cpp_generator::generate_deserialize_struct(ofstream& out,
                                                  t_struct* tstruct,
                                                  string iprot,
                                                  string prefix,
                                                  bool pointer) {
  if (pointer) {
    indent(out) << "if (!" << prefix << ") { " << endl;
    indent(out) << indent() << prefix << " = cxx::shared_ptr<" << type_name(tstruct) << ">(new " << type_name(tstruct) << ");" << endl;
    indent(out) << "}" << endl;
    indent(out) <<
      "xfer += " << prefix << "->read(" << iprot << ");" << endl;
    indent(out) << "bool wasSet = false;" << endl;
    const vector<t_field*>& members = tstruct->get_members();
    vector<t_field*>::const_iterator f_iter;
    for (f_iter = members.begin(); f_iter != members.end(); ++f_iter) {

      indent(out) << "if (" << prefix << "->__isset." << (*f_iter)->get_name() << ") { wasSet = true; }" << endl;
    }
    indent(out) << "if (!wasSet) { " << prefix << ".reset(); }" << endl;
  } else {
    indent(out) <<
      "xfer += " << prefix << ".read(" << iprot << ");" << endl;
  }
}

void t_cpp_generator::generate_deserialize_container(ofstream& out,
                                                     t_type* ttype,
                                                     string iprot,
                                                     string prefix) {
  scope_up(out);

  string size = tmp("_size");
  string ktype = tmp("_ktype");
  string vtype = tmp("_vtype");
  string etype = tmp("_etype");

  t_container* tcontainer = (t_container*)ttype;
  bool use_push = tcontainer->has_cpp_name();

  indent(out) <<
    prefix << ".clear();" << endl <<
    indent() << "uint32_t " << size << ";" << endl;

  // Declare variables, read header
  if (ttype->is_map()) {
    out <<
      indent() << "::pebble::dr::protocol::TType " << ktype << ";" << endl <<
      indent() << "::pebble::dr::protocol::TType " << vtype << ";" << endl <<
      indent() << "xfer += " << iprot << "->readMapBegin(" <<
                   ktype << ", " << vtype << ", " << size << ");" << endl;
  } else if (ttype->is_set()) {
    out <<
      indent() << "::pebble::dr::protocol::TType " << etype << ";" << endl <<
      indent() << "xfer += " << iprot << "->readSetBegin(" <<
                   etype << ", " << size << ");" << endl;
  } else if (ttype->is_list()) {
    out <<
      indent() << "::pebble::dr::protocol::TType " << etype << ";" << endl <<
      indent() << "xfer += " << iprot << "->readListBegin(" <<
      etype << ", " << size << ");" << endl;
    if (!use_push) {
      indent(out) << prefix << ".resize(" << size << ");" << endl;
    }
  }


  // For loop iterates over elements
  string i = tmp("_i");
  out <<
    indent() << "uint32_t " << i << ";" << endl <<
    indent() << "for (" << i << " = 0; " << i << " < " << size << "; ++" << i << ")" << endl;

    scope_up(out);

    if (ttype->is_map()) {
      generate_deserialize_map_element(out, (t_map*)ttype, iprot, prefix);
    } else if (ttype->is_set()) {
      generate_deserialize_set_element(out, (t_set*)ttype, iprot, prefix);
    } else if (ttype->is_list()) {
      generate_deserialize_list_element(out, (t_list*)ttype, iprot, prefix, use_push, i);
    }

    scope_down(out);

  // Read container end
  if (ttype->is_map()) {
    indent(out) << "xfer += " << iprot << "->readMapEnd();" << endl;
  } else if (ttype->is_set()) {
    indent(out) << "xfer += " << iprot << "->readSetEnd();" << endl;
  } else if (ttype->is_list()) {
    indent(out) << "xfer += " << iprot << "->readListEnd();" << endl;
  }

  scope_down(out);
}


/**
 * Generates code to deserialize a map
 */
void t_cpp_generator::generate_deserialize_map_element(ofstream& out,
                                                       t_map* tmap,
                                                       string iprot,
                                                       string prefix) {
  string key = tmp("_key");
  string val = tmp("_val");
  t_field fkey(tmap->get_key_type(), key);
  t_field fval(tmap->get_val_type(), val);

  out <<
    indent() << declare_field(&fkey) << endl;

  generate_deserialize_field(out, &fkey, iprot);
  indent(out) <<
    declare_field(&fval, false, false, false, true) << " = " <<
    prefix << "[" << key << "];" << endl;

  generate_deserialize_field(out, &fval, iprot);
}

void t_cpp_generator::generate_deserialize_set_element(ofstream& out,
                                                       t_set* tset,
                                                       string iprot,
                                                       string prefix) {
  string elem = tmp("_elem");
  t_field felem(tset->get_elem_type(), elem);

  indent(out) <<
    declare_field(&felem) << endl;

  generate_deserialize_field(out, &felem, iprot);

  indent(out) <<
    prefix << ".insert(" << elem << ");" << endl;
}

void t_cpp_generator::generate_deserialize_list_element(ofstream& out,
                                                        t_list* tlist,
                                                        string iprot,
                                                        string prefix,
                                                        bool use_push,
                                                        string index) {
  if (use_push) {
    string elem = tmp("_elem");
    t_field felem(tlist->get_elem_type(), elem);
    indent(out) << declare_field(&felem) << endl;
    generate_deserialize_field(out, &felem, iprot);
    indent(out) << prefix << ".push_back(" << elem << ");" << endl;
  } else {
    t_field felem(tlist->get_elem_type(), prefix + "[" + index + "]");
    generate_deserialize_field(out, &felem, iprot);
  }
}


/**
 * Serializes a field of any type.
 *
 * @param tfield The field to serialize
 * @param prefix Name to prepend to field name
 */
void t_cpp_generator::generate_serialize_field(ofstream& out,
                                               t_field* tfield,
                                               string oprot,
                                               string prefix,
                                               string suffix) {
  t_type* type = get_true_type(tfield->get_type());

  string name = prefix + tfield->get_name() + suffix;

  // Do nothing for void types
  if (type->is_void()) {
    throw "CANNOT GENERATE SERIALIZE CODE FOR void TYPE: " + name;
  }



  if (type->is_struct() || type->is_xception()) {
    generate_serialize_struct(out,
                              (t_struct*)type,
                              oprot,
                              name,
                              is_reference(tfield));
  } else if (type->is_container()) {
    generate_serialize_container(out, type, oprot, name);
  } else if (type->is_base_type() || type->is_enum()) {

    indent(out) <<
      "xfer += "<< oprot << "->";

    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw
          "compiler error: cannot serialize void field in a struct: " + name;
        break;
      case t_base_type::TYPE_STRING:
        if (((t_base_type*)type)->is_binary()) {
          out << "writeBinary(" << name << ");";
        }
        else {
          out << "writeString(" << name << ");";
        }
        break;
      case t_base_type::TYPE_BOOL:
        out << "writeBool(" << name << ");";
        break;
      case t_base_type::TYPE_BYTE:
        out << "writeByte(static_cast<int8_t>(" << name << "));";
        break;
      case t_base_type::TYPE_I16:
        out << "writeI16(static_cast<int16_t>(" << name << "));";
        break;
      case t_base_type::TYPE_I32:
        out << "writeI32(static_cast<int32_t>(" << name << "));";
        break;
      case t_base_type::TYPE_I64:
        out << "writeI64(static_cast<int64_t>(" << name << "));";
        break;
      case t_base_type::TYPE_DOUBLE:
        out << "writeDouble(" << name << ");";
        break;
      default:
        throw "compiler error: no C++ writer for base type " + t_base_type::t_base_name(tbase) + name;
      }
    } else if (type->is_enum()) {
      out << "writeI32(static_cast<int32_t>(" << name << "));";
    }
    out << endl;
  } else {
    printf("DO NOT KNOW HOW TO SERIALIZE FIELD '%s' TYPE '%s'\n",
           name.c_str(),
           type_name(type).c_str());
  }
}

/**
 * Serializes all the members of a struct.
 *
 * @param tstruct The struct to serialize
 * @param prefix  String prefix to attach to all fields
 */
void t_cpp_generator::generate_serialize_struct(ofstream& out,
                                                t_struct* tstruct,
                                                string oprot,
                                                string prefix,
                                                bool pointer) {
  if (pointer) {
    indent(out) << "if (" << prefix << ") {" << endl;
    indent(out) << indent() << "xfer += " << prefix << "->write("<< oprot << "); " << endl;
    indent(out)  << "} else {" << oprot << "->writeStructBegin(\"" <<
      tstruct->get_name() << "\"); " << endl;
    indent(out) << indent() << oprot << "->writeStructEnd();" << endl;
    indent(out) << indent() << oprot <<  "->writeFieldStop();" << endl;
    indent(out) << "}" << endl;
  } else {
    indent(out) <<
      "xfer += " << prefix << ".write(" << oprot << ");" << endl;
  }
}

void t_cpp_generator::generate_serialize_container(ofstream& out,
                                                   t_type* ttype,
                                                   string oprot,
                                                   string prefix) {
  scope_up(out);

  if (ttype->is_map()) {
    indent(out) <<
      "xfer += " << oprot << "->writeMapBegin(" <<
      type_to_enum(((t_map*)ttype)->get_key_type()) << ", " <<
      type_to_enum(((t_map*)ttype)->get_val_type()) << ", " <<
      "static_cast<uint32_t>(" << prefix << ".size()));" << endl;
  } else if (ttype->is_set()) {
    indent(out) <<
      "xfer += " << oprot << "->writeSetBegin(" <<
      type_to_enum(((t_set*)ttype)->get_elem_type()) << ", " <<
      "static_cast<uint32_t>(" << prefix << ".size()));" << endl;
  } else if (ttype->is_list()) {
    indent(out) <<
      "xfer += " << oprot << "->writeListBegin(" <<
      type_to_enum(((t_list*)ttype)->get_elem_type()) << ", " <<
      "static_cast<uint32_t>(" << prefix << ".size()));" << endl;
  }

  string iter = tmp("_iter");
  out <<
    indent() << type_name(ttype) << "::const_iterator " << iter << ";" << endl <<
    indent() << "for (" << iter << " = " << prefix  << ".begin(); " << iter << " != " << prefix << ".end(); ++" << iter << ")" << endl;
  scope_up(out);
    if (ttype->is_map()) {
      generate_serialize_map_element(out, (t_map*)ttype, iter, oprot);
    } else if (ttype->is_set()) {
      generate_serialize_set_element(out, (t_set*)ttype, iter, oprot);
    } else if (ttype->is_list()) {
      generate_serialize_list_element(out, (t_list*)ttype, iter, oprot);
    }
  scope_down(out);

  if (ttype->is_map()) {
    indent(out) <<
      "xfer += " << oprot << "->writeMapEnd();" << endl;
  } else if (ttype->is_set()) {
    indent(out) <<
      "xfer += " << oprot << "->writeSetEnd();" << endl;
  } else if (ttype->is_list()) {
    indent(out) <<
      "xfer += " << oprot << "->writeListEnd();" << endl;
  }

  scope_down(out);
}

/**
 * Serializes the members of a map.
 *
 */
void t_cpp_generator::generate_serialize_map_element(ofstream& out,
                                                     t_map* tmap,
                                                     string iter,
                                                     string oprot) {
  t_field kfield(tmap->get_key_type(), iter + "->first");
  generate_serialize_field(out, &kfield, oprot, "");

  t_field vfield(tmap->get_val_type(), iter + "->second");
  generate_serialize_field(out, &vfield, oprot, "");
}

/**
 * Serializes the members of a set.
 */
void t_cpp_generator::generate_serialize_set_element(ofstream& out,
                                                     t_set* tset,
                                                     string iter,
                                                     string oprot) {
  t_field efield(tset->get_elem_type(), "(*" + iter + ")");
  generate_serialize_field(out, &efield, oprot, "");
}

/**
 * Serializes the members of a list.
 */
void t_cpp_generator::generate_serialize_list_element(ofstream& out,
                                                      t_list* tlist,
                                                      string iter,
                                                      string oprot) {
  t_field efield(tlist->get_elem_type(), "(*" + iter + ")");
  generate_serialize_field(out, &efield, oprot, "");
}

/**
 * Makes a :: prefix for a namespace
 *
 * @param ns The namespace, w/ periods in it
 * @return Namespaces
 */
string t_cpp_generator::namespace_prefix(string ns) {
  // Always start with "::", to avoid possible name collisions with
  // other names in one of the current namespaces.
  //
  // We also need a leading space, in case the name is used inside of a
  // template parameter.  "MyTemplate<::foo::Bar>" is not valid C++,
  // since "<:" is an alternative token for "[".
  string result = " ::";

  if (ns.size() == 0) {
    return result;
  }
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += ns.substr(0, loc);
    result += "::";
    ns = ns.substr(loc+1);
  }
  if (ns.size() > 0) {
    result += ns + "::";
  }
  return result;
}

/**
 * Opens namespace.
 *
 * @param ns The namespace, w/ periods in it
 * @return Namespaces
 */
string t_cpp_generator::namespace_open(string ns) {
  if (ns.size() == 0) {
    return "";
  }
  string result = "";
  string separator = "";
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += separator;
    result += "namespace ";
    result += ns.substr(0, loc);
    result += " {";
    separator = " ";
    ns = ns.substr(loc+1);
  }
  if (ns.size() > 0) {
    result += separator + "namespace " + ns + " {";
  }
  return result;
}

/**
 * Closes namespace.
 *
 * @param ns The namespace, w/ periods in it
 * @return Namespaces
 */
string t_cpp_generator::namespace_close(string ns) {
  if (ns.size() == 0) {
    return "";
  }
  string result = "}";
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += "}";
    ns = ns.substr(loc+1);
  }
  result += " // namespace " + ns;
  return result;
}

/**
 * Returns a C++ type name
 *
 * @param ttype The type
 * @return String of the type name, i.e. std::set<type>
 */
string t_cpp_generator::type_name(t_type* ttype, bool in_typedef, bool arg) {
  if (ttype->is_base_type()) {
    string bname = base_type_name(((t_base_type*)ttype)->get_base());
    std::map<string, string>::iterator it = ttype->annotations_.find("cpp.type");
    if (it != ttype->annotations_.end()) {
      bname = it->second;
    }

    if (bname.find("int") == 0) {
      it = ttype->annotations_.find("u");
      if (it != ttype->annotations_.end()) {
          bname = "u" + bname;
      }
    }

    if (!arg) {
      return bname;
    }

    if (((t_base_type*)ttype)->get_base() == t_base_type::TYPE_STRING) {
      return "const " + bname + "&";
    } else {
      return "const " + bname;
    }
  }

  // Check for a custom overloaded C++ name
  if (ttype->is_container()) {
    string cname;

    t_container* tcontainer = (t_container*) ttype;
    if (tcontainer->has_cpp_name()) {
      cname = tcontainer->get_cpp_name();
    } else if (ttype->is_map()) {
      t_map* tmap = (t_map*) ttype;
      cname = "std::map<" +
        type_name(tmap->get_key_type(), in_typedef) + ", " +
        type_name(tmap->get_val_type(), in_typedef) + "> ";
    } else if (ttype->is_set()) {
      t_set* tset = (t_set*) ttype;
      cname = "std::set<" + type_name(tset->get_elem_type(), in_typedef) + "> ";
    } else if (ttype->is_list()) {
      t_list* tlist = (t_list*) ttype;
      cname = "std::vector<" + type_name(tlist->get_elem_type(), in_typedef) + "> ";
    }

    if (arg) {
      return "const " + cname + "&";
    } else {
      return cname;
    }
  }

  string class_prefix;
  if (in_typedef && (ttype->is_struct() || ttype->is_xception())) {
    class_prefix = "class ";
  }

  // Check if it needs to be namespaced
  string pname;
  t_program* program = ttype->get_program();
  if (program != NULL && program != program_) {
    pname =
      class_prefix +
      namespace_prefix(program->get_namespace("cpp")) +
      ttype->get_name();
  } else {
    pname = class_prefix + ttype->get_name();
  }

  if (ttype->is_enum() && !gen_pure_enums_) {
    pname += "::type";
  }

  if (arg) {
    if (is_complex_type(ttype)) {
      return "const " + pname + "&";
    } else {
      return "const " + pname;
    }
  } else {
    return pname;
  }
}

/**
 * Returns the C++ type that corresponds to the thrift type.
 *
 * @param tbase The base type
 * @return Explicit C++ type, i.e. "int32_t"
 */
string t_cpp_generator::base_type_name(t_base_type::t_base tbase) {
  switch (tbase) {
  case t_base_type::TYPE_VOID:
    return "void";
  case t_base_type::TYPE_STRING:
    return "std::string";
  case t_base_type::TYPE_BOOL:
    return "bool";
  case t_base_type::TYPE_BYTE:
    return "int8_t";
  case t_base_type::TYPE_I16:
    return "int16_t";
  case t_base_type::TYPE_I32:
    return "int32_t";
  case t_base_type::TYPE_I64:
    return "int64_t";
  case t_base_type::TYPE_DOUBLE:
    return "double";
  default:
    throw "compiler error: no C++ base type name for base type " + t_base_type::t_base_name(tbase);
  }
}

/**
 * Declares a field, which may include initialization as necessary.
 *
 * @param ttype The type
 * @return Field declaration, i.e. int x = 0;
 */
string t_cpp_generator::declare_field(t_field* tfield, bool init, bool pointer, bool constant, bool reference, std::string alias) {
  // TODO(mcslee): do we ever need to initialize the field?
  string result = "";
  if (constant) {
    result += "const ";
  }
  result += type_name(tfield->get_type());
  if (is_reference(tfield)) {
    result = "cxx::shared_ptr<" + result + ">";
  }
  if (pointer) {
    result += "*";
  }
  if (reference) {
    result += "&";
  }
  result += " " + (alias.empty() ? tfield->get_name() : alias);
  if (init) {
    t_type* type = get_true_type(tfield->get_type());

    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      switch (tbase) {
      case t_base_type::TYPE_VOID:
      case t_base_type::TYPE_STRING:
        break;
      case t_base_type::TYPE_BOOL:
        result += " = false";
        break;
      case t_base_type::TYPE_BYTE:
      case t_base_type::TYPE_I16:
      case t_base_type::TYPE_I32:
      case t_base_type::TYPE_I64:
        result += " = 0";
        break;
      case t_base_type::TYPE_DOUBLE:
        result += " = (double)0";
        break;
      default:
        throw "compiler error: no C++ initializer for base type " + t_base_type::t_base_name(tbase);
      }
    } else if (type->is_enum()) {
      result += " = (" + type_name(type) + ")0";
    }
  }
  if (!reference) {
    result += ";";
  }
  return result;
}

/**
 * Renders a function signature of the form 'type name(args)'
 *
 * @param tfunction Function definition
 * @return String of rendered function definition
 */
string t_cpp_generator::function_signature(t_function* tfunction,
                                           string style,
                                           string prefix,
                                           bool name_params,
                                           std::string preargs) {
  // TODO:
  t_type* ttype = tfunction->get_returntype();
  t_struct* arglist = tfunction->get_arglist();
  bool has_xceptions = !tfunction->get_xceptions()->get_members().empty();

  if (style == "") {
    if (is_complex_type(ttype)) {
      return
        "void " + prefix + tfunction->get_name() +
        "(" + preargs + ", " + type_name(ttype) + (name_params ? "& response" : "& /* response */") +
        argument_list(arglist, name_params, true) + ")";
    } else {
      std::string args = argument_list(arglist, name_params);
      if (!args.empty() && !preargs.empty()) { preargs.append(", "); }
      return
        type_name(ttype) + " " + prefix + tfunction->get_name() +
        "(" + preargs + args + ")";
    }
  } else if (style.substr(0,3) == "Cob") {
    string cob_type;
    string exn_cob;
    if (style == "CobCl") {
      /*
      cob_type = "(" + service_name_ + "CobClient";
      if (gen_templates_) {
        cob_type += "T<Protocol_>";
      }
      cob_type += "* client)";
      */
      if (!ttype->is_void()) {
        if (is_complex_type(ttype)) {
          cob_type = "(int, " + type_name(ttype) + "&)";
        } else {
          cob_type = "(int, " + type_name(ttype) + ")";
        }
      } else {
        cob_type = "(int)";
      }
      //if (!ttype->is_void()) {
      //  exn_cob = ", " + type_name(ttype) + (name_params ? "& response" : "& /* response */");
      //}
      if (!preargs.empty()) { preargs.append(", "); }
    } else if (style =="CobSv") {
      cob_type = (ttype->is_void()
                  ? "()"
                  : ("(" + type_name(ttype) + " const& response)"));
      if (has_xceptions) {
        exn_cob = ", cxx::function<void(::pebble::rpc::TDelayedException* _throw)> /* exn_cob */";
      }
    } else {
      throw "UNKNOWN STYLE";
    }

    return
      "void " + prefix + tfunction->get_name() +
      "(" + preargs + "cxx::function<void" + cob_type + "> cob" + exn_cob +
      argument_list(arglist, name_params, true) + ")";
  } else {
    throw "UNKNOWN STYLE";
  }
}

std::string t_cpp_generator::function_signature_if(t_function* tfunction,
                                           string style,
                                           string prefix,
                                           bool name_params) {
  t_type* ttype     = tfunction->get_returntype();
  t_struct* arglist = tfunction->get_arglist();

  std::string args = argument_list(arglist, name_params, false);
  std::string ret_sync;
  std::string ret_parallel;
  std::string ret_async = "const cxx::function<void(int ret_code)>& cb";;
  std::string ret_server;

  std::string type_ref;
  std::string type_const;
  if (is_complex_type(ttype)) {
      type_ref   = "&";
      type_const = "const ";
  }

  if (!ttype->is_void()) {
    ret_sync   = type_name(ttype) + "* response";
    ret_parallel = type_name(ttype) + "* response, ";
    ret_async  = "const cxx::function<void(int32_t ret_code, " + type_const + type_name(ttype) + type_ref + " response)>& cb";
  }

  if (!tfunction->is_oneway()) {
    if (!ttype->is_void()) {
      ret_server = "cxx::function<void(int32_t ret_code, " + type_const + type_name(ttype) + type_ref + " response)>& rsp";
    } else {
      ret_server = "cxx::function<void(int32_t ret_code)>& rsp";
    }
  }

  // sync
  if (style == "") {
    if (!args.empty() && !ret_sync.empty()) {
        args += ", ";
    }
    return "int32_t " + prefix + tfunction->get_name() + "(" + args + ret_sync + ")";
  }

  // parallel
  if (style == "Parallel") {
      if (!args.empty()) {
          args += ", ";
      }
      return "int32_t " + prefix + tfunction->get_name() + "(" + args + "int32_t* ret_code, " + ret_parallel + "uint32_t* num_called, uint32_t* num_parallel)";
  }

  // async
  if (style == "CobCl") {
    if (!args.empty() && !ret_async.empty()) {
        args += ", ";
    }
    return "void " + prefix + tfunction->get_name() + "(" + args + ret_async + ")";
  }

  // service interface
  if (style =="CobSv") {
    if (!args.empty() && !ret_server.empty()) {
        args += ", ";
    }
    return "void " + prefix + tfunction->get_name() + "(" + args + ret_server + ")";
  }

  return "UNKNOWN STYLE";
}

/**
 * Renders a field list
 *
 * @param tstruct The struct definition
 * @return Comma sepearated list of all field names in that struct
 */
string t_cpp_generator::argument_list(t_struct* tstruct, bool name_params, bool start_comma) {
  string result = "";

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  bool first = !start_comma;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      result += ", ";
    }
    result += type_name((*f_iter)->get_type(), false, true) + " " +
      (name_params ? (*f_iter)->get_name() : "/* " + (*f_iter)->get_name() + " */");
  }
  return result;
}

/**
 * Converts the parse type to a C++ enum string for the given type.
 *
 * @param type Thrift Type
 * @return String of C++ code to definition of that type constant
 */
string t_cpp_generator::type_to_enum(t_type* type) {
  type = get_true_type(type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "NO T_VOID CONSTRUCT";
    case t_base_type::TYPE_STRING:
      return "::pebble::dr::protocol::T_STRING";
    case t_base_type::TYPE_BOOL:
      return "::pebble::dr::protocol::T_BOOL";
    case t_base_type::TYPE_BYTE:
      return "::pebble::dr::protocol::T_BYTE";
    case t_base_type::TYPE_I16:
      return "::pebble::dr::protocol::T_I16";
    case t_base_type::TYPE_I32:
      return "::pebble::dr::protocol::T_I32";
    case t_base_type::TYPE_I64:
      return "::pebble::dr::protocol::T_I64";
    case t_base_type::TYPE_DOUBLE:
      return "::pebble::dr::protocol::T_DOUBLE";
    }
  } else if (type->is_enum()) {
    return "::pebble::dr::protocol::T_I32";
  } else if (type->is_struct()) {
    return "::pebble::dr::protocol::T_STRUCT";
  } else if (type->is_xception()) {
    return "::pebble::dr::protocol::T_STRUCT";
  } else if (type->is_map()) {
    return "::pebble::dr::protocol::T_MAP";
  } else if (type->is_set()) {
    return "::pebble::dr::protocol::T_SET";
  } else if (type->is_list()) {
    return "::pebble::dr::protocol::T_LIST";
  }

  throw "INVALID TYPE IN type_to_enum: " + type->get_name();
}

/**
 * Returns the symbol name of the local reflection of a type.
 */
string t_cpp_generator::local_reflection_name(const char* prefix, t_type* ttype, bool external) {
  ttype = get_true_type(ttype);

  // We have to use the program name as part of the identifier because
  // if two thrift "programs" are compiled into one actual program
  // you would get a symbol collision if they both defined list<i32>.
  // trlo = Thrift Reflection LOcal.
  string prog;
  string name;
  string nspace;

  // TODO(dreiss): Would it be better to pregenerate the base types
  //               and put them in Thrift.{h,cpp} ?

  if (ttype->is_base_type()) {
    prog = program_->get_name();
    name = ttype->get_ascii_fingerprint();
  } else if (ttype->is_enum()) {
    assert(ttype->get_program() != NULL);
    prog = ttype->get_program()->get_name();
    name = ttype->get_ascii_fingerprint();
  } else if (ttype->is_container()) {
    prog = program_->get_name();
    name = ttype->get_ascii_fingerprint();
  } else {
    assert(ttype->is_struct() || ttype->is_xception());
    assert(ttype->get_program() != NULL);
    prog = ttype->get_program()->get_name();
    name = ttype->get_ascii_fingerprint();
  }

  if (external &&
      ttype->get_program() != NULL &&
      ttype->get_program() != program_) {
    nspace = namespace_prefix(ttype->get_program()->get_namespace("cpp"));
  }

  return nspace + "trlo_" + prefix + "_" + prog + "_" + name;
}

string t_cpp_generator::get_include_prefix(const t_program& program) const {
  string include_prefix = program.get_include_prefix();
  if (!use_include_prefix_ ||
      (include_prefix.size() > 0 && include_prefix[0] == '/')) {
    // if flag is turned off or this is absolute path, return empty prefix
    return "";
  }

  string::size_type last_slash = string::npos;
  if ((last_slash = include_prefix.rfind("/")) != string::npos) {
    return include_prefix.substr(0, last_slash) +
      (get_program()->is_out_path_absolute() ? "/" : "/" + out_dir_base_ + "/");

  }

  return "";
}

// default open cob_style and pure_enums.
THRIFT_REGISTER_GENERATOR(cpp, "C++ for server",
/*"    cob_style:       Generate \"Continuation OBject\"-style classes.\n"
"    no_client_completion:\n"
"                     Omit calls to completion__() in CobClient class.\n"
"    no_default_operators:\n"
"                     Omits generation of default operators ==, != and <\n"
"    templates:       Generate templatized reader/writer methods.\n"
"    pure_enums:      Generate pure enums instead of wrapper classes.\n"
"    dense:           Generate type specifications for the dense protocol.\n"
"    include_prefix:  Use full include paths in generated files.\n"
"    client:       Generate code for terminal(default generate for server).\n"
*/ "\n"
)

