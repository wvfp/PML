add_library(pml_core
    call_stack.cpp
    error.cpp
    source_manager.cpp
    types.cpp
)

target_include_directories(pml_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
target_link_libraries(pml_core PUBLIC nlohmann_json::nlohmann_json)
