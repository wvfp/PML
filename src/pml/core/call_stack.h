#pragma once

// ==========================================================================================================================================================================================================================================═
// Call Stack — track function call sites for error reporting
// ==========================================================================================================================================================================================================================================═

#include "error.h"

#include <string>
#include <vector>

namespace pml {

/// Thread-local runtime call stack.
///
/// Push a frame before invoking a user-defined procedure or builtin and pop it
/// on return. The stack is captured when an error is formatted so that users
/// see the sequence of calls that led to the failure.
class CallStack {
public:
    /// Return the thread-local call stack instance.
    [[nodiscard]] static CallStack& instance();

    CallStack() = default;
    CallStack(const CallStack&) = delete;
    CallStack& operator=(const CallStack&) = delete;

    /// Push a frame onto the stack.
    void push(const CallFrame& frame);
    void push(std::string function_name, SourceLocation call_site);

    /// Pop the top frame. Safe to call when the stack is empty.
    void pop();

    /// Remove all frames.
    void clear();

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] const std::vector<CallFrame>& frames() const noexcept;

    /// RAII guard that pops the stack on destruction.
    struct FrameGuard {
        ~FrameGuard();
        FrameGuard(const FrameGuard&) = delete;
        FrameGuard& operator=(const FrameGuard&) = delete;

    private:
        explicit FrameGuard(CallStack& stack);
        CallStack& stack_;
        friend class CallStack;
    };

    /// Push a frame and return an RAII guard that pops it.
    [[nodiscard]] FrameGuard push_guard(const CallFrame& frame);
    [[nodiscard]] FrameGuard push_guard(
        std::string function_name, SourceLocation call_site);

private:
    std::vector<CallFrame> frames_;
};

/// Format a list of call frames as a human-readable call stack.
/// Returns an empty string if `frames` is empty.
[[nodiscard]] std::string format_call_stack(
    const std::vector<CallFrame>& frames);

}  // namespace pml
