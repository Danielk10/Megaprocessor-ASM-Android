cmake_minimum_required(VERSION 3.16)
project(megaprocessor_assembler_cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(assembler-cli
    main.cpp
    ../../app/src/main/cpp/assembler.cpp
    ../../app/src/main/cpp/utils.cpp
)

target_include_directories(assembler-cli PRIVATE
    ../../app/src/main/cpp
)

target_compile_definitions(assembler-cli PRIVATE
    PROJECT_ROOT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
