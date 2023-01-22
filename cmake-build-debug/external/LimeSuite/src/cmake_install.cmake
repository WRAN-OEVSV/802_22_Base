# Install script for directory: /home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/lime" TYPE FILE FILES
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/lime/LimeSuite.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/VersionInfo.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/Logger.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/SystemResources.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/LimeSuiteConfig.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/ADF4002/ADF4002.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/lms7002m_mcu/MCU_BD.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/lms7002m_mcu/MCU_File.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/ConnectionRegistry/IConnection.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/ConnectionRegistry/ConnectionHandle.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/ConnectionRegistry/ConnectionRegistry.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/lms7002m/LMS7002M.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/lms7002m/LMS7002M_RegistersMap.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/lime/LMS7002M_parameters.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/protocols/Streamer.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/protocols/ADCUnits.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/protocols/LMS64CCommands.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/protocols/LMS64CProtocol.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/protocols/LMSBoards.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/protocols/dataTypes.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/protocols/fifo.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/Si5351C/Si5351C.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/FPGA_common/FPGA_common.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/API/lms7_device.h"
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/src/limeRFE/limeRFE.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libLimeSuite.so.22.09.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libLimeSuite.so.22.09-1"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/src/libLimeSuite.so.22.09.0"
    "/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/src/libLimeSuite.so.22.09-1"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libLimeSuite.so.22.09.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libLimeSuite.so.22.09-1"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libLimeSuite.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libLimeSuite.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libLimeSuite.so"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/src/libLimeSuite.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libLimeSuite.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libLimeSuite.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libLimeSuite.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/src/LimeSuite.pc")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/src/GFIR/cmake_install.cmake")
  include("/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/src/utilities/cmake_install.cmake")
  include("/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/src/examples/cmake_install.cmake")
  include("/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/src/limeRFE/cmake_install.cmake")

endif()

