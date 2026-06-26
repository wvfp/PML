#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Platform Abstraction
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Centralises platform-specific code (Windows vs POSIX) so individual source
// files don't need to sprinkle #ifdef _WIN32 everywhere.
// ==========================================================================================================================================================================================================================================═

#include <cerrno>
#include <cstdio>

namespace pml {

// ---- fopen wrapper --------------------------------------------------------------------------------------------------------------------
//
// MSVC's fopen_s is in the C11 annex K and is not portable.  Wrap both paths
// so callers use a single convention.
//
// Usage:
//   FILE* fp = nullptr;
//   if (pml_fopen(fp, path, "wb")) { /* error */ }

#ifdef _WIN32

inline int pml_fopen(FILE*& fp, const char* path, const char* mode) noexcept {
    errno = 0;
    return fopen_s(&fp, path, mode);
}

#else

inline int pml_fopen(FILE*& fp, const char* path, const char* mode) noexcept {
    fp = fopen(path, mode);
    return fp ? 0 : errno;
}

#endif

// ---- <format> polyfill note ------------------------------------------------------------------------------------------------─
//
// C++23 <format> is available on:
//   - MSVC 2022 17.0+
//   - GCC 14+ / libstdc++ 14+
//   - Clang 18+ / libc++ 19+
//
// If building with an older toolchain, define PML_NO_FORMAT and provide
// a fmt::format fallback via the {fmt} library.
//
// Currently all supported toolchains have <format> natively.

} // namespace pml
