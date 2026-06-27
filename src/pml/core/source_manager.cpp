// ==========================================================================================================================================================================================================================================═
// Source Manager — Implementation
// ==========================================================================================================================================================================================================================================═

#include "source_manager.h"
#include "call_stack.h"

#include "pml/api/context.h"

#include <algorithm>
#include <format>
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

SourceManager& get_global_source_manager() {
    if (auto* ctx = PMLContext::current_ptr()) {
        if (ctx->source_manager) {
            return *ctx->source_manager;
        }
    }
    static SourceManager instance;
    return instance;
}

}  // namespace pml

// ==========================================================================================================================================================================================================================================═
// Error formatting with source context
// ==========================================================================================================================================================================================================================================═

namespace pml {

namespace {

/// Number of context lines to show before and after the error line.
constexpr int CONTEXT_LINES = 2;

/// Format a single error with source context (rich version).
std::string format_single_error(
    const PMLException& err, SourceManager& src, int index, int total) {
    std::string result;

    // Error header with index for multi-error
    if (total > 1) {
        result += std::format("\n--- Error {} of {} ---\n", index + 1, total);
    }

    result += "Error: ";
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

            // Determine line range for context
            int context_start = (std::max)(1, loc.line - CONTEXT_LINES);
            int context_end = loc.line + CONTEXT_LINES;

            // Compute line number width for alignment
            int max_line = context_end;
            int line_width = (std::max)(3, static_cast<int>(std::to_string(max_line).length()));

            // Get the line count in the file to cap context_end
            // We need to check how many lines are available
            // Try loading the last context line; if empty, reduce
            while (context_end > loc.line) {
                std::string test_line = src.get_line(filename, context_end);
                if (!test_line.empty() || context_end == loc.line) break;
                --context_end;
            }

            result += "\n";

            // Print context lines before the error
            for (int l = context_start; l <= context_end; ++l) {
                std::string line_text = src.get_line(filename, l);
                if (l == loc.line) {
                    // Error line with arrow marker
                    result += std::format(" {:>{}} | {}\n", l, line_width, line_text);

                    // Caret/range line
                    std::string marker_line(line_width + 3, ' ');
                    int caret_col = (loc.column > 0) ? loc.column : 1;

                    if (loc.end_column > 0 && loc.end_line == l) {
                        // Range marker: ~ from start to end
                        int range_end = (std::min)(
                            static_cast<int>(line_text.length()) + 1,
                            loc.end_column);
                        marker_line.append(caret_col - 1, ' ');
                        marker_line.append(
                            (std::max)(1, range_end - caret_col), '~');
                        marker_line += "  here";
                    } else if (loc.end_column > 0 && loc.end_line > l) {
                        // Multi-line range: starts here, continues
                        marker_line.append(caret_col - 1, ' ');
                        marker_line.append(
                            (std::max)(1, static_cast<int>(line_text.length()) - caret_col + 2), '~');
                        marker_line += "  (continues on next line)";
                    } else {
                        // Single caret
                        marker_line.append(caret_col - 1, ' ');
                        marker_line += '^';
                    }
                    result += marker_line;
                    result += '\n';
                } else {
                    // Context line (before or after error)
                    result += std::format(" {:>{}} | {}\n", l, line_width, line_text);
                }
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

}  // anonymous namespace

std::string format_error_with_source(
    const PMLException& err, SourceManager& src) {
    std::string result;

    if (err.details.empty()) {
        // Single error
        result = format_single_error(err, src, 0, 1);
    } else {
        // Multi-error: header + each sub-error
        result = std::format("Found {} error(s):\n", err.details.size());
        for (size_t i = 0; i < err.details.size(); ++i) {
            result += format_single_error(err.details[i], src,
                                          static_cast<int>(i),
                                          static_cast<int>(err.details.size()));
        }
    }

    return result;
}

}  // namespace pml
