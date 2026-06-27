// ==========================================================================================================================================================================================================================================═
// Call Stack — Implementation
// ==========================================================================================================================================================================================================================================═

#include "call_stack.h"

#include "pml/api/context.h"

#include <format>
#include <utility>

namespace pml {

// ==========================================================================================================================================================================================================================================═
// CallStack
// ==========================================================================================================================================================================================================================================═

CallStack& CallStack::instance() {
    if (auto* ctx = PMLContext::current_ptr()) {
        if (ctx->call_stack) {
            return *ctx->call_stack;
        }
    }
    thread_local CallStack cs;
    return cs;
}

void CallStack::push(const CallFrame& frame) {
    frames_.push_back(frame);
}

void CallStack::push(std::string function_name, SourceLocation call_site) {
    frames_.push_back(CallFrame{std::move(function_name), std::move(call_site)});
}

void CallStack::pop() {
    if (!frames_.empty()) {
        frames_.pop_back();
    }
}

void CallStack::clear() {
    frames_.clear();
}

bool CallStack::empty() const noexcept {
    return frames_.empty();
}

size_t CallStack::size() const noexcept {
    return frames_.size();
}

const std::vector<CallFrame>& CallStack::frames() const noexcept {
    return frames_;
}

CallStack::FrameGuard CallStack::push_guard(const CallFrame& frame) {
    push(frame);
    return FrameGuard(*this);
}

CallStack::FrameGuard CallStack::push_guard(
    std::string function_name, SourceLocation call_site) {
    push(std::move(function_name), std::move(call_site));
    return FrameGuard(*this);
}

// ==========================================================================================================================================================================================================================================═
// FrameGuard
// ==========================================================================================================================================================================================================================================═

CallStack::FrameGuard::FrameGuard(CallStack& stack) : stack_(stack) {}

CallStack::FrameGuard::~FrameGuard() {
    stack_.pop();
}

// ==========================================================================================================================================================================================================================================═
// Formatting
// ==========================================================================================================================================================================================================================================═

std::string format_call_stack(const std::vector<CallFrame>& frames) {
    if (frames.empty()) {
        return {};
    }

    std::string result = "\nCall stack:\n";
    // Print innermost (most recent) frame first.
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
        const std::string& name = it->function_name.empty()
                                      ? std::string("<anonymous>")
                                      : it->function_name;
        result += std::format("  at {} ({})\n", name, it->call_site.to_string());
    }
    return result;
}

}  // namespace pml
