########################################################################
## Create RPX-100 project
########################################################################
cmake_minimum_required(VERSION 3.24.x)
project("RPX-100" DESCRIPTION "Open SDR Platform" LANGUAGES CXX C)


########################################################################
## compiler flags
########################################################################

## C++17 is a required language feature for this project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

## optimizing must be looked at in more details - especially what is helping on complex math and fft
# add_definitions(-march=native)
# add_definitions(-mfpmath=sse)

## Set CMAKE Policy
cmake_policy(SET CMP0135 NEW)

## set build type to distinguish DEBUG, RELEASE and CPE, BASE
## IMPORTANT this must stay on Release for the moment as it affects the external libraries especially limesuite
## which has zero performance if it is running in DEBUG mode
set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE CPE)
#set(CMAKE_BUILD_TYPE BASE)


## CPM cmake dependency manager
set(CPM_DOWNLOAD_VERSION 0.34.0)
if(CPM_SOURCE_CACHE)
    set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
            https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
            ${CPM_DOWNLOAD_LOCATION}
            )
endif()
include(${CPM_DOWNLOAD_LOCATION})


set(RPX-100_SOURCES
        "${PROJECT_SOURCE_DIR}/phy/LimeRadioThread.cpp"
        "${PROJECT_SOURCE_DIR}/phy/RadioThread.cpp"
        "${PROJECT_SOURCE_DIR}/phy/LimeRadio.cpp"
        "${PROJECT_SOURCE_DIR}/phy/Radio.cpp"
        "${PROJECT_SOURCE_DIR}/phy/PhyThread.cpp"
        "${PROJECT_SOURCE_DIR}/phy/PhyFrameSync.cpp"
        "${PROJECT_SOURCE_DIR}/phy/PhyFrameGen.cpp"
        "${PROJECT_SOURCE_DIR}/phy/PhyIQDebug.cpp"
        "${PROJECT_SOURCE_DIR}/util/log.cpp"
        "${PROJECT_SOURCE_DIR}/util/write_csv_file.cpp"
        "${PROJECT_SOURCE_DIR}/util/ws_spectrogram.cpp"
        "${PROJECT_SOURCE_DIR}/util/WebSocketServer.cpp"
        "${PROJECT_SOURCE_DIR}/util/Argon2Wrapper.cpp"
        "${PROJECT_SOURCE_DIR}/util/User.cpp"
        )

set(RPX-100_INCLUDES
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/phy"
        "${PROJECT_SOURCE_DIR}/util"
        "${PROJECT_SOURCE_DIR}/external/spdlog/include"
        "${PROJECT_SOURCE_DIR}/external/argon2/include"
        "${CMAKE_BINARY_DIR}/_deps/liquid-build/include/liquid"
        "${CMAKE_BINARY_DIR}/_deps/libwebsockets-build/include/"
        "${CMAKE_BINARY_DIR}/_deps/libwebsockets-build/"
        )


## LimeSuite
set(ENABLE_EXAMPLES OFF CACHE INTERNAL "")
set(ENABLE_GUI OFF CACHE INTERNAL "")
set(ENABLE_OCTAVE OFF CACHE INTERNAL "")
set(ENABLE_SOAPY_LMS7 OFF CACHE INTERNAL "")
set(ENABLE_LIMERFE OFF CACHE INTERNAL "")
add_subdirectory("external/LimeSuite")

## nlohmann json
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory("external/json")

## cxxopts
option(CXXOPTS_ENABLE_WARNINGS ON)
add_subdirectory("external/cxxopts")

## Threads
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

## fetch libwebsockets and add library

