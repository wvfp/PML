add_library(pml_core
    call_stack.cpp
    error.cpp
    source_manager.cpp
    types.cpp
    texture_cache.cpp
)

target_include_directories(pml_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
target_link_libraries(pml_core PUBLIC nlohmann_json::nlohmann_json)

# texture_cache.cpp needs Skia headers for sk_sp<SkImage> (PRIVATE: header
# only forward-declares SkImage, so downstream targets don't need this).
if(PML_USE_SKIA AND TARGET skia_headers)
    target_link_libraries(pml_core PRIVATE skia_headers)
endif()
