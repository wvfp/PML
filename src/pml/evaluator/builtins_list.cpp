// ==========================================================================================================================================================================================================================================═
// PML Built-in Procedures — List Operations
// ==========================================================================================================================================================================================================================================═

#include "builtins.h"
#include "builtins_helpers.h"

#include "evaluator.h"

#include <algorithm>
#include <format>

namespace pml {

// ---- car implementation --------------------------------------------------------
static Result<Value> car_impl(const std::vector<Value>& args, Environment&) {
    if (args.size() != 1) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    const auto* lst = args[0].as_list();
    if (!lst || !*lst || (*lst)->elements.empty()) {
        return std::unexpected(
            type_error("car: expected non-empty list"));
    }
    return (*lst)->elements[0];
}

// ---- cdr implementation --------------------------------------------------------
static Result<Value> cdr_impl(const std::vector<Value>& args, Environment&) {
    if (args.size() != 1) {
        return std::unexpected(
            arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }
    const auto* lst = args[0].as_list();
    if (!lst || !*lst || (*lst)->elements.empty()) {
        return std::unexpected(
            type_error("cdr: expected non-empty list"));
    }
    std::vector<Value> rest((*lst)->elements.begin() + 1,
                            (*lst)->elements.end());
    return make_list_value(std::move(rest));
}

void register_list_builtins(std::shared_ptr<Environment> env) {
    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
        env->define(name, Value(proc));
    };

    def("car", car_impl);
    def("cdr", cdr_impl);
    def("first", car_impl);
    def("rest", cdr_impl);

    def("second", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 1)
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(a.size())));
        const auto* lst = a[0].as_list();
        if (!lst || !*lst || (*lst)->elements.size() < 2)
            return std::unexpected(type_error("second: list too short"));
        return (*lst)->elements[1];
    });

    def("third", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 1)
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(a.size())));
        const auto* lst = a[0].as_list();
        if (!lst || !*lst || (*lst)->elements.size() < 3)
            return std::unexpected(type_error("third: list too short"));
        return (*lst)->elements[2];
    });

    // cadr = car of cdr — second element
    // caddr = car of cdr of cdr — third element
    auto combo = [](const std::vector<Value>& args, Environment&,
                    const char* name, size_t n) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst || (*lst)->elements.size() <= n) {
            return std::unexpected(
                type_error(std::string(name) + ": list too short (need at least " +
                           std::to_string(n + 1) + " elements)"));
        }
        return (*lst)->elements[n];
    };
    def("cadr",  [combo](auto& a, auto& e) { return combo(a, e, "cadr", 1); });
    def("caddr", [combo](auto& a, auto& e) { return combo(a, e, "caddr", 2); });

    def("cons", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const Value& a = args[0];
        const Value& b = args[1];

        if (const auto* lst = b.as_list()) {
            if (*lst) {
                std::vector<Value> result;
                result.reserve(1 + (*lst)->elements.size());
                result.push_back(a);
                result.insert(result.end(),
                              (*lst)->elements.begin(),
                              (*lst)->elements.end());
                return make_list_value(std::move(result));
            }
        }
        return make_list_value({a, b});
    });

    def("list", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        return make_list_value(args);
    });

    def("length", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("length: expected list"));
        }
        return Value(static_cast<int64_t>((*lst)->elements.size()));
    });

    def("append", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        std::vector<Value> result;
        for (const auto& arg : args) {
            const auto* lst = arg.as_list();
            if (!lst || !*lst) {
                return std::unexpected(
                    type_error("append: expected list arguments"));
            }
            result.insert(result.end(),
                          (*lst)->elements.begin(),
                          (*lst)->elements.end());
        }
        return make_list_value(std::move(result));
    });

    def("reverse", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("reverse: expected list"));
        }
        auto reversed = (*lst)->elements;
        std::reverse(reversed.begin(), reversed.end());
        return make_list_value(std::move(reversed));
    });

    def("nth", [](const std::vector<Value>& a, Environment&) -> Result<Value> {
        if (a.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(a.size())));
        }
        const auto* lst = a[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("nth: second argument must be a list"));
        }
        int64_t idx = to_int64(a[0]);
        if (idx < 0 || static_cast<size_t>(idx) >= (*lst)->elements.size()) {
            return std::unexpected(
                general_error(std::format("nth: index {} out of range", idx)));
        }
        return (*lst)->elements[static_cast<size_t>(idx)];
    });

    def("list-ref", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("list-ref: first argument must be list"));
        }
        int64_t index = to_int64(args[1]);
        if (index < 0 || static_cast<size_t>(index) >= (*lst)->elements.size()) {
            return std::unexpected(
                general_error(std::format("list-ref: index {} out of range for list of "
                                           "length {}", index,
                                           (*lst)->elements.size())));
        }
        return (*lst)->elements[static_cast<size_t>(index)];
    });

    def("list-tail", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[0].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("list-tail: first argument must be list"));
        }
        int64_t index = to_int64(args[1]);
        if (index < 0 || static_cast<size_t>(index) > (*lst)->elements.size()) {
            return std::unexpected(
                general_error(std::format("list-tail: index {} out of range for list of "
                                           "length {}", index,
                                           (*lst)->elements.size())));
        }
        std::vector<Value> tail((*lst)->elements.begin() + static_cast<size_t>(index),
                                (*lst)->elements.end());
        return make_list_value(std::move(tail));
    });

    def("member", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("member: second argument must be list"));
        }
        const auto& elems = (*lst)->elements;
        for (size_t i = 0; i < elems.size(); ++i) {
            if (deep_equal(elems[i], args[0])) {
                std::vector<Value> tail(elems.begin() + i, elems.end());
                return make_list_value(std::move(tail));
            }
        }
        return Value(false);
    });

    def("assoc", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("assoc: second argument must be list"));
        }
        for (const auto& item : (*lst)->elements) {
            const auto* pair = item.as_list();
            if (pair && *pair && !(*pair)->elements.empty()) {
                if (deep_equal((*pair)->elements[0], args[0])) {
                    return item;
                }
            }
        }
        return Value(false);
    });

    // ---- memq, assq --------------------------------------------------------------------------------------------------------─

    def("memq", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("memq: second argument must be list"));
        }
        for (size_t i = 0; i < (*lst)->elements.size(); ++i) {
            if ((*lst)->elements[i] == args[0]) {
                std::vector<Value> tail;
                tail.reserve((*lst)->elements.size() - i);
                for (size_t j = i; j < (*lst)->elements.size(); ++j) {
                    tail.push_back((*lst)->elements[j]);
                }
                return make_list_value(std::move(tail));
            }
        }
        return Value(false);
    });

    def("assq", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(
                type_error("assq: second argument must be list"));
        }
        for (const auto& item : (*lst)->elements) {
            const auto* pair = item.as_list();
            if (pair && *pair && !(*pair)->elements.empty()) {
                if ((*pair)->elements[0] == args[0]) {
                    return item;
                }
            }
        }
        return Value(false);
    });

    def("range", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() < 1 || args.size() > 3) {
            return std::unexpected(
                type_error("range: expects 1, 2, or 3 arguments (end [start [step]])"));
        }
        int64_t start, end, step;
        if (args.size() == 1) {
            // (range end) → (range 0 end 1)
            start = 0;
            end = to_int64(args[0]);
            step = 1;
        } else {
            start = to_int64(args[0]);
            end = to_int64(args[1]);
            step = (args.size() == 3) ? to_int64(args[2]) : 1;
        }

        if (step == 0) {
            return std::unexpected(
                general_error("range: step cannot be zero"));
        }

        if (step > 0 && end <= start) {
            return make_list_value(std::vector<Value>{});
        }
        if (step < 0 && end >= start) {
            return make_list_value(std::vector<Value>{});
        }

        std::vector<Value> result;
        if (step > 0) {
            for (int64_t i = start; i < end; i += step) {
                result.push_back(Value(i));
            }
        } else {
            for (int64_t i = start; i > end; i += step) {
                result.push_back(Value(i));
            }
        }
        return make_list_value(std::move(result));
    });

    // Higher-order list functions (need evaluator access to apply fn)
    def("map", [](const std::vector<Value>& args, Environment& env) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(type_error("map: second argument must be a list"));
        }
        std::vector<Value> out;
        out.reserve((*lst)->elements.size());
        for (const auto& item : (*lst)->elements) {
            auto r = trampoline(apply_function(args[0], {item}, {}, env.shared_from_this()));
            if (!r) return std::unexpected(std::move(r.error()));
            out.push_back(std::move(*r));
        }
        return make_list_value(std::move(out));
    });

    def("filter", [](const std::vector<Value>& args, Environment& env) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(type_error("filter: second argument must be a list"));
        }
        std::vector<Value> out;
        for (const auto& item : (*lst)->elements) {
            auto r = trampoline(apply_function(args[0], {item}, {}, env.shared_from_this()));
            if (!r) return std::unexpected(std::move(r.error()));
            if (is_truthy(*r)) out.push_back(item);
        }
        return make_list_value(std::move(out));
    });

    def("reduce", [](const std::vector<Value>& args, Environment& env) -> Result<Value> {
        if (args.size() != 3) {
            return std::unexpected(
                arity_error(SourceLocation{}, 3, static_cast<int>(args.size())));
        }
        const auto* lst = args[2].as_list();
        if (!lst || !*lst) {
            return std::unexpected(type_error("reduce: third argument must be a list"));
        }
        Value acc = args[1];
        for (const auto& item : (*lst)->elements) {
            auto r = trampoline(apply_function(args[0], {acc, item}, {}, env.shared_from_this()));
            if (!r) return std::unexpected(std::move(r.error()));
            acc = std::move(*r);
        }
        return acc;
    });

    def("for-each", [](const std::vector<Value>& args, Environment& env) -> Result<Value> {
        if (args.size() != 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        const auto* lst = args[1].as_list();
        if (!lst || !*lst) {
            return std::unexpected(type_error("for-each: second argument must be a list"));
        }
        for (const auto& item : (*lst)->elements) {
            auto r = trampoline(apply_function(args[0], {item}, {}, env.shared_from_this()));
            if (!r) return std::unexpected(std::move(r.error()));
        }
        return make_nil_value();
    });

    def("apply", [](const std::vector<Value>& args, Environment& env) -> Result<Value> {
        if (args.size() < 2) {
            return std::unexpected(
                arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
        }
        std::vector<Value> call_args;
        for (size_t i = 1; i + 1 < args.size(); ++i) {
            call_args.push_back(args[i]);
        }
        const auto* lst = args.back().as_list();
        if (!lst || !*lst) {
            return std::unexpected(type_error("apply: last argument must be a list"));
        }
        call_args.insert(call_args.end(), (*lst)->elements.begin(), (*lst)->elements.end());
        return trampoline(apply_function(args[0], call_args, {}, env.shared_from_this()));
    });

    def("null?", [](const std::vector<Value>& args, Environment&) -> Result<Value> {
        if (args.size() != 1) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }
        if (is_nil(args[0])) return Value(true);
        if (const auto* lst = args[0].as_list()) {
            if (*lst && (*lst)->elements.empty()) return Value(true);
        }
        return Value(false);
    });
}

}  // namespace pml
