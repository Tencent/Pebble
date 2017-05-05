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

#include <memory>
#include <sstream>

#include "cpp_generator.h"
#include "cpp_generator_helpers.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"


namespace pebble {

class ProtoBufMethod : public Method {
public:
    ProtoBufMethod(const google::protobuf::MethodDescriptor* method) : method_(method) {}

    std::string name() const { return method_->name(); }

    std::string input_type_name() const {
        return ClassName(method_->input_type(), true);
    }
    std::string output_type_name() const {
        return ClassName(method_->output_type(), true);
    }

    // PB v3 支持4种请求响应模型: 一对一、一对多、多对一、多对多
    bool NoStreaming() const { // 一对一
        // return !method_->client_streaming() && !method_->server_streaming();
        return true; // default
    }

    bool ClientOnlyStreaming() const { // 多对一
        // return method_->client_streaming() && !method_->server_streaming();
        return false; // pb 2.x unsupport
    }

    bool ServerOnlyStreaming() const { // 一对多
        // return !method_->client_streaming() && method_->server_streaming();
        return false; // pb 2.x unsupport
    }

    bool BidiStreaming() const { // 多对多
        // return method_->client_streaming() && method_->server_streaming();
        return false; // pb 2.x unsupport
    }

    std::string GetLeadingComments() const {
        return GetCppComments(method_, true);
    }

    std::string GetTrailingComments() const {
        return GetCppComments(method_, false);
    }

    std::string timeout_ms() const {
        // 使用扩展google.protobuf.MethodOptions的方式使用起来比较麻烦
        // 增加用户使用成本，直接在client增加设置超时接口
        return "-1";
    }

private:
    const google::protobuf::MethodDescriptor* method_;
};

class ProtoBufService : public Service {
public:
    ProtoBufService(const google::protobuf::ServiceDescriptor* service) : service_(service) {}

    std::string name() const { return service_->name(); }

    int method_count() const { return service_->method_count(); }

    cxx::shared_ptr<const Method> method(int i) const {
        return cxx::shared_ptr<const Method>(new ProtoBufMethod(service_->method(i)));
    }

    std::string GetLeadingComments() const {
        return GetCppComments(service_, true);
    }

    std::string GetTrailingComments() const {
        return GetCppComments(service_, false);
    }

private:
    const google::protobuf::ServiceDescriptor* service_;
};

class ProtoBufPrinter : public Printer {
public:
    ProtoBufPrinter(std::string* str)
        : output_stream_(str), printer_(&output_stream_, '$') {}

    void Print(const std::map<std::string, std::string>& vars, const char* string_template) {
        printer_.Print(vars, string_template);
    }

    void Print(const char* string) { printer_.Print(string); }
    void Indent() { printer_.Indent(); printer_.Indent(); }
    void Outdent() { printer_.Outdent(); printer_.Outdent(); }

private:
    google::protobuf::io::StringOutputStream output_stream_;
    google::protobuf::io::Printer printer_;
};

class ProtoBufFile : public File {
public:
    ProtoBufFile(const google::protobuf::FileDescriptor* file) : file_(file) {}

    std::string filename() const { return file_->name(); }
    std::string filename_without_ext() const {
        return StripProto(filename());
    }
    std::string filename_without_path() const {
        std::string fn(filename());
        size_t pos = fn.find_last_of('/');
        if (pos > 0) {
            fn = fn.substr(pos + 1);
        }
        return StripProto(fn);
    }

    std::string message_header_ext() const { return ".pb.h"; }
    std::string service_header_ext() const { return ".rpc.pb.h"; }
    std::string service_imp_header_ext() const { return ".rpc.pb.inh"; }

    std::string package() const { return file_->package(); }
    std::vector<std::string> package_parts() const {
        std::vector<std::string> tmp;
        StringUtility::Split(package(), ".", &tmp);
        return tmp;
    }

    std::string additional_headers() const { return ""; }

    int service_count() const { return file_->service_count(); };
    cxx::shared_ptr<const Service> service(int i) const {
        return cxx::shared_ptr<const Service>(new ProtoBufService(file_->service(i)));
    }

    cxx::shared_ptr<Printer> CreatePrinter(std::string *str) const {
        return cxx::shared_ptr<Printer>(new ProtoBufPrinter(str));
    }

    std::string GetLeadingComments() const {
        return "";
    }

