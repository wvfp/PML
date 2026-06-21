// ═══════════════════════════════════════════════════════════════════════════════
// PML CLI — Entry Point
//
// Ported from pml/repl.py.  Provides:
//   - File execution:      pml file.pml
//   - Interactive REPL:    pml
//   - Watch mode:          pml file.pml --watch
//   - JSON output:         pml file.pml --json
//   - Output directory:    pml file.pml -o ./output
//   - GIF animation:       pml file.pml --gif
//
// Execution modes are split into:
//   cli_execute.cpp  — file / JSON / REPL modes
//   cli_watch.cpp    — watch mode (platform-specific)
//   cli_errors.cpp   — error formatting
// ═══════════════════════════════════════════════════════════════════════════════

#include "cli_shared.h"
#include "cli_errors.h"
#include "cli_execute.h"
#include "cli_watch.h"

#include "pml/api/api.h"
#include "pml/layer/layer_builtins.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

// Global source cache for file-mode error snippets.
pml::SourceManager g_source_manager;

// ═══════════════════════════════════════════════════════════════════════════════
// Argument parsing
// ═══════════════════════════════════════════════════════════════════════════════

static CLIOptions parse_args(int argc, char* argv[])
{
    CLIOptions opts;
    bool file_seen = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            opts.help = true;
        } else if (arg == "-o" || arg == "--output-dir") {
            if (i + 1 < argc) {
                opts.output_dir = argv[++i];
            } else {
                std::cerr << "Error: --output-dir (-o) requires a directory path argument."
                          << std::endl;
                std::exit(1);
            }
        } else if (arg == "-w" || arg == "--watch") {
            opts.watch = true;
        } else if (arg == "--json") {
            opts.json = true;
        } else if (arg == "--gif") {
            opts.gif = true;
        } else if (arg[0] == '-') {
            std::cerr << "Error: unknown option: " << arg << std::endl;
            std::exit(1);
        } else if (!file_seen) {
            opts.file = arg;
            file_seen = true;
        } else {
            std::cerr << "Error: unexpected argument: " << arg << std::endl;
            std::exit(1);
        }
    }

    return opts;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Help output
// ═══════════════════════════════════════════════════════════════════════════════

static void print_help(const char* prog)
{
    std::printf(
        "PML — Picture Markup Language v0.1.0\n"
        "\n"
        "Usage: %s [file] [options]\n"
        "\n"
        "Arguments:\n"
        "  file                    PML source file to execute (omit for REPL)\n"
        "\n"
        "Options:\n"
        "  -o, --output-dir DIR    Output directory for rendered files\n"
        "  -w, --watch             Watch file for changes and re-execute\n"
        "      --json              Output results as JSON\n"
        "      --gif               Enable GIF animation export\n"
        "  -h, --help              Show this help message\n"
        "\n"
        "Examples:\n"
        "  %s                      Start interactive REPL\n"
        "  %s hello.pml            Execute hello.pml\n"
        "  %s hello.pml -o ./out   Execute and render to ./out/\n"
        "  %s hello.pml --watch    Watch and re-execute on changes\n"
        "  %s hello.pml --json     Output execution results as JSON\n",
        prog, prog, prog, prog, prog, prog);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Main entry point
// ═══════════════════════════════════════════════════════════════════════════════

int main(int argc, char* argv[])
{
    auto opts = parse_args(argc, argv);

    if (opts.help) {
        print_help(argv[0]);
        return 0;
    }

    // Create the PML runtime
    pml::PMLRuntime runtime;

    if (opts.file.empty()) {
        // REPL mode
        return pml::run_repl_mode(runtime);
    }

    if (opts.watch) {
        // Watch mode — auto-rerun on file change
        return pml::run_watch_mode(opts, runtime);
    }

    if (opts.json) {
        // JSON output mode
        return pml::run_json_mode(opts, runtime);
    }

    // Default file execution mode
    return pml::run_file_mode(opts, runtime);
}
