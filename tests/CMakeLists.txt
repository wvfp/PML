enable_testing()

# ---- GTest test runner ------------------------------------------------------------------------------------------------------------─
add_executable(pml-tests
    stub.cpp
    test_lexer.cpp
    test_parser.cpp
    test_evaluator.cpp
    test_graphics.cpp
    test_skeleton.cpp
    test_api.cpp
    test_modules.cpp
    test_animation.cpp
    test_layer.cpp
    test_asset.cpp
    test_polygon_perturb.cpp
    test_stdlib_macros.cpp
)

target_include_directories(pml-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml-tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    pml_core
    pml_frontend
    pml_evaluator
    pml_graphics
    pml_graphics3d
    pml_backend
    pml_layer
    pml_filter
    pml_asset
    pml_sprites
    pml_animation
    pml_skeleton
    pml_module
    pml_api
)

# ---- Builtins smoke test (standalone, no GTest) ------------------------------------------------------------
add_executable(pml-builtins-smoke
    builtins_smoke.cpp
)

target_include_directories(pml-builtins-smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml-builtins-smoke PRIVATE
    pml_core
    pml_frontend
    pml_evaluator
    pml_graphics
    pml_graphics3d
    pml_backend
    pml_layer
    pml_asset
    pml_module
    pml_api
)

# ---- Layer unit tests (isolated from pml_sprites build issues) --------------------------------
add_executable(pml-layer-test
    test_layer.cpp
    test_layer_render.cpp
    test_spritesheet.cpp
)

target_include_directories(pml-layer-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml-layer-test PRIVATE
    GTest::gtest
    GTest::gtest_main
    pml_core
    pml_graphics
    pml_backend
    pml_layer
    pml_filter
)

# ---- Filter unit tests ----------------------------------------------------------------------------------------------------------------
add_executable(pml-filter-test
    test_filter.cpp
)

target_include_directories(pml-filter-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml-filter-test PRIVATE
    GTest::gtest
    GTest::gtest_main
    pml_core
    pml_graphics
    pml_backend
    pml_layer
    pml_filter
)

# Register tests via CTest (runs from repo root so relative paths resolve correctly)
include(GoogleTest)
gtest_discover_tests(pml-tests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
gtest_discover_tests(pml-layer-test PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
gtest_discover_tests(pml-filter-test PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Smoke test (standalone, no GTest discovery)
add_test(NAME pml-builtins-smoke COMMAND pml-builtins-smoke WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