FetchContent_Declare(libwebsockets URL https://github.com/warmcat/libwebsockets/archive/refs/tags/v4.3.2.tar.gz)
FetchContent_MakeAvailable(libwebsockets)
add_library(websockets-fixed INTERFACE)
target_include_directories(websockets-fixed
        INTERFACE
        ${libwebsockets_SOURCE_DIR}/include
        ${libwebsockets_BINARY_DIR}/include)
target_link_libraries(websockets-fixed INTERFACE websockets)

## add and compile dependencies: fftw3, liquid-dsp libaries
CPMAddPackage(
        DOWNLOAD_EXTRACT_TIMESTAMP false
        NAME fftw3
        URL "http://fftw.org/fftw-3.3.10.tar.gz"
        OPTIONS "ENABLE_THREADS ON" "ENABLE_FLOAT ON" "BUILD_TESTS OFF"
)

CPMAddPackage(
        DOWNLOAD_EXTRACT_TIMESTAMP false
        NAME liquid
        GITHUB_REPOSITORY jgaeddert/liquid-dsp
        GIT_TAG v1.5.0
        VERSION v1.5.0
        DOWNLOAD_ONLY YES
)

if (liquid_ADDED)
    set( liquid_LIBRARY "${liquid_BINARY_DIR}/lib/libliquid.so" )
    set(CONFIGURE_ARGS "--prefix=${liquid_BINARY_DIR}")

    execute_process( COMMAND "./bootstrap.sh"
            WORKING_DIRECTORY ${liquid_SOURCE_DIR}
            RESULT_VARIABLE BS_RESULT OUTPUT_VARIABLE
            BS_OUTPUT ERROR_VARIABLE BS_ERROR)

    execute_process( COMMAND "./configure" ${CONFIGURE_ARGS}
            WORKING_DIRECTORY ${liquid_SOURCE_DIR}
            RESULT_VARIABLE BS_RESULT OUTPUT_VARIABLE
            BS_OUTPUT ERROR_VARIABLE BS_ERROR)

    execute_process( COMMAND "make"
            WORKING_DIRECTORY ${liquid_SOURCE_DIR}
            RESULT_VARIABLE BS_RESULT OUTPUT_VARIABLE
            BS_OUTPUT ERROR_VARIABLE BS_ERROR)

    execute_process( COMMAND "make" "install"
            WORKING_DIRECTORY ${liquid_SOURCE_DIR}
            RESULT_VARIABLE BS_RESULT OUTPUT_VARIABLE
            BS_OUTPUT ERROR_VARIABLE BS_ERROR)
    add_library( liquid::liquid INTERFACE IMPORTED GLOBAL)
    target_include_directories( liquid::liquid INTERFACE ${liquid_BINARY_DIR}/include)
    target_link_libraries( liquid::liquid INTERFACE "${liquid_LIBRARY}")
endif()

## add argon2 sources for compiling
set(ARGON2_SOURCES
        ${PROJECT_SOURCE_DIR}/external/argon2/src/argon2.c
        ${PROJECT_SOURCE_DIR}/external/argon2/src/core.c
        ${PROJECT_SOURCE_DIR}/external/argon2/src/thread.c
        ${PROJECT_SOURCE_DIR}/external/argon2/src/ref.c
        ${PROJECT_SOURCE_DIR}/external/argon2/src/encoding.c
        ${PROJECT_SOURCE_DIR}/external/argon2/src/blake2/blake2b.c
        )


## create RPX-100 executable
add_executable(RPX-100 RPX-100.cpp ${RPX-100_SOURCES} ${ARGON2_SOURCES})
target_include_directories(RPX-100 PUBLIC ${RPX-100_INCLUDES})
target_link_libraries(RPX-100 liquid::liquid websockets LimeSuite Threads::Threads nlohmann_json::nlohmann_json cxxopts::cxxopts)


## create radio_test executable
add_executable(radio_test radio_test.cpp ${RPX-100_SOURCES} ${ARGON2_SOURCES})
target_include_directories(radio_test PUBLIC ${RPX-100_INCLUDES})
target_link_libraries(radio_test liquid::liquid websockets LimeSuite Threads::Threads nlohmann_json::nlohmann_json cxxopts::cxxopts)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/SystemConfig.json ${CMAKE_CURRENT_BINARY_DIR}/SystemConfig.json COPYONLY)

