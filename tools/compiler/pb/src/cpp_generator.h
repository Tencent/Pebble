

#ifndef _PEBBLE_COMPILER_PB_CPP_GENERATOR_H
#define _PEBBLE_COMPILER_PB_CPP_GENERATOR_H

// cpp_generator.h/.cc do not directly depend on GRPC/ProtoBuf, such that they
// can be used to generate code for other serialization systems, such as
// FlatBuffers.

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "common/platform.h"

namespace pebble {

// Contains all the parameters that are parsed from the command line.
class Parameters {
public:
    // Puts the service into a namespace
    std::string services_namespace;
    // Use system includes (<>) or local includes ("")
    bool use_system_headers;
    // Prefix to any grpc include
    std::string search_path;
};

// A common interface for objects having comments in the source.
// Return formatted comments to be inserted in generated code.
class CommentHolder {
public:
    virtual ~CommentHolder() {}
    virtual std::string GetLeadingComments() const = 0;
    virtual std::string GetTrailingComments() const = 0;
};

// An abstract interface representing a method.
class Method : public CommentHolder {
public:
    virtual ~Method() {}

    virtual std::string name() const = 0;

    virtual std::string input_type_name() const = 0;
    virtual std::string output_type_name() const = 0;

    virtual bool NoStreaming() const = 0;
    virtual bool ClientOnlyStreaming() const = 0;
    virtual bool ServerOnlyStreaming() const = 0;
    virtual bool BidiStreaming() const = 0;

    virtual std::string timeout_ms() const = 0;
};

// An abstract interface representing a service.
class Service : public CommentHolder {
public:
  virtual ~Service() {}

  virtual std::string name() const = 0;

  virtual int method_count() const = 0;
  virtual cxx::shared_ptr<const Method> method(int i) const = 0;
};

class Printer {
public:
    virtual ~Printer() {}

    virtual void Print(const std::map<std::string, std::string>& vars,
                     const char* template_string) = 0;
    virtual void Print(const char* string) = 0;
    virtual void Indent() = 0;
    virtual void Outdent() = 0;
};

// An interface that allows the source generated to be output using various
// libraries/idls/serializers.
class File : public CommentHolder {
public:
    virtual ~File() {}

    virtual std::string filename() const = 0;
    virtual std::string filename_without_ext() const = 0;
    virtual std::string filename_without_path() const = 0;
    virtual std::string message_header_ext() const = 0;
    virtual std::string service_header_ext() const = 0;
    virtual std::string service_imp_header_ext() const = 0;
    virtual std::string package() const = 0;
    virtual std::vector<std::string> package_parts() const = 0;
    virtual std::string additional_headers() const = 0;

    virtual int service_count() const = 0;
    virtual cxx::shared_ptr<const Service> service(int i) const = 0;

    virtual cxx::shared_ptr<Printer> CreatePrinter(std::string* str) const = 0;
};

// 生成头文件首部
std::string GetHeaderPrologue(File* file, const Parameters& params, const std::string& inh);

// 生成头文件include部分 + 引用声明
std::string GetHeaderIncludes(File* file, const Parameters& params);

// 生成头文件include部分 + 引用声明
std::string GetINHeaderIncludes(File* file, const Parameters& params);

// 生成源文件include部分
std::string GetSourceIncludes(File* file, const Parameters& params);

// 生成头文件尾部
std::string GetHeaderEpilogue(File* file, const Parameters& params, const std::string& inh);

// 生成源文件首部
std::string GetSourcePrologue(File* file, const Parameters& params);

// 获取服务定义头文件内容
std::string GetHeaderServices(File* file, const Parameters& params);

// 获取服务内部定义头文件内容
std::string GetINHeaderService(File* file, const Parameters& params);

// Return the services for generated source file.
std::string GetSourceServices(File* file, const Parameters& params);

// Return the epilogue of the generated source file.
std::string GetSourceEpilogue(File* file, const Parameters& params);

} // namespace pebble

#endif  // _PEBBLE_COMPILER_PB_CPP_GENERATOR_H