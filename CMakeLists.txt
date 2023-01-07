########################################################################
## Create WRAN project
########################################################################
cmake_minimum_required(VERSION 3.18.1)
project("WRAN")


########################################################################
## compiler flags
########################################################################

#C++17 is a required language feature for this project
set(CMAKE_CXX_STANDARD 17)

set(WRAN_SOURCES
    "${PROJECT_SOURCE_DIR}/phy/LimeRadioThread.cpp" 
    "${PROJECT_SOURCE_DIR}/phy/RadioThread.cpp" 
    "${PROJECT_SOURCE_DIR}/util/log.cpp"
)

set(WRAN_INCLUDES
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/phy"
    "${PROJECT_SOURCE_DIR}/util"
    "${PROJECT_SOURCE_DIR}/external/spdlog/include"
)

set(WRAN_LIBRARIES
    "-L/usr/lib"
    "-lLimeSuite"
)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)


# create radio_test executable; used for testing sdr and phy topics at the moment
add_executable(radio_test radio_test.cpp ${WRAN_SOURCES})
target_include_directories(radio_test PUBLIC ${WRAN_INCLUDES})
target_link_libraries(radio_test ${WRAN_LIBRARIES} Threads::Threads )

#message(${WRAN_INCLUDES})
#message(${WRAN_SOURCES})

