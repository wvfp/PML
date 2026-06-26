// ==========================================================================================================================================================================================================================================═
// PML Path Command Types — SVG path parser & command utilities
// ==========================================================================================================================================================================================================================================═

#include "path_types.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <optional>
#include <string>
#include <system_error>

namespace pml {

namespace {

// ---- SVG path data tokeniser ----------------------------------------------------------------------------------------─
//
// Tokenises an SVG `d` attribute into command letters and numeric values,
// handling comma/whitespace separation and implicit commands.

enum class TokenType { Command, Number, Eof };

struct Token {
    TokenType type;
    char      cmd_char = 0;     // valid for Command
    double    number   = 0.0;   // valid for Number
};

class SvgTokenizer {
public:
    explicit SvgTokenizer(std::string_view src) : src_(src), pos_(0) {}

    Token peek() {
        if (!peeked_) {
            peeked_ = true;
            peeked_token_ = next();
        }
        return peeked_token_;
    }

    Token consume() {
        if (peeked_) {
            peeked_ = false;
            return peeked_token_;
        }
        return next();
    }

    bool eof() const { return pos_ >= src_.size() && !peeked_; }

private:
    std::string_view src_;
    size_t pos_ = 0;
    bool peeked_ = false;
    Token peeked_token_;

    void skip_ws() {
        while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
            ++pos_;
        }
        // Skip commas
        while (pos_ < src_.size() && src_[pos_] == ',') {
            ++pos_;
            // Skip whitespace after comma too
            while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
                ++pos_;
            }
        }
    }

    Token next() {
        skip_ws();
        if (pos_ >= src_.size()) return {TokenType::Eof, 0, 0.0};

        char c = src_[pos_];

        // Command letters: A-Z, a-z (but not e/E which are part of scientific notation)
        if (std::isalpha(static_cast<unsigned char>(c)) && c != 'e' && c != 'E') {
            ++pos_;
            return {TokenType::Command, c, 0.0};
        }

        // Number: digits, +/-, ., e/E for scientific notation
        return parse_number();
    }

    Token parse_number() {
        size_t start = pos_;

        // Optional sign
        if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) {
            ++pos_;
        }

        // Digits before decimal point
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            ++pos_;
        }

        // Decimal point and digits after
        if (pos_ < src_.size() && src_[pos_] == '.') {
            ++pos_;
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                ++pos_;
            }
        }

        // Scientific notation
        if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) {
                ++pos_;
            }
            while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                ++pos_;
            }
        }

        if (start == pos_) {
            // No digits consumed — shouldn't happen if caller checks
            return {TokenType::Eof, 0, 0.0};
        }

        double val = 0.0;
        std::from_chars(src_.data() + start, src_.data() + pos_, val);
        return {TokenType::Number, 0, val};
    }
};

} // anonymous namespace

// ==============================================================================================================================================================================================================================═
// Command letter ↔ PathCmdType mapping
// ==============================================================================================================================================================================================================================═

std::optional<PathCmdType> svg_letter_to_cmd(char c) noexcept {
    switch (c) {
        case 'M': return PathCmdType::MoveTo;
        case 'm': return PathCmdType::MoveToR;
        case 'L': return PathCmdType::LineTo;
        case 'l': return PathCmdType::LineToR;
        case 'H': return PathCmdType::HLineTo;
        case 'h': return PathCmdType::HLineToR;
        case 'V': return PathCmdType::VLineTo;
        case 'v': return PathCmdType::VLineToR;
        case 'C': return PathCmdType::CubicTo;
        case 'c': return PathCmdType::CubicToR;
        case 'S': return PathCmdType::SmoothCubicTo;
        case 's': return PathCmdType::SmoothCubicToR;
        case 'Q': return PathCmdType::QuadTo;
        case 'q': return PathCmdType::QuadToR;
        case 'T': return PathCmdType::SmoothQuadTo;
        case 't': return PathCmdType::SmoothQuadToR;
        case 'A': return PathCmdType::Arc;
        case 'a': return PathCmdType::ArcR;
        case 'Z': case 'z': return PathCmdType::Close;
        default:  return std::nullopt;
    }
}

char cmd_to_svg_letter(PathCmdType type) noexcept {
    switch (type) {
        case PathCmdType::MoveTo:         return 'M';
        case PathCmdType::MoveToR:        return 'm';
        case PathCmdType::LineTo:         return 'L';
        case PathCmdType::LineToR:        return 'l';
        case PathCmdType::HLineTo:        return 'H';
        case PathCmdType::HLineToR:       return 'h';
        case PathCmdType::VLineTo:        return 'V';
        case PathCmdType::VLineToR:       return 'v';
        case PathCmdType::CubicTo:        return 'C';
        case PathCmdType::CubicToR:       return 'c';
        case PathCmdType::SmoothCubicTo:  return 'S';
        case PathCmdType::SmoothCubicToR: return 's';
        case PathCmdType::QuadTo:         return 'Q';
        case PathCmdType::QuadToR:        return 'q';
        case PathCmdType::SmoothQuadTo:   return 'T';
        case PathCmdType::SmoothQuadToR:  return 't';
        case PathCmdType::Arc:            return 'A';
        case PathCmdType::ArcR:           return 'a';
        case PathCmdType::Close:          return 'Z';
    }
    return '?';
}