    std::string GetTrailingComments() const {
        return "";
    }

private:
      const google::protobuf::FileDescriptor *file_;
};

class CppServiceGenerator : public google::protobuf::compiler::CodeGenerator {
public:
    CppServiceGenerator() {}
    virtual ~CppServiceGenerator() {}

    virtual bool Generate(const google::protobuf::FileDescriptor* file,
                            const std::string& parameter,
                            google::protobuf::compiler::GeneratorContext* context,
                            std::string* error) const {
        if (file->options().cc_generic_services()) {
            *error = "cpp pebble proto compiler plugin does not work with generic service."
                    "To generate pebble service, please set \"cc_generic_service = false\".";
            return false;
        }

        Parameters generator_parameters;
        generator_parameters.use_system_headers = false;

        ProtoBufFile pbfile(file);

        if (!parameter.empty()) {
            std::vector<std::string> parameters_list;
            StringUtility::Split(parameter, ",", &parameters_list);

            for (std::vector<std::string>::iterator it = parameters_list.begin();
                it != parameters_list.end(); it++) {

                std::vector<std::string> param;
                StringUtility::Split(*it, "=", &param);

                if (param[0] == "services_namespace") {
                    generator_parameters.services_namespace = param[1];
                } else if (param[0] == "use_system_headers") {
                    if (param[1] == "true") {
                        generator_parameters.use_system_headers = true;
                    } else if (param[1] == "false") {
                        generator_parameters.use_system_headers = false;
                    } else {
                        *error = std::string("Invalid parameter: ") + *it;
                        return false;
                    }
                } else if (param[0] == "search_path") {
                    generator_parameters.search_path = param[1];
                } else {
                    *error = std::string("Unknown parameter: ") + *it;
                    return false;
                }
            }
        }

        std::string file_name = StripProto(file->name());

        std::string inheader_code =
            GetHeaderPrologue(&pbfile, generator_parameters, "inh") +
            GetINHeaderIncludes(&pbfile, generator_parameters) +
            GetINHeaderService(&pbfile, generator_parameters) +
            GetHeaderEpilogue(&pbfile, generator_parameters, "inh");
        cxx::shared_ptr<google::protobuf::io::ZeroCopyOutputStream> inheader_output(
            context->Open(file_name + ".rpc.pb.inh"));
        google::protobuf::io::CodedOutputStream inheader_coded_out(inheader_output.get());
        inheader_coded_out.WriteRaw(inheader_code.data(), inheader_code.size());

        std::string header_code =
            GetHeaderPrologue(&pbfile, generator_parameters, "h") +
            GetHeaderIncludes(&pbfile, generator_parameters) +
            GetHeaderServices(&pbfile, generator_parameters) +
            GetHeaderEpilogue(&pbfile, generator_parameters, "h");
        cxx::shared_ptr<google::protobuf::io::ZeroCopyOutputStream> header_output(
            context->Open(file_name + ".rpc.pb.h"));
        google::protobuf::io::CodedOutputStream header_coded_out(header_output.get());
        header_coded_out.WriteRaw(header_code.data(), header_code.size());

        std::string source_code =
            GetSourcePrologue(&pbfile, generator_parameters) +
            GetSourceIncludes(&pbfile, generator_parameters) +
            GetSourceServices(&pbfile, generator_parameters) +
            GetSourceEpilogue(&pbfile, generator_parameters);
        cxx::shared_ptr<google::protobuf::io::ZeroCopyOutputStream> source_output(
            context->Open(file_name + ".rpc.pb.cc"));
        google::protobuf::io::CodedOutputStream source_coded_out(source_output.get());
        source_coded_out.WriteRaw(source_code.data(), source_code.size());

        return true;
    }

private:
    // Insert the given code into the given file at the given insertion point.
    void Insert(google::protobuf::compiler::GeneratorContext* context,
                const std::string& filename, const std::string& insertion_point,
                const std::string& code) const {
        cxx::shared_ptr<google::protobuf::io::ZeroCopyOutputStream> output(
            context->OpenForInsert(filename, insertion_point));
        google::protobuf::io::CodedOutputStream coded_out(output.get());
        coded_out.WriteRaw(code.data(), code.size());
    }
};

} // namespace pebble

int main(int argc, char *argv[]) {
    pebble::CppServiceGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}

