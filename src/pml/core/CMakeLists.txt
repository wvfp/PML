add_library(pml_core
    error.cpp
    types.cpp
)

target_include_directories(pml_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml_core PUBLIC nlohmann_json::nlohmann_json)
