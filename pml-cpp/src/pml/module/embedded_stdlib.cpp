// ═══════════════════════════════════════════════════════════════════════════════
// PML Embedded Stdlib Loader — Implementation
// ───────────────────────────────────────────────────────────────────────────────
// Loads all embedded standard library modules (from the auto-generated
// embedded_stdlib.h) into the global environment at runtime.
// ═══════════════════════════════════════════════════════════════════════════════

// Include the auto-generated embedded stdlib data FIRST.
// The relative path resolves from src/pml/module/ to src/pml/core/.
#include "../core/embedded_stdlib.h"

#include "embedded_stdlib.h"       // own declaration
#include "environment.h"            // Environment
#include "evaluator.h"              // evaluate()
#include "expander.h"               // Expander
#include "lexer.h"                  // Lexer
#include "parser.h"                 // Parser

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace pml {

namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════════════════════════
// load_embedded_stdlib — entry point
// ═══════════════════════════════════════════════════════════════════════════════

void load_embedded_stdlib(std::shared_ptr<Environment> global_env) {
    for (size_t i = 0; i < stdlib_entries_count; ++i) {
        const auto& entry = stdlib_entries[i];

        // Extract module name: path stem (basename without .pml extension)
        // e.g. "math.pml" → "math", "sprites/body.pml" → "body"
        std::string module_name = fs::path(entry.path).stem().string();

        // Wrap the embedded C-string data into a std::string for the Lexer.
        // entry.data is null-terminated, entry.size is the content length.
        std::string source(entry.data, entry.size);

        // ── 1. Lex ──────────────────────────────────────────────────────
        Lexer lexer(source, entry.path);
        auto tokens = lexer.tokenize();
        if (!tokens) {
            std::cerr << "Warning: stdlib module '" << module_name
                      << "' (" << entry.path << ") failed to load [lex]: "
                      << tokens.error().what() << std::endl;
            continue;
        }

        // ── 2. Parse ─────────────────────────────────────────────────────
        Parser parser(std::move(*tokens), entry.path);
        auto ast = parser.parse();
        if (!ast) {
            std::cerr << "Warning: stdlib module '" << module_name
                      << "' (" << entry.path << ") failed to load [parse]: "
                      << ast.error().what() << std::endl;
            continue;
        }

        // ── 3. Macro expansion ───────────────────────────────────────────
        Expander expander(global_env);
        auto expanded = expander.expand_all(*ast);
        if (!expanded) {
            std::cerr << "Warning: stdlib module '" << module_name
                      << "' (" << entry.path << ") failed to load [expand]: "
                      << expanded.error().what() << std::endl;
            continue;
        }

        // ── 4. Evaluate into the global environment ─────────────────────
        bool module_failed = false;
        for (const auto& expr : *expanded) {
            auto result = evaluate(expr, global_env);
            if (!result) {
                std::cerr << "Warning: stdlib module '" << module_name
                          << "' (" << entry.path << ") failed to load [eval]: "
                          << result.error().what() << std::endl;
                module_failed = true;
                break;
            }
        }

        if (module_failed) {
            continue;
        }
    }
}

}  // namespace pml
