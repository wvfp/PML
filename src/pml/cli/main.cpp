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
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/api/api.h"
#include "pml/api/context.h"
#include "pml/core/error.h"
#include "pml/core/source_manager.h"
#include "pml/layer/layer_builtins.h"
#include "pml/module/embedded_stdlib.h"
#include "repl.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

// Global source cache for file-mode error snippets.
static pml::SourceManager g_source_manager;

// ═══════════════════════════════════════════════════════════════════════════════
// Forward declarations (defined below)
// ═══════════════════════════════════════════════════════════════════════════════

struct CLIOptions;
static CLIOptions parse_args(int argc, char* argv[]);
static int run_file_mode(const CLIOptions& opts);
static int run_repl_mode(const CLIOptions& opts);
static int run_watch_mode(const CLIOptions& opts);
static int run_json_mode(const CLIOptions& opts);
static void print_help(const char* prog);
static void print_error(const pml::PMLException& err);
static void print_json_error(const pml::PMLException& err);
static nlohmann::json error_to_json(const pml::PMLException& e);
static nlohmann::json error_to_json(int code, const std::string& type,
                                     const std::string& message);

// ═══════════════════════════════════════════════════════════════════════════════
// CLI options struct
// ═══════════════════════════════════════════════════════════════════════════════

struct CLIOptions {
    std::string file;             ///< File to execute (empty = REPL mode)
    std::string output_dir;       ///< -o / --output-dir
    bool watch{false};            ///< -w / --watch
    bool json{false};             ///< --json
    bool gif{false};              ///< --gif
    bool help{false};             ///< -h / --help
};

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
// Output directory handling
// ═══════════════════════════════════════════════════════════════════════════════

