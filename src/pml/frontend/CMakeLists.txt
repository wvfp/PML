add_library(pml_frontend
    lexer.cpp
    parser.cpp
    expander.cpp
)

target_include_directories(pml_frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(pml_frontend PRIVATE ${PROJECT_SOURCE_DIR}/src/pml/evaluator)
target_link_libraries(pml_frontend PUBLIC pml_core)
