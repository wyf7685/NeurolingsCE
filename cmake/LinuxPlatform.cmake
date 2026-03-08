# cmake/LinuxPlatform.cmake
#
# Linux-specific platform setup for NeurolingsCE:
#   - Generates kwin_script.c  (KDE KWin script embedded as a C byte array)
#   - Generates gnome_script.c (GNOME Shell extension ZIP embedded as a C byte array)
#   - Adds Linux platform sources to the target
#   - Links Qt6::DBus and X11
#   - Suppresses the Ppmd8 ODR collision between libarchive and unarr
#
# Required variables (set before include()-ing this file):
#   LINUX_TARGET          - CMake target name to configure (e.g. NeurolingsCE)
#   LINUX_SOURCE_DIR      - Absolute path to the project root (CMAKE_CURRENT_SOURCE_DIR)
#   LINUX_BINARY_DIR      - Absolute path to the build directory (CMAKE_CURRENT_BINARY_DIR)
#   BIN2CPP_CMAKE_SCRIPT  - Absolute path to cmake/bin2cpp.cmake

foreach(_var LINUX_TARGET LINUX_SOURCE_DIR LINUX_BINARY_DIR BIN2CPP_CMAKE_SCRIPT)
  if(NOT DEFINED ${_var})
    message(FATAL_ERROR "cmake/LinuxPlatform.cmake: ${_var} must be set before include()-ing this file.")
  endif()
endforeach()

set(_plat_dir "${LINUX_SOURCE_DIR}/src/platform/Platform/Linux")

# ---------------------------------------------------------------------------
# 1. kwin_script.c — embed kwin_script.js as a C byte array
#    Included directly by KDEWindowObserverBackend.cc as:
#      #include "kwin_script.c"
#    Produces: kwin_script[]  kwin_script_len
# ---------------------------------------------------------------------------
add_custom_command(
  OUTPUT "${_plat_dir}/kwin_script.c"
  COMMAND ${CMAKE_COMMAND}
    -DINPUT=${_plat_dir}/kwin_script.js
    -DOUTPUT=${_plat_dir}/kwin_script.c
    -P ${BIN2CPP_CMAKE_SCRIPT}
  DEPENDS
    "${_plat_dir}/kwin_script.js"
    "${BIN2CPP_CMAKE_SCRIPT}"
  VERBATIM
)

# ---------------------------------------------------------------------------
# 2. gnome_script.c — zip the GNOME Shell extension, then embed as a C byte array
#    Included directly by GNOMEWindowObserverBackend.cc as:
#      #include "gnome_script.c"
#    Produces: gnome_script[]  gnome_script_len
# ---------------------------------------------------------------------------
find_program(ZIP_EXECUTABLE zip REQUIRED)

set(_gnome_zip "${LINUX_BINARY_DIR}/gnome_script.zip")

add_custom_command(
  OUTPUT "${_gnome_zip}"
  COMMAND ${ZIP_EXECUTABLE} -jr "${_gnome_zip}"
    "${_plat_dir}/gnome_script/metadata.json"
    "${_plat_dir}/gnome_script/extension.js"
  DEPENDS
    "${_plat_dir}/gnome_script/metadata.json"
    "${_plat_dir}/gnome_script/extension.js"
  VERBATIM
)

add_custom_command(
  OUTPUT "${_plat_dir}/gnome_script.c"
  COMMAND ${CMAKE_COMMAND}
    -DINPUT=${_gnome_zip}
    -DOUTPUT=${_plat_dir}/gnome_script.c
    -P ${BIN2CPP_CMAKE_SCRIPT}
  DEPENDS
    "${_gnome_zip}"
    "${BIN2CPP_CMAKE_SCRIPT}"
  VERBATIM
)

# Convenience target so callers can depend on both generated files at once.
add_custom_target(linux_platform_scripts
  DEPENDS
    "${_plat_dir}/kwin_script.c"
    "${_plat_dir}/gnome_script.c"
)
add_dependencies(${LINUX_TARGET} linux_platform_scripts)

# Tell CMake that the two .cc files that #include generated .c files must wait
# for the generators to finish before they are compiled.
set_source_files_properties(
  src/platform/Platform/Linux/KDEWindowObserverBackend.cc
  PROPERTIES OBJECT_DEPENDS "${_plat_dir}/kwin_script.c"
)
set_source_files_properties(
  src/platform/Platform/Linux/GNOMEWindowObserverBackend.cc
  PROPERTIES OBJECT_DEPENDS "${_plat_dir}/gnome_script.c"
)

# ---------------------------------------------------------------------------
# 3. Platform sources
# ---------------------------------------------------------------------------
target_sources(${LINUX_TARGET} PRIVATE
  src/platform/Platform/Linux/Platform.cc
  src/platform/Platform/Linux/ActiveWindowObserver.cc
  src/platform/Platform/Linux/PrivateActiveWindowObserver.cc
  src/platform/Platform/Linux/PrivateActiveWindowObserver.hpp
  src/platform/Platform/Linux/KWin.cc
  src/platform/Platform/Linux/KDEWindowObserverBackend.cc
  src/platform/Platform/Linux/GNOME.cc
  src/platform/Platform/Linux/GNOMEWindowObserverBackend.cc
  src/platform/Platform/Linux/DBus.cc
  src/platform/Platform/Linux/ExtensionFile.cc
)

# ---------------------------------------------------------------------------
# 4. Qt DBus (GNOME/KDE window tracking)
# ---------------------------------------------------------------------------
find_package(Qt6 COMPONENTS DBus REQUIRED)
target_link_libraries(${LINUX_TARGET} PRIVATE Qt6::DBus)

# ---------------------------------------------------------------------------
# 5. X11 (Platform.cc uses XFlush, XOpenDisplay, etc.)
# ---------------------------------------------------------------------------
find_package(X11 REQUIRED)
target_link_libraries(${LINUX_TARGET} PRIVATE X11::X11)

# ---------------------------------------------------------------------------
# 6. Ppmd8 ODR workaround
#    libarchive (archive_ppmd8.c) and unarr (lzmasdk/Ppmd8.c) both bundle the
#    7-zip Ppmd8 implementation under identical symbol names. Neither provides a
#    CMake option to disable its copy. The two implementations are functionally
#    identical; suppress the linker error and let the first one win.
# ---------------------------------------------------------------------------
target_link_options(${LINUX_TARGET} PRIVATE -Wl,--allow-multiple-definition)