static void setup_output_dir(const std::string& dir)
{
    if (dir.empty()) return;

    // Create the directory (including parents)
    fs::path out_path(dir);
    std::error_code ec;
    fs::create_directories(out_path, ec);
    if (ec) {
        std::cerr << "Warning: could not create output directory '"
                  << dir << "': " << ec.message() << std::endl;
        return;
    }

    // Change to the output directory so render paths resolve relative to it
    fs::current_path(fs::absolute(out_path), ec);
    if (ec) {
        std::cerr << "Warning: could not change to output directory '"
                  << dir << "': " << ec.message() << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Error formatting
// ═══════════════════════════════════════════════════════════════════════════════

static void print_error(const pml::PMLException& err)
{
    // Format with source snippet, line number and caret when available.
    std::cerr << pml::format_error_with_source(err, g_source_manager);
}

/// Convert a structured RenderResult error JSON back to a printable message.
/// Loads the source file into `g_source_manager` so snippets can be shown.
static void print_render_error(const nlohmann::json& err)
{
    pml::PMLException exc;
    exc.code = pml::ErrorCode::GeneralError;
    exc.message = err.value("message", "Unknown error");

    if (err.contains("type")) {
        const std::string type = err.value("type", "GeneralError");
        if (type == "PMLSyntaxError")    exc.code = pml::ErrorCode::PMLSyntaxError;
        else if (type == "PMLTypeError") exc.code = pml::ErrorCode::PMLTypeError;
        else if (type == "UnboundVariableError")
            exc.code = pml::ErrorCode::UnboundVariableError;
        else if (type == "ArityError")   exc.code = pml::ErrorCode::ArityError;
        else if (type == "ImportError")  exc.code = pml::ErrorCode::ImportError;
        else if (type == "CircularImportError")
            exc.code = pml::ErrorCode::CircularImportError;
        else if (type == "MacroExpansionDepthError")
            exc.code = pml::ErrorCode::MacroExpansionDepthError;
        else if (type == "AccessError")  exc.code = pml::ErrorCode::AccessError;
        else if (type == "ResourceError") exc.code = pml::ErrorCode::ResourceError;
        else if (type == "IKNoSolutionError")
            exc.code = pml::ErrorCode::IKNoSolutionError;
        else if (type == "PMLAssertionError")
            exc.code = pml::ErrorCode::PMLAssertionError;
    }

    if (err.contains("line")) {
        pml::SourceLocation loc;
        loc.line = err.value("line", 0);
        loc.column = err.value("column", 0);
        loc.filename = err.value("filename", "");
        exc.location = std::move(loc);
    }
if (err.contains("hint") && !err["hint"].is_null()) {
        exc.repair_hint = err.value("hint", "");
    }

    if (err.contains("call_stack") && err["call_stack"].is_array()) {
        for (const auto& frame : err["call_stack"]) {
            pml::CallFrame f;
            f.function_name = frame.value("function", "");
            // Parse location string "filename:line:col" or "line line:col"
            std::string loc_str = frame.value("location", "");
            if (!loc_str.empty()) {
                auto last_colon = loc_str.find_last_of(':');
                if (last_colon != std::string::npos) {
                    auto prev_colon = loc_str.find_last_of(':', last_colon - 1);
                    try {
                        if (prev_colon != std::string::npos) {
                            f.call_site.filename = loc_str.substr(0, prev_colon);
                            f.call_site.line = std::stoi(loc_str.substr(prev_colon + 1, last_colon - prev_colon - 1));
                            f.call_site.column = std::stoi(loc_str.substr(last_colon + 1));
                        } else {
                            f.call_site.line = std::stoi(loc_str.substr(0, last_colon));
                            f.call_site.column = std::stoi(loc_str.substr(last_colon + 1));
                        }
                    } catch (...) {
                        // Ignore parse errors; leave location empty.
                    }
                }
            }
            exc.call_stack.push_back(std::move(f));
        }
    }

    if (err.contains("details") && err["details"].is_array()) {
        std::cerr << pml::format_error_with_source(exc, g_source_manager);
        int index = 1;
        for (const auto& detail : err["details"]) {
            std::cerr << "\n  --- Error " << index++ << " ---\n";
            print_render_error(detail);
        }
    } else {
        std::cerr << pml::format_error_with_source(exc, g_source_manager);
    }
}

static nlohmann::json error_to_json(const pml::PMLException& e)
{
    nlohmann::json j;
    j["type"] = std::string(pml::to_string(e.code));
    j["message"] = e.message;
    if (e.location.has_value()) {
        const auto& loc = *e.location;
        if (loc.line > 0)   j["line"] = loc.line;
        if (loc.column > 0) j["column"] = loc.column;
        if (!loc.filename.empty()) j["filename"] = loc.filename;
    }
    j["hint"] = e.repair_hint.has_value() ? nlohmann::json(*e.repair_hint) : nlohmann::json(nullptr);
    return j;
}

static nlohmann::json error_to_json(int /*code*/, const std::string& type,
                                     const std::string& message)
{
    nlohmann::json j;
    j["type"] = type;
    j["message"] = message;
    return j;
}

static void print_json_error(const pml::PMLException& err)
{
    nlohmann::json j;
    j["success"] = false;
    j["result"] = nullptr;
    j["files"] = nlohmann::json::array();
    j["error"] = error_to_json(err);
    std::cout << j.dump(2) << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════════
// File execution mode
// ═══════════════════════════════════════════════════════════════════════════════

static int run_file_mode(const CLIOptions& opts, pml::PMLRuntime& runtime)
{
    // Pre-load source so error messages can show source snippets.
    g_source_manager.load_file(opts.file);

    auto result = runtime.execute_file(opts.file);

    if (!result.success) {
        if (result.error.has_value()) {
            const auto& err = *result.error;
            std::string type = err.value("type", "Error");
            std::string msg = err.value("message", "Unknown error");

            // Check if it's a file-not-found type of error
            if (type == "ResourceError" &&
                msg.find("Cannot open file") != std::string::npos) {
                std::cerr << "Error: file not found: " << opts.file << std::endl;
            } else {
                print_render_error(err);
            }
        } else {
            std::cerr << "Error: execution failed" << std::endl;
        }
        return 1;
    }

    // Auto-render any registered compositions when an output directory is set.
    if (!opts.output_dir.empty()) {
        for (const auto& comp : runtime.compositions()) {
            if (!comp) continue;
            auto render_result = pml::render_composition_to_disk(*comp, opts.output_dir);
            if (!render_result) {
                print_render_error(pml::error_to_dict(render_result.error()));
                return 1;
            }
        }
    }

    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════════
// JSON output mode
// ═══════════════════════════════════════════════════════════════════════════════

static int run_json_mode(const CLIOptions& opts, pml::PMLRuntime& runtime)
{
    nlohmann::json result;

    try {
        auto r = runtime.execute_file(opts.file);
        if (r.success) {
            result["success"] = true;
            result["value"] = r.value;
            result["files"] = r.files;
            result["error"] = nullptr;
        } else {
            result["success"] = false;
            result["value"] = nullptr;
            result["files"] = nlohmann::json::array();
            if (r.error.has_value()) {
                result["error"] = *r.error;
            } else {
                result["error"] = error_to_json(0, "UnknownError", "Execution failed");
            }
        }
    } catch (const std::exception& e) {
        result["success"] = false;
        result["value"] = nullptr;
        result["files"] = nlohmann::json::array();
        result["error"] = error_to_json(0, "InternalError", e.what());
    }

    std::cout << result.dump(2) << std::endl;

    if (!result["success"].get<bool>()) {
        return 1;
    }
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Watch mode (Linux inotify)
// ═══════════════════════════════════════════════════════════════════════════════

#ifdef __linux__
#include <sys/inotify.h>
#include <unistd.h>
#include <climits>
#include <cstring>

static int run_watch_mode(const CLIOptions& opts, pml::PMLRuntime& runtime)
{
    // Resolve to absolute path for inotify
    fs::path abs_path = fs::absolute(opts.file);
    std::string filename = abs_path.filename().string();
    std::string dirname  = abs_path.parent_path().string();

    if (!fs::exists(abs_path)) {
        std::cerr << "Error: file not found: " << opts.file << std::endl;
        return 1;
    }

    // Initial run
    std::cout << "Watching " << opts.file << " for changes... (Ctrl-C to stop)"
              << std::endl;
    g_source_manager.load_file(opts.file);
    auto result = runtime.execute_file(opts.file);
    if (!result.success) {
        if (result.error.has_value()) {
            print_render_error(*result.error);
        }
    }

    // Set up inotify
    int inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        std::cerr << "Error: failed to initialize inotify: "
                  << std::strerror(errno) << std::endl;
        return 1;
    }

    int wd = inotify_add_watch(inotify_fd, dirname.c_str(), IN_CLOSE_WRITE);
    if (wd < 0) {
        std::cerr << "Error: failed to watch directory '" << dirname << "': "
                  << std::strerror(errno) << std::endl;
        close(inotify_fd);
        return 1;
    }

    // Event buffer: read up to 64 events at once
    constexpr size_t BUF_SIZE = 64 * (sizeof(struct inotify_event) + NAME_MAX + 1);
    char buf[BUF_SIZE];

    while (true) {
        ssize_t len = read(inotify_fd, buf, BUF_SIZE);
        if (len < 0) {
            if (errno == EINTR) {
                // Interrupted by signal (e.g. Ctrl-C from another thread)
                continue;
            }
            std::cerr << "Error: inotify read failed: "
                      << std::strerror(errno) << std::endl;
            break;
        }

        // Process events
        for (char* ptr = buf; ptr < buf + len; ) {
            auto* event = reinterpret_cast<struct inotify_event*>(ptr);

            // Check if the modified file is the one we're watching
            if (event->len > 0 && filename == event->name) {
                // Small delay to let the file finish writing
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // Re-run
                auto now = std::time(nullptr);
                char time_str[32];
                std::strftime(time_str, sizeof(time_str), "%H:%M:%S",
                              std::localtime(&now));
                std::cout << "\n--- " << time_str << " ---" << std::endl;

                g_source_manager.load_file(opts.file);
                auto new_result = runtime.execute_file(opts.file);
                if (!new_result.success) {
                    if (new_result.error.has_value()) {
                        print_render_error(*new_result.error);
                    }
                }
            }

            ptr += sizeof(struct inotify_event) + event->len;
        }
    }

    close(inotify_fd);
    return 0;
}

#elif defined(_WIN32)
// Windows native directory change notification (avoids mtime polling)
#include <windows.h>
#include <ctime>

static int run_watch_mode(const CLIOptions& opts, pml::PMLRuntime& runtime)
{
    fs::path abs_path = fs::absolute(opts.file);
    std::string filename = abs_path.filename().string();
    std::string dirname  = abs_path.parent_path().string();

    if (!fs::exists(abs_path)) {
        std::cerr << "Error: file not found: " << opts.file << std::endl;
        return 1;
    }

    // Initial run
    std::cout << "Watching " << opts.file << " for changes... (Ctrl-C to stop)"
              << std::endl;
    g_source_manager.load_file(opts.file);
    auto init_result = runtime.execute_file(opts.file);
    if (!init_result.success && init_result.error.has_value()) {
        print_render_error(*init_result.error);
    }

    auto last_mtime = fs::last_write_time(abs_path);

    HANDLE hChange = FindFirstChangeNotificationA(
        dirname.c_str(),
        FALSE,  // do not watch subtree
        FILE_NOTIFY_CHANGE_LAST_WRITE);

    if (hChange == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: failed to start directory watch for '"
                  << dirname << "'" << std::endl;
        return 1;
    }

    while (true) {
        DWORD wait_status = WaitForSingleObject(hChange, 1000);
        if (wait_status == WAIT_OBJECT_0) {
            // A file in the directory changed; verify it was the watched file.
            try {
                auto mtime = fs::last_write_time(abs_path);
                if (mtime != last_mtime) {
                    last_mtime = mtime;

                    auto now = std::time(nullptr);
                    char time_str[32];
                    std::strftime(time_str, sizeof(time_str), "%H:%M:%S",
                                  std::localtime(&now));
                    std::cout << "\n--- " << time_str << " ---" << std::endl;

                    g_source_manager.load_file(opts.file);
                    auto result = runtime.execute_file(opts.file);
                    if (!result.success && result.error.has_value()) {
                        print_render_error(*result.error);
                    }
                }
            } catch (const fs::filesystem_error&) {
                // File might be temporarily unavailable
            }

            if (!FindNextChangeNotification(hChange)) {
                std::cerr << "Error: FindNextChangeNotification failed"
                          << std::endl;
                break;
            }
        } else if (wait_status == WAIT_FAILED) {
            std::cerr << "Error: WaitForSingleObject failed" << std::endl;
            break;
        }
    }

    FindCloseChangeNotification(hChange);
    return 0;
}

#else
// Fallback for non-Linux, non-Windows: poll-based watch using file modification time
#include <ctime>

static int run_watch_mode(const CLIOptions& opts, pml::PMLRuntime& runtime)
{
    if (!fs::exists(opts.file)) {
        std::cerr << "Error: file not found: " << opts.file << std::endl;
        return 1;
    }

    // Initial run
    std::cout << "Watching " << opts.file << " for changes... (Ctrl-C to stop)"
              << std::endl;
    g_source_manager.load_file(opts.file);
    auto init_result = runtime.execute_file(opts.file);
    if (!init_result.success && init_result.error.has_value()) {
        print_render_error(*init_result.error);
    }

    auto last_mtime = fs::last_write_time(opts.file);

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        try {
            auto mtime = fs::last_write_time(opts.file);
            if (mtime != last_mtime) {
                last_mtime = mtime;

                auto now = std::time(nullptr);
                char time_str[32];
                std::strftime(time_str, sizeof(time_str), "%H:%M:%S",
                              std::localtime(&now));
                std::cout << "\n--- " << time_str << " ---" << std::endl;

                g_source_manager.load_file(opts.file);
                auto result = runtime.execute_file(opts.file);
                if (!result.success && result.error.has_value()) {
                    print_render_error(*result.error);
                }
            }
        } catch (const fs::filesystem_error&) {
            // File might be temporarily unavailable
            continue;
        }
    }
}
#endif  // __linux__

// ═══════════════════════════════════════════════════════════════════════════════
// REPL mode
// ═══════════════════════════════════════════════════════════════════════════════

static int run_repl_mode(pml::PMLRuntime& runtime)
{
    // Load embedded standard library modules into the global environment
    auto env = runtime.env();
    pml::load_embedded_stdlib(env);

    // Enter interactive REPL
    pml::run_repl(env);
    return 0;
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
        "  -w, --watch             Watch source file for changes and re-run\n"
        "      --json              Output structured JSON result to stdout\n"
        "      --gif               Force GIF animation output\n"
        "  -h, --help              Show this help message and exit\n"
        "\n"
        "Examples:\n"
        "  %s script.pml           Execute a PML file\n"
        "  %s                      Start interactive REPL\n"
        "  %s script.pml -o ./out  Execute with output directory\n"
        "  %s script.pml --watch   Watch file for changes\n"
        "  %s script.pml --json    Output JSON result\n",
        prog, prog, prog, prog, prog, prog);
}

// ═══════════════════════════════════════════════════════════════════════════════
// main()
// ═══════════════════════════════════════════════════════════════════════════════

int main(int argc, char* argv[])
{
    auto opts = parse_args(argc, argv);

    // Help
    if (opts.help) {
        print_help(argv[0]);
        return 0;
    }

    // Resolve paths before we change the working directory for the output
    // directory. This allows relative input/output paths to work together.
    if (!opts.file.empty()) {
        opts.file = fs::absolute(opts.file).string();
    }
    if (!opts.output_dir.empty()) {
        opts.output_dir = fs::absolute(opts.output_dir).string();
    }

    // Setup output directory
    setup_output_dir(opts.output_dir);

    // Create the PML runtime (loads builtins)
    pml::PMLRuntime runtime;

    // Dispatch based on mode
    if (opts.file.empty()) {
        // REPL mode
        return run_repl_mode(runtime);
    }

    if (opts.json) {
        return run_json_mode(opts, runtime);
    }

    if (opts.watch) {
        return run_watch_mode(opts, runtime);
    }

    // Default: file execution mode
    return run_file_mode(opts, runtime);
}