std::optional<PathCmdType> string_to_cmd(std::string_view name) noexcept {
    // Map PML command symbol names to PathCmdType
    static constexpr struct { std::string_view name; PathCmdType type; } kMap[] = {
        {"move-to",         PathCmdType::MoveTo},
        {"move-to-r",       PathCmdType::MoveToR},
        {"line-to",         PathCmdType::LineTo},
        {"line-to-r",       PathCmdType::LineToR},
        {"hline-to",        PathCmdType::HLineTo},
        {"hline-to-r",      PathCmdType::HLineToR},
        {"vline-to",        PathCmdType::VLineTo},
        {"vline-to-r",      PathCmdType::VLineToR},
        {"cubic-to",        PathCmdType::CubicTo},
        {"cubic-to-r",      PathCmdType::CubicToR},
        {"smooth-cubic-to", PathCmdType::SmoothCubicTo},
        {"smooth-cubic-to-r", PathCmdType::SmoothCubicToR},
        {"quad-to",         PathCmdType::QuadTo},
        {"quad-to-r",       PathCmdType::QuadToR},
        {"smooth-quad-to",  PathCmdType::SmoothQuadTo},
        {"smooth-quad-to-r", PathCmdType::SmoothQuadToR},
        {"arc",             PathCmdType::Arc},
        {"arc-r",           PathCmdType::ArcR},
        {"close",           PathCmdType::Close},
    };
    for (const auto& entry : kMap) {
        if (entry.name == name) return entry.type;
    }
    return std::nullopt;
}

// ==============================================================================================================================================================================================================================═
// SVG path string parser
// ==============================================================================================================================================================================================================================═

std::expected<std::vector<PathCommand>, std::string>
parse_svg_path_string(std::string_view d) noexcept {
    if (d.empty()) {
        return std::unexpected(std::string("SVG path data is empty"));
    }

    SvgTokenizer tokenizer(d);
    std::vector<PathCommand> commands;

    // First token must be a command
    Token tok = tokenizer.consume();
    if (tok.type != TokenType::Command) {
        return std::unexpected(std::string("SVG path must start with a command letter"));
    }

    // We process one "command + its arguments" at a time.
    // If a number appears without a preceding command, it's an implicit
    // repeat of the previous command (per SVG spec).
    PathCmdType current_cmd = PathCmdType::Close; // sentinel
    bool first = true;

    while (tok.type != TokenType::Eof) {
        if (tok.type == TokenType::Command) {
            auto opt_cmd = svg_letter_to_cmd(tok.cmd_char);
            if (!opt_cmd) {
                return std::unexpected(
                    std::string("Unknown SVG path command: ") + tok.cmd_char);
            }
            current_cmd = *opt_cmd;
            first = false;
            tok = tokenizer.consume(); // move to the first argument
        } else if (tok.type == TokenType::Number) {
            if (first) {
                return std::unexpected(std::string(
                    "SVG path data starts with a number, not a command"));
            }
            // Implicit repeat of the previous command — we don't re-consume
            // the number; we leave it as the first argument below.
            // This works because current_cmd is already set.
        } else {
            break;
        }

        // Handle Close (no arguments, just emit and move on)
        if (current_cmd == PathCmdType::Close) {
            commands.emplace_back(current_cmd, std::vector<double>{});
            tok = tokenizer.consume();
            continue;
        }

        // Collect arguments for this command.
        // SVG allows repeated argument groups: e.g. "M 10 10 20 20 30 30"
        // means M 10 10, then implicit L 20 20, L 30 30.
        // Similarly for C: "C 0 0 10 10 20 20 30 30 40 40 50 50"
        // means two cubic beziers.
        const uint8_t arg_count = kArgCounts[static_cast<uint8_t>(current_cmd)];
        bool had_args = false;

        while (true) {
            // Skip trailing commands (they'll be picked up on next iteration)
            if (tok.type == TokenType::Command) break;
            if (tok.type == TokenType::Eof) break;

            std::vector<double> args;
            args.reserve(arg_count);

            // For implicit repeats, the first number is already in tok
            if (tok.type == TokenType::Number) {
                args.push_back(tok.number);
                tok = tokenizer.consume();
            }

            while (args.size() < arg_count) {
                if (tok.type != TokenType::Number) {
                    // Check if it's a command that interrupted us
                    break;
                }
                args.push_back(tok.number);
                tok = tokenizer.consume();
            }

            if (args.empty()) break;

            if (args.size() < arg_count) {
                return std::unexpected(
                    std::string("SVG path command ") +
                    cmd_to_svg_letter(current_cmd) +
                    " requires " + std::to_string(arg_count) +
                    " arguments, got " + std::to_string(args.size()));
            }

            commands.emplace_back(current_cmd, std::move(args));
            had_args = true;

            // Check if this command type allows implicit repeats
            // (M has a special case: implicit L after the first pair)
            if (current_cmd == PathCmdType::MoveTo ||
                current_cmd == PathCmdType::MoveToR) {
                // After the first MoveTo pair, remaining pairs are LineTo
                current_cmd = (current_cmd == PathCmdType::MoveTo)
                    ? PathCmdType::LineTo
                    : PathCmdType::LineToR;
            }
        }

        if (!had_args && current_cmd != PathCmdType::Close) {
            return std::unexpected(
                std::string("SVG path command ") +
                cmd_to_svg_letter(current_cmd) +
                " requires " + std::to_string(arg_count) +
                " arguments but none were found");
        }

        // Continue loop — tok already advanced
    }

    return commands;
}

} // namespace pml
