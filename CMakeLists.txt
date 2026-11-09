cmake_minimum_required(VERSION 3.18)
project(ssa3_bench LANGUAGES CXX)

# ------------------------------------------------------------------
# Language standards
# ------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# ------------------------------------------------------------------
# Main executable
# ------------------------------------------------------------------
add_executable(${PROJECT_NAME}
    main.cpp
)

execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    "-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\"")

# ------------------------------------------------------------------
# SRP library (FetchContent)
# ------------------------------------------------------------------
include(FetchContent)

# set(SM3D_ENABLE_CUDA ON CACHE BOOL "" FORCE)
FetchContent_Declare(SRP
    GIT_REPOSITORY https://github.com/paulzap/Real-Time-Solar-Radiation-Pressure-Modeling.git
    GIT_TAG        v1.2.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SRP)

# CUDA settings for the static library + final executable
if(SM3D_HAS_CUDA)
    enable_language(CUDA)

    set_target_properties(srp PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CUDA_STANDARD 20
        CUDA_STANDARD_REQUIRED ON
        CUDA_SEPARABLE_COMPILATION OFF
        CUDA_RESOLVE_DEVICE_SYMBOLS OFF
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE CUDA::cudart)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE srp)

#[[
# ------------------------------------------------------------------
# CSPICE / SOFA
# ------------------------------------------------------------------
if(WIN32)
    set(SSA3_CSPICE "${CMAKE_CURRENT_SOURCE_DIR}/libs/cspice.lib")
    set(SSA3_SOFA   "${CMAKE_CURRENT_SOURCE_DIR}/libs/sofa_lib.lib")
else()
    set(SSA3_CSPICE "${CMAKE_CURRENT_SOURCE_DIR}/libs/cspice.a")
    set(SSA3_SOFA   "${CMAKE_CURRENT_SOURCE_DIR}/libs/libsofa_c.a")
endif()

foreach(_lib ${SSA3_CSPICE} ${SSA3_SOFA})
    if(NOT EXISTS ${_lib})
        message(FATAL_ERROR "Required library not found: ${_lib}")
    endif()
endforeach()

target_link_libraries(${PROJECT_NAME} PRIVATE ${SSA3_CSPICE} ${SSA3_SOFA})

set(SSA3_SOFA_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/libs/sofa")
set(SSA3_CSPICE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/cspice/include")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SSA3_SOFA_DIR}
    ${SSA3_CSPICE_DIR}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    SSA3_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Files"
    SSA3_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
]]

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /bigobj)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

# ----------------------------------------------------------------
# CLI11
# ----------------------------------------------------------------
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.7.2          # current stable release (recommended)
    GIT_SHALLOW    TRUE            # faster download
    QUIET
)

FetchContent_MakeAvailable(CLI11)

target_link_libraries(${PROJECT_NAME} PRIVATE CLI11::CLI11)