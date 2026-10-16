cmake_minimum_required(VERSION 3.23)

project(KingCore
    VERSION 1.0.0.0
    DESCRIPTION "High-Performance Unified Modding Framework for Skyrim"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# CommonLibSSE-NG (prefer the vcpkg package from vcpkg.json; allow a source tree
# override for development checkouts).
find_package(CommonLibSSE CONFIG QUIET)
if(TARGET CommonLibSSE::CommonLibSSE)
    set(KINGCORE_COMMONLIBSSE_TARGET CommonLibSSE::CommonLibSSE)
else()
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(COMMONLIBSSE_NG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../CommonLibSSE-NG-3.7.0/CommonLibSSE-NG-3.7.0"
        CACHE PATH "Path to the CommonLibSSE-NG source tree")
    if(NOT EXISTS "${COMMONLIBSSE_NG_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "CommonLibSSE-NG package was not found and no source tree exists at "
            "'${COMMONLIBSSE_NG_DIR}'. Enable the vcpkg manifest or set COMMONLIBSSE_NG_DIR.")
    endif()
    add_subdirectory("${COMMONLIBSSE_NG_DIR}" CommonLibSSE-NG)
    set(KINGCORE_COMMONLIBSSE_TARGET CommonLibSSE)
endif()

# Core Plugin Library
add_library(${PROJECT_NAME} SHARED
    src/Main.cpp
    src/CoreManager.cpp
    src/CoreManager.h
    src/StatusUI.cpp
    src/StatusUI.h
    src/PrismaUI_API.h
    include/KingCore/KingCoreAPI.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${KINGCORE_COMMONLIBSSE_TARGET}
    spdlog::spdlog
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_include_directories(${PROJECT_NAME} PUBLIC
    include
)
target_include_directories(${PROJECT_NAME} PRIVATE
    src
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX- /EHsc /permissive->
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    SKSE_SUPPORT_XBYAK=1
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Data"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Data"
    COMMENT "Copying King Core PrismaUI view assets"
)
