
cmake_minimum_required(VERSION 3.16)

project(OptiLang VERSION 1.0 LANGUAGES CXX)

# ==========================================
# 1. C++ CONFIGURATION
# ==========================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ==========================================
# 2. FLEX AND BISON
# ==========================================

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(OptiLangParser
    "${CMAKE_CURRENT_SOURCE_DIR}/src/parser.y"
    "${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cpp"
    DEFINES_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/parser.tab.hpp"
)

FLEX_TARGET(OptiLangLexer
    "${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.l"
    "${CMAKE_CURRENT_BINARY_DIR}/lexer.yy.cpp"
)

# Ensure Bison's generated header is available
# when compiling the Flex scanner.
ADD_FLEX_BISON_DEPENDENCY(
    OptiLangLexer
    OptiLangParser
)

# ==========================================
# 3. FIND GLPK
# ==========================================

find_path(GLPK_INCLUDE_DIR NAMES glpk.h)
find_library(GLPK_LIBRARY NAMES glpk)

if(NOT GLPK_INCLUDE_DIR OR NOT GLPK_LIBRARY)
    message(FATAL_ERROR
        "GLPK was not found. On Arch/Omarchy install "
        "it with: sudo pacman -S --needed glpk"
    )
endif()

add_library(GLPK::GLPK UNKNOWN IMPORTED)

set_target_properties(GLPK::GLPK PROPERTIES
    IMPORTED_LOCATION "${GLPK_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GLPK_INCLUDE_DIR}"
)

# ==========================================
# 4. COMPILER CORE LIBRARY
# ==========================================

add_library(optilang_core STATIC
    src/symbol_table_builder.cpp
    src/semantic_analyzer.cpp
    src/linear_ir.cpp
    src/glpk_solver.cpp
)

target_include_directories(optilang_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(optilang_core PUBLIC
    GLPK::GLPK
)

# ==========================================
# 5. MAIN COMPILER EXECUTABLE
# ==========================================

add_executable(optilang
    src/parser_driver.cpp
    ${BISON_OptiLangParser_OUTPUTS}
    ${FLEX_OptiLangLexer_OUTPUTS}
)

target_include_directories(optilang PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(optilang PRIVATE
    optilang_core
)

# ==========================================
# 6. AUTOMATED TESTING
# ==========================================

include(CTest)

if(BUILD_TESTING)

    # Helper for registering C++ tests.
    function(optilang_add_test name source_file)

        add_executable(${name} "${source_file}")

        target_link_libraries(${name} PRIVATE
            optilang_core
        )

        # Existing tests use assert().
        # Keep assertions active in Release builds.
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${name} PRIVATE
                -UNDEBUG
            )
        endif()

        add_test(NAME ${name} COMMAND ${name})

    endfunction()

    # AST and Symbol Table test
    optilang_add_test(test_ast
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/ast_symbol_demo.cpp"
    )

    # Symbol Table Builder test
    optilang_add_test(test_symbols
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_symbol_table_builder.cpp"
    )

    # Intermediate Representation test
    optilang_add_test(test_ir
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_linear_ir.cpp"
    )

    # GLPK Solver test
    optilang_add_test(test_glpk
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_glpk_solver.cpp"
    )

    include(tests/cli_cases.cmake)

endif()

