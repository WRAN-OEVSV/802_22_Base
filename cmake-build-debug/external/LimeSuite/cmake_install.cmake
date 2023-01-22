# Install script for directory: /home/bernhard/CLionProjects/802_22_Base/external/LimeSuite

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/LimeSuite" TYPE FILE FILES
    "/home/bernhard/CLionProjects/802_22_Base/external/LimeSuite/cmake/Modules/LimeSuiteConfig.cmake"
    "/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/LimeSuiteConfigVersion.cmake"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/src/cmake_install.cmake")
  include("/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/mcu_program/cmake_install.cmake")
  include("/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/LimeUtil/cmake_install.cmake")
  include("/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/QuickTest/cmake_install.cmake")
  include("/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/SoapyLMS7/cmake_install.cmake")
  include("/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/Desktop/cmake_install.cmake")
  include("/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/octave/cmake_install.cmake")

endif()

