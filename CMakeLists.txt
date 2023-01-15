########################################################################
## Create WRAN project
########################################################################
cmake_minimum_required(VERSION 3.18.1)
project("WRAN" DESCRIPTION "WRAN 802.22 HAMNET" LANGUAGES CXX)


########################################################################
## compiler flags
########################################################################

#C++17 is a required language feature for this project
set(CMAKE_CXX_STANDARD           17 )
set(CMAKE_CXX_STANDARD_REQUIRED  ON )
set(CMAKE_CXX_EXTENSIONS         OFF)

#optimizing must be looked at in more details - especially what is helping on complex math and fft
add_definitions(-march=native)
add_definitions(-mfpmath=sse)


set(WRAN_SOURCES
    "${PROJECT_SOURCE_DIR}/phy/LimeRadioThread.cpp" 
    "${PROJECT_SOURCE_DIR}/phy/RadioThread.cpp" 
    "${PROJECT_SOURCE_DIR}/phy/PhyThread.cpp" 
    "${PROJECT_SOURCE_DIR}/phy/PhyFrameSync.cpp" 
    "${PROJECT_SOURCE_DIR}/util/log.cpp"
    "${PROJECT_SOURCE_DIR}/util/write_csv_file.cpp"
    "${PROJECT_SOURCE_DIR}/util/ws_spectrogram.cpp"
    "${PROJECT_SOURCE_DIR}/util/WebSocketServer.cpp"
)

set(WRAN_INCLUDES
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/phy"
    "${PROJECT_SOURCE_DIR}/util"
    "${PROJECT_SOURCE_DIR}/external/spdlog/include"
    "${PROJECT_SOURCE_DIR}/external/liquid-dsp/include"
)

#set(WRAN_LIBRARIES
#    "liquid"
#)


# LimeSuite
set(ENABLE_EXAMPLES OFF CACHE INTERNAL "")
set(ENABLE_GUI OFF CACHE INTERNAL "")
set(ENABLE_OCTAVE OFF CACHE INTERNAL "")
set(ENABLE_SOAPY_LMS7 OFF CACHE INTERNAL "")
set(ENABLE_LIMERFE OFF CACHE INTERNAL "")
add_subdirectory("external/LimeSuite")

# nlohmann json
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory("external/json")

# cxxopts
option(CXXOPTS_ENABLE_WARNINGS ON)
add_subdirectory("external/cxxopts")

# Threads
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)


# create radio_test executable; used for testing sdr and phy topics at the moment
add_executable(radio_test radio_test.cpp ${WRAN_SOURCES})
target_include_directories(radio_test PUBLIC ${WRAN_INCLUDES})
target_link_libraries(radio_test liquid websockets LimeSuite Threads::Threads nlohmann_json::nlohmann_json cxxopts::cxxopts)
#target_link_libraries(radio_test LimeSuite Threads::Threads nlohmann_json::nlohmann_json cxxopts::cxxopts)

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/SystemConfig.json ${CMAKE_CURRENT_BINARY_DIR}/SystemConfig.json COPYONLY)
