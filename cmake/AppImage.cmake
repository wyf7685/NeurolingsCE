# cmake/AppImage.cmake  (include mode)
#
# Defines the 'appimage' custom target for Linux AppImage packaging.
# Mirrors the logic in the legacy Makefile:
#   publish/Linux/$(CONFIG)/NeurolingsCE.AppImage target.
#
# Required variables (set before include()-ing this file):
#   APPIMAGE_TARGET      - CMake target to package (e.g. NeurolingsCE)
#   APPIMAGE_SOURCE_DIR  - Absolute path to the project root
#   APPIMAGE_BINARY_DIR  - Absolute path to the build directory

foreach(_var APPIMAGE_TARGET APPIMAGE_SOURCE_DIR APPIMAGE_BINARY_DIR)
  if(NOT DEFINED ${_var})
    message(FATAL_ERROR "cmake/AppImage.cmake: ${_var} must be set before include()-ing this file.")
  endif()
endforeach()

# -------------------------------------------------------------------------
# Architecture detection
# -------------------------------------------------------------------------
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
  set(_appimage_arch "x86_64")
elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
  set(_appimage_arch "aarch64")
else()
  message(WARNING
    "cmake/AppImage.cmake: unsupported host architecture "
    "'${CMAKE_HOST_SYSTEM_PROCESSOR}'. The 'appimage' target will not be available."
  )
  return()
endif()

# -------------------------------------------------------------------------
# Paths
# -------------------------------------------------------------------------
set(_tool_dir    "${APPIMAGE_BINARY_DIR}/appimage-tools")
set(_appdir      "${APPIMAGE_BINARY_DIR}/AppDir")
set(_output      "${APPIMAGE_BINARY_DIR}/NeurolingsCE.AppImage")

set(_linuxdeploy        "${_tool_dir}/linuxdeploy-${_appimage_arch}.AppImage")
set(_plugin_qt          "${_tool_dir}/linuxdeploy-plugin-qt-${_appimage_arch}.AppImage")
set(_plugin_appimage    "${_tool_dir}/linuxdeploy-plugin-appimage-${_appimage_arch}.AppImage")

set(_desktop_file  "${APPIMAGE_SOURCE_DIR}/src/packaging/io.github.qingchenyouforcc.NeurolingsCE.desktop")
set(_icon_file     "${APPIMAGE_SOURCE_DIR}/src/packaging/io.github.qingchenyouforcc.NeurolingsCE.png")

# Derive Qt bin directory from Qt6_DIR
# Qt6_DIR is e.g. /opt/Qt/6.8.2/gcc_64/lib/cmake/Qt6  →  prefix = .../gcc_64
get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
set(_qt_bin_dir "${_qt_prefix}/bin")

# -------------------------------------------------------------------------
# 'appimage' target
# -------------------------------------------------------------------------
add_custom_target(appimage
  # 1. Download linuxdeploy + plugins (skips already-downloaded files)
  COMMAND ${CMAKE_COMMAND}
    -DTOOL_DIR=${_tool_dir}
    -DARCH=${_appimage_arch}
    -P ${APPIMAGE_SOURCE_DIR}/cmake/DownloadLinuxdeploy.cmake

  # 2. Remove stale AppDir from a previous run
  COMMAND ${CMAKE_COMMAND} -E rm -rf "${_appdir}"

  # 3. Run linuxdeploy to assemble and bundle the AppImage.
  #    APPIMAGE_EXTRACT_AND_RUN=1 bypasses FUSE (required in WSL and some CI
  #    environments where FUSE is unavailable). All three AppImage binaries
  #    (linuxdeploy, plugin-qt, plugin-appimage) honour this variable.
  #    NO_STRIP=1 matches the Makefile behaviour (keeps debug info for CI).
  #    QMAKE points linuxdeploy-plugin-qt at the correct Qt installation.
  #    LINUXDEPLOY_PLUGIN_QT and LINUXDEPLOY_PLUGIN_APPIMAGE tell linuxdeploy
  #    where the sidecar plugins live when they are not on PATH.
  COMMAND ${CMAKE_COMMAND} -E env
    APPIMAGE_EXTRACT_AND_RUN=1
    NO_STRIP=1
    QMAKE=${_qt_bin_dir}/qmake6
    LINUXDEPLOY_PLUGIN_QT=${_plugin_qt}
    LINUXDEPLOY_PLUGIN_APPIMAGE=${_plugin_appimage}
    "${_linuxdeploy}"
      --appdir "${_appdir}"
      --executable "$<TARGET_FILE:${APPIMAGE_TARGET}>"
      --desktop-file "${_desktop_file}"
      --icon-file "${_icon_file}"
      --output appimage
      --plugin qt

  # 4. linuxdeploy names the file NeurolingsCE-<arch>.AppImage; rename to a
  #    stable path so downstream steps don't need to glob.
  COMMAND ${CMAKE_COMMAND}
    -DSEARCH_DIR=${APPIMAGE_BINARY_DIR}
    -DOUTPUT=${_output}
    -P ${APPIMAGE_SOURCE_DIR}/cmake/RenameAppImage.cmake

  WORKING_DIRECTORY "${APPIMAGE_BINARY_DIR}"
  DEPENDS ${APPIMAGE_TARGET}
  VERBATIM
  USES_TERMINAL
  COMMENT "Building AppImage for ${APPIMAGE_TARGET} (arch: ${_appimage_arch})"
)
