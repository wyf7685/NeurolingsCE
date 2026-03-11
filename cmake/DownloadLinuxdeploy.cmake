# cmake/DownloadLinuxdeploy.cmake  (script mode, called via cmake -P)
#
# Downloads linuxdeploy + its Qt and AppImage plugins for the target architecture.
# Already-downloaded files are skipped.
#
# Required variables (pass via -D):
#   TOOL_DIR  - directory where the AppImage tools are stored
#   ARCH      - target architecture string: x86_64 or aarch64

foreach(_var TOOL_DIR ARCH)
  if(NOT DEFINED ${_var})
    message(FATAL_ERROR "cmake/DownloadLinuxdeploy.cmake: ${_var} must be set via -D.")
  endif()
endforeach()

set(_base "https://github.com/linuxdeploy")
set(_tools
  "linuxdeploy/releases/latest/download/linuxdeploy-${ARCH}.AppImage"
  "linuxdeploy-plugin-qt/releases/latest/download/linuxdeploy-plugin-qt-${ARCH}.AppImage"
  "linuxdeploy-plugin-appimage/releases/latest/download/linuxdeploy-plugin-appimage-${ARCH}.AppImage"
)

file(MAKE_DIRECTORY "${TOOL_DIR}")

foreach(_rel ${_tools})
  get_filename_component(_name "${_rel}" NAME)
  set(_dest "${TOOL_DIR}/${_name}")

  if(EXISTS "${_dest}")
    message(STATUS "linuxdeploy: already present: ${_dest}")
  else()
    message(STATUS "linuxdeploy: downloading: ${_dest}")
    set(_url "${_base}/${_rel}")
    message(STATUS "Downloading ${_url}")
    execute_process(
      COMMAND curl -L -f --retry 3 --retry-delay 2 
              -o "${_dest}" "${_url}" 
      RESULT_VARIABLE _code)
    if(NOT _code EQUAL 0)
      file(REMOVE "${_dest}")
      message(FATAL_ERROR "Download failed (${_code}): ${_url}")
    endif()
    file(CHMOD "${_dest}"
      PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
    )
    message(STATUS "Downloaded: ${_dest}")
  endif()
endforeach()
