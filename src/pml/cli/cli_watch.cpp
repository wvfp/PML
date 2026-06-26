// ==========================================================================================================================================================================================================================================═
// PML CLI — Watch mode (platform-specific)
//
// Linux:    inotify
// Windows:  FindFirstChangeNotification
// Other:    poll-based (file modification time)
//
// Ported from pml/repl.py.
// ==========================================================================================================================================================================================================================================═

#include "cli_watch.h"
#include "cli_shared.h"
#include "cli_errors.h"

#include "pml/api/api.h"

#include <iostream>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

// ==========================================================================================================================================================================================================================================═
// Output directory handling
// ==========================================================================================================================================================================================================================================═

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

// ==========================================================================================================================================================================================================================================═
// Watch mode (Linux inotify)
// ==========================================================================================================================================================================================================================================═

#ifdef __linux__
#include <sys/inotify.h>
#include <unistd.h>
#include <climits>
#include <cstring>

namespace pml {
int run_watch_mode(const CLIOptions& opts, PMLRuntime& runtime)
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
            pml::print_render_error(*result.error);
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
                        pml::print_render_error(*new_result.error);
                    }
                }
            }

            ptr += sizeof(struct inotify_event) + event->len;
        }
    }

    close(inotify_fd);
    return 0;
}
} // namespace pml

#elif defined(_WIN32)
// Windows native directory change notification (avoids mtime polling)
#include <windows.h>
#include <ctime>

namespace pml {
int run_watch_mode(const CLIOptions& opts, PMLRuntime& runtime)
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
        pml::print_render_error(*init_result.error);
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
                        pml::print_render_error(*result.error);
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
} // namespace pml

#else
// Fallback for non-Linux, non-Windows: poll-based watch using file modification time
#include <ctime>

namespace pml {
int run_watch_mode(const CLIOptions& opts, PMLRuntime& runtime)
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
        pml::print_render_error(*init_result.error);
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
                    pml::print_render_error(*result.error);
                }
            }
        } catch (const fs::filesystem_error&) {
            // File might be temporarily unavailable
            continue;
        }
    }
}
} // namespace pml
#endif  // __linux__
