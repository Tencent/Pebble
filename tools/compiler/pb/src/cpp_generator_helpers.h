

#ifndef _PEBBLE_COMPILER_PB_CPP_GENERATOR_HELPERS_H_
#define _PEBBLE_COMPILER_PB_CPP_GENERATOR_HELPERS_H_

#include <map>
#include <sstream>

#include "common/string_utility.h"
#include "google/protobuf/descriptor.h"


namespace pebble {

inline std::string StripProto(std::string filename) {
    if (!StringUtility::StripSuffix(&filename, ".protodevel")) {
        StringUtility::StripSuffix(&filename, ".proto");
    }
    return filename;
}

inline std::string DotsToColons(const std::string &name) {
    std::string tmp(name);
    StringUtility::string_replace(".", "::", &tmp);
    return tmp;
}

inline std::string DotsToUnderscores(const std::string &name) {
    std::string tmp(name);
    StringUtility::string_replace(".", "_", &tmp);
    return tmp;
}

inline std::string ClassName(const google::protobuf::Descriptor *descriptor, bool qualified) {
  // Find "outer", the descriptor of the top-level message in which "descriptor" is embedded.
    const google::protobuf::Descriptor *outer = descriptor;
    while (outer->containing_type() != NULL) outer = outer->containing_type();

    const std::string &outer_name = outer->full_name();
    std::string inner_name = descriptor->full_name().substr(outer_name.size());

    if (qualified) {
        return "::" + DotsToColons(outer_name) + DotsToUnderscores(inner_name);
    }

    return outer->name() + DotsToUnderscores(inner_name);
}

// Get leading or trailing comments in a string. Comment lines start with "// ".
// Leading detached comments are put in in front of leading comments.
template <typename DescriptorType>
inline std::string GetCppComments(const DescriptorType *desc, bool leading) {
    google::protobuf::SourceLocation location;
    if (!desc->GetSourceLocation(&location)) {
        return "";
    }
    std::vector<std::string> comments;
    if (leading) {
        StringUtility::Split(location.leading_comments, "\n", &comments);
    } else {
        StringUtility::Split(location.trailing_comments, "\n", &comments);
    }
    std::ostringstream oss;
    for (std::vector<std::string>::iterator it = comments.begin(); it != comments.end(); ++it) {
        if (it != comments.begin()) {
            oss << "\n";
        }
        oss << "//" << *it;
    }
    return oss.str();
}

}  // namespace pebble

#endif  // _PEBBLE_COMPILER_PB_CPP_GENERATOR_HELPERS_H_