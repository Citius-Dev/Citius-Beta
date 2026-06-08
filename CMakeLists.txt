cmake_minimum_required(VERSION 3.10)
project(Citius LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(SOURCES
    src/main.cpp
    src/Basic/Token.cpp
    src/Basic/Diagnostic.cpp
    src/Lexer/Lexer.cpp
    src/AST/AST.cpp
    src/Parser/Parser.cpp
    src/Sema/Sema.cpp
    src/CodeGen/CodeGen.cpp
)

set(HEADERS
    src/Basic/Token.h
    src/Basic/TokenKinds.h
    src/Basic/Diagnostic.h
    src/Basic/SourceLocation.h
    src/Lexer/Lexer.h
    src/AST/AST.h
    src/Parser/Parser.h
    src/Sema/Sema.h
    src/CodeGen/CodeGen.h
)

add_executable(citius ${SOURCES} ${HEADERS})
target_include_directories(citius PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
