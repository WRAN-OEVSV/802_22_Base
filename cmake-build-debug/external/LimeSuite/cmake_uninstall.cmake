if(NOT EXISTS "/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/install_manifest.txt")
  message(FATAL_ERROR "Cannot find install manifest: /home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/install_manifest.txt")
endif(NOT EXISTS "/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/install_manifest.txt")

file(READ "/home/bernhard/CLionProjects/802_22_Base/cmake-build-debug/external/LimeSuite/install_manifest.txt" files)
string(REGEX REPLACE "\n" ";" files "${files}")
foreach(file ${files})
  message(STATUS "Uninstalling $ENV{DESTDIR}${file}")
  if(IS_SYMLINK "$ENV{DESTDIR}${file}" OR EXISTS "$ENV{DESTDIR}${file}")
    exec_program(
      "/home/bernhard/.local/share/JetBrains/Toolbox/apps/CLion/ch-0/223.8214.51/bin/cmake/linux/bin/cmake" ARGS "-E remove \"$ENV{DESTDIR}${file}\""
      OUTPUT_VARIABLE rm_out
      RETURN_VALUE rm_retval
      )
    if(NOT "${rm_retval}" STREQUAL 0)
      message(FATAL_ERROR "Problem when removing $ENV{DESTDIR}${file}")
    endif(NOT "${rm_retval}" STREQUAL 0)
  else(IS_SYMLINK "$ENV{DESTDIR}${file}" OR EXISTS "$ENV{DESTDIR}${file}")
    message(STATUS "File $ENV{DESTDIR}${file} does not exist.")
  endif(IS_SYMLINK "$ENV{DESTDIR}${file}" OR EXISTS "$ENV{DESTDIR}${file}")
endforeach(file)
