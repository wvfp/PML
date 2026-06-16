enable_testing()

# ── GTest test runner ───────────────────────────────────────────────────────
add_executable(pml-tests
    stub.cpp
)

target_include_directories(pml-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml-tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    pml_core
    pml_frontend
    pml_evaluator
    pml_graphics
    pml_sprites
    pml_animation
    pml_skeleton
    pml_module
)

# ── Builtins smoke test (standalone, no GTest) ──────────────────────────────
add_executable(pml-builtins-smoke
    builtins_smoke.cpp
)

target_include_directories(pml-builtins-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml-builtins-smoke PRIVATE
    pml_core
    pml_frontend
    pml_evaluator
    pml_module
)

# Discover tests via CTest
include(GoogleTest)
gtest_discover_tests(pml-tests)
