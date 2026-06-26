// ==========================================================================================================================================================================================================================================═
// Source Manager — Implementation
// ==========================================================================================================================================================================================================================================═

#include "source_manager.h"
#include "call_stack.h"

#include <fstream>
#include <sstream>
#include <utility>

namespace pml {

namespace {

/// Split `text` into lines, preserving empty lines.
std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

}  // anonymous namespace

bool SourceManager::load_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    load_source(filename, ss.str());
    return true;
}

void SourceManager::load_source(
    const std::string& filename, const std::string& source) {
    file_cache_[filename] = split_lines(source);
}

bool SourceManager::ensure_loaded(const std::string& filename) {
    if (is_loaded(filename)) {
        return true;
    }
    return load_file(filename);
}

std::string SourceManager::get_line(
    const std::string& filename, int line) const {
    auto it = file_cache_.find(filename);
    if (it == file_cache_.end()) {
        return {};
    }
    const auto& lines = it->second;
    if (line < 1 || static_cast<size_t>(line) > lines.size()) {
        return {};
    }
    return lines[line - 1];
}

bool SourceManager::is_loaded(const std::string& filename) const noexcept {
    return file_cache_.find(filename) != file_cache_.end();
}

}  // namespace pml

// ==========================================================================================================================================================================================================================================═
// Error formatting with source context
// ==========================================================================================================================================================================================================================================═

namespace pml {

std::string format_error_with_source(
    const PMLException& err, SourceManager& src) {
    std::string result = "Error: ";
    result += err.what();

    if (err.location.has_value()) {
        const auto& loc = *err.location;
        if (loc.line > 0) {
            const std::string& filename = loc.filename.empty()
                                              ? std::string("<stdin>")
                                              : loc.filename;
            if (!src.is_loaded(filename) && !loc.filename.empty()) {
                src.ensure_loaded(loc.filename);
            }
            std::string line_text = src.get_line(filename, loc.line);
            if (!line_text.empty()) {
                std::string line_num_str = std::to_string(loc.line);
                result += "\n\n  ";
                result += line_num_str;
                result += " | ";
                result += line_text;
                result += '\n';

                // Caret padding: "  line | " is 4 chars + line number width
                std::string caret_padding(4 + line_num_str.length(), ' ');
                int col = loc.column > 0 ? loc.column : 1;
                caret_padding.append(col - 1, ' ');
                result += caret_padding;
                result += "^\n";
            }
        }
    }

    if (err.repair_hint.has_value() && !err.repair_hint->empty()) {
        result += "  Hint: ";
        result += *err.repair_hint;
        result += '\n';
    }

    result += format_call_stack(err.call_stack);

    return result;
}

}  // namespace pml
