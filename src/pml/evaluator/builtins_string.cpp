// ═══════════════════════════════════════════════════════════════════════════════
// PML Built-in Procedures — String Operations
// ═══════════════════════════════════════════════════════════════════════════════

#include "builtins.h"
#include "builtins_helpers.h"

#include <string>

namespace pml {

void register_string_builtins(std::shared_ptr<Environment> env) {
    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
        env->define(name, Value(proc));
    };

    def("string-append",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            std::string result;
            for (const auto& arg : args) {
                const auto* s = arg.as_string();
                if (!s) {
                    return std::unexpected(
                        type_error("string-append: expected string arguments"));
                }
                result += *s;
            }
            return Value(std::move(result));
        });

    def("string-length",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* s = args[0].as_string();
            if (!s) {
                return std::unexpected(
                    type_error("string-length: expected string"));
            }
            return Value(static_cast<int64_t>(s->size()));
        });

    def("substring",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 3) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 3, static_cast<int>(args.size())));
            }
            const auto* s = args[0].as_string();
            if (!s) {
                return std::unexpected(
                    type_error("substring: expected string"));
            }
            int64_t start = to_int64(args[1]);
            int64_t end = to_int64(args[2]);

            if (start < 0) start = 0;
            if (end > static_cast<int64_t>(s->size())) end = static_cast<int64_t>(s->size());
            if (start >= end) return Value(std::string(""));

            return Value(s->substr(static_cast<size_t>(start),
                                    static_cast<size_t>(end - start)));
        });

    // ── string-copy, make-string ──────────────────────────────

    def("string-copy", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        const auto* s = args[0].as_string();
        if (!s) {
            return std::unexpected(
                type_error("string-copy: expected string"));
        }
        return Value(*s);
    });

    def("make-string", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 1 || args.size() > 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (!is_integer(args[0]) || to_int64(args[0]) < 0) {
            return std::unexpected(
                type_error("make-string: expected non-negative integer"));
        }
        int64_t len = to_int64(args[0]);
        char fill = ' ';
        if (args.size() == 2) {
            const auto* s = args[1].as_string();
            if (!s || s->size() != 1) {
                return std::unexpected(
                    type_error("make-string: fill must be a single character string"));
            }
            fill = (*s)[0];
        }
        return Value(std::string(static_cast<size_t>(len), fill));
    });

    def("string-ref",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 2) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 2, static_cast<int>(args.size())));
            }
            const auto* s = args[0].as_string();
            if (!s) {
                return std::unexpected(
                    type_error("string-ref: expected string"));
            }
            int64_t index = to_int64(args[1]);
            if (index < 0 || static_cast<size_t>(index) >= s->size()) {
                return std::unexpected(
                    general_error(std::format("string-ref: index {} out of range "
                                               "for string of length {}",
                                               index, s->size())));
            }
            return Value(std::string(1, (*s)[static_cast<size_t>(index)]));
        });

    def("number->string",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            if (!is_number(args[0])) {
                return std::unexpected(
                    type_error("number->string: expected numeric argument"));
            }
            if (args[0].is_double()) {
                return Value(value_to_string(args[0]));
            }
            return Value(std::to_string(to_int64(args[0])));
        });

    def("string->number",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* s = args[0].as_string();
            if (!s) {
                return std::unexpected(
                    type_error("string->number: expected string"));
            }
            try {
                size_t idx = 0;
                if (s->find('.') != std::string::npos ||
                    s->find('e') != std::string::npos ||
                    s->find('E') != std::string::npos) {
                    double val = std::stod(*s, &idx);
                    if (idx != s->size()) {
                        return std::unexpected(type_error(
                            std::format("string->number: cannot convert '{}'",
                                        *s)));
                    }
                    return Value(val);
                }
                int64_t val = std::stoll(*s, &idx);
                if (idx != s->size()) {
                    return std::unexpected(type_error(
                        std::format("string->number: cannot convert '{}'",
                                    *s)));
                }
                return Value(val);
            } catch (const std::exception&) {
                return std::unexpected(
                    type_error(std::format("string->number: cannot convert '{}'",
                                            *s)));
            }
        });

    def("format",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.empty()) {
                return std::unexpected(
                    type_error("format: expected at least 1 argument"));
            }
            const auto* fmt = args[0].as_string();
            if (!fmt) {
                return std::unexpected(
                    type_error("format: first argument must be string"));
            }
            std::string result = *fmt;
            for (size_t i = 1; i < args.size(); ++i) {
                size_t pos = result.find("~a");
                if (pos == std::string::npos) break;
                result.replace(pos, 2, value_to_string(args[i]));
            }
            return Value(std::move(result));
        });

    def("string->symbol",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* s = args[0].as_string();
            if (!s) {
                return std::unexpected(
                    type_error("string->symbol: expected string"));
            }
            return Value(Symbol(*s));
        });

    def("symbol->string",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* sym = args[0].as_symbol();
            if (!sym) {
                return std::unexpected(
                    type_error("symbol->string: expected symbol"));
            }
            return Value(std::string(sym->name));
        });

    def("string->list",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* s = args[0].as_string();
            if (!s) {
                return std::unexpected(
                    type_error("string->list: expected string"));
            }
            std::vector<Value> chars;
            for (char c : *s) {
                chars.push_back(Value(std::string(1, c)));
            }
            return make_list_value(std::move(chars));
        });

    def("list->string",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() != 1) {
                return std::unexpected(arity_error(
                    SourceLocation{}, 1, static_cast<int>(args.size())));
            }
            const auto* lst = args[0].as_list();
            if (!lst || !*lst) {
                return std::unexpected(
                    type_error("list->string: expected list of strings"));
            }
            std::string result;
            for (const auto& v : (*lst)->elements) {
                const auto* s = v.as_string();
                if (!s) {
                    return std::unexpected(
                        type_error("list->string: list elements must be strings"));
                }
                result += *s;
            }
            return Value(std::move(result));
        });

    def("string=?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() < 2) {
                return std::unexpected(
                    type_error("string=? requires at least 2 arguments"));
            }
            const auto* first = args[0].as_string();
            if (!first) {
                return std::unexpected(
                    type_error("string=?: expected string arguments"));
            }
            for (size_t i = 1; i < args.size(); ++i) {
                const auto* s = args[i].as_string();
                if (!s || *s != *first) return Value(false);
            }
            return Value(true);
        });

    def("string<?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() < 2) {
                return std::unexpected(
                    type_error("string<? requires at least 2 arguments"));
            }
            std::vector<const std::string*> strs;
            for (const auto& arg : args) {
                const auto* s = arg.as_string();
                if (!s) {
                    return std::unexpected(
                        type_error("string<?: expected string arguments"));
                }
                strs.push_back(s);
            }
            for (size_t i = 1; i < strs.size(); ++i) {
                if (!(*strs[i - 1] < *strs[i])) return Value(false);
            }
            return Value(true);
        });

    def("string>?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() < 2) {
                return std::unexpected(
                    type_error("string>? requires at least 2 arguments"));
            }
            std::vector<const std::string*> strs;
            for (const auto& arg : args) {
                const auto* s = arg.as_string();
                if (!s) {
                    return std::unexpected(
                        type_error("string>?: expected string arguments"));
                }
                strs.push_back(s);
            }
            for (size_t i = 1; i < strs.size(); ++i) {
                if (!(*strs[i - 1] > *strs[i])) return Value(false);
            }
            return Value(true);
        });

    def("string<=?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() < 2) {
                return std::unexpected(
                    type_error("string<=? requires at least 2 arguments"));
            }
            std::vector<const std::string*> strs;
            for (const auto& arg : args) {
                const auto* s = arg.as_string();
                if (!s) {
                    return std::unexpected(
                        type_error("string<=?: expected string arguments"));
                }
                strs.push_back(s);
            }
            for (size_t i = 1; i < strs.size(); ++i) {
                if (!(*strs[i - 1] <= *strs[i])) return Value(false);
            }
            return Value(true);
        });

    def("string>=?",
        [](const std::vector<Value>& args, Environment&) -> Result<Value> {
            if (args.size() < 2) {
                return std::unexpected(
                    type_error("string>=? requires at least 2 arguments"));
            }
            std::vector<const std::string*> strs;
            for (const auto& arg : args) {
                const auto* s = arg.as_string();
                if (!s) {
                    return std::unexpected(
                        type_error("string>=?: expected string arguments"));
                }
                strs.push_back(s);
            }
            for (size_t i = 1; i < strs.size(); ++i) {
                if (!(*strs[i - 1] >= *strs[i])) return Value(false);
            }
            return Value(true);
        });
}

}  // namespace pml
