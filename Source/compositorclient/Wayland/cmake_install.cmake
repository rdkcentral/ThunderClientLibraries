# Install script for directory: /home/h.sainul/rpi-br/rpi-main/buildroot-18Jun/output/build/wpeframework-clientlibraries-9b5d6c6915913936d7edfba212fc51257fd3cc93/Source/compositorclient/Wayland

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
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
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xlibsx" OR NOT CMAKE_INSTALL_COMPONENT)
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libcompositorclient.so.1.0.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libcompositorclient.so.1"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libcompositorclient.so"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/home/h.sainul/rpi-br/rpi-main/buildroot-18Jun/output/build/wpeframework-clientlibraries-9b5d6c6915913936d7edfba212fc51257fd3cc93/Source/compositorclient/Wayland/libcompositorclient.so.1.0.0"
    "/home/h.sainul/rpi-br/rpi-main/buildroot-18Jun/output/build/wpeframework-clientlibraries-9b5d6c6915913936d7edfba212fc51257fd3cc93/Source/compositorclient/Wayland/libcompositorclient.so.1"
    "/home/h.sainul/rpi-br/rpi-main/buildroot-18Jun/output/build/wpeframework-clientlibraries-9b5d6c6915913936d7edfba212fc51257fd3cc93/Source/compositorclient/Wayland/libcompositorclient.so"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libcompositorclient.so.1.0.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libcompositorclient.so.1"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libcompositorclient.so"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/home/h.sainul/rpi-br/rpi-main/buildroot-18Jun/output/host/bin/arm-buildroot-linux-gnueabihf-strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/home/h.sainul/rpi-br/rpi-main/buildroot-18Jun/output/build/wpeframework-clientlibraries-9b5d6c6915913936d7edfba212fc51257fd3cc93/Source/compositorclient/Wayland/compositorclient.pc")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/compositorclient/compositorclientTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/compositorclient/compositorclientTargets.cmake"
         "/home/h.sainul/rpi-br/rpi-main/buildroot-18Jun/output/build/wpeframework-clientlibraries-9b5d6c6915913936d7edfba212fc51257fd3cc93/Source/compositorclient/Wayland/CMakeFiles/Export/lib/cmake/compositorclient/compositorclientTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/compositorclient/compositorclientTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/compositorclient/compositorclientTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/compositorclient" TYPE FILE FILES "/home/h.sainul/rpi-br/rpi-main/buildroot-18Jun/output/build/wpeframework-clientlibraries-9b5d6c6915913936d7edfba212fc51257fd3cc93/Source/compositorclient/Wayland/CMakeFiles/Export/lib/cmake/compositorclient/compositorclientTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/compositorclient" TYPE FILE FILES "/home/h.sainul/rpi-br/rpi-main/buildroot-18Jun/output/build/wpeframework-clientlibraries-9b5d6c6915913936d7edfba212fc51257fd3cc93/Source/compositorclient/Wayland/CMakeFiles/Export/lib/cmake/compositorclient/compositorclientTargets-release.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/compositorclient" TYPE FILE FILES
    "/home/h.sainul/rpi-br/rpi-main/buildroot-18Jun/output/build/wpeframework-clientlibraries-9b5d6c6915913936d7edfba212fc51257fd3cc93/Source/compositorclient/Wayland/compositorclientConfigVersion.cmake"
    "/home/h.sainul/rpi-br/rpi-main/buildroot-18Jun/output/build/wpeframework-clientlibraries-9b5d6c6915913936d7edfba212fc51257fd3cc93/Source/compositorclient/Wayland/compositorclientConfig.cmake"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/compositorclient" TYPE FILE FILES "/home/h.sainul/rpi-br/rpi-main/buildroot-18Jun/output/build/wpeframework-clientlibraries-9b5d6c6915913936d7edfba212fc51257fd3cc93/Source/compositorclient/Wayland/Implementation.h")
endif()

