add_executable(pml-mcp
    mcp_server.cpp
)

target_include_directories(pml-mcp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..   # for pml/mcp/mcp_server.h → pml/api/api.h etc.
)

target_link_libraries(pml-mcp PRIVATE
    pml_core
    pml_frontend
    pml_evaluator
    pml_graphics
    pml_layer
    pml_sprites
    pml_animation
    pml_skeleton
    pml_module
    pml_api
    nlohmann_json::nlohmann_json
)
install(TARGETS pml-mcp DESTINATION bin)
