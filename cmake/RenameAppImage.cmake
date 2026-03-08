# cmake/RenameAppImage.cmake  (script mode, called via cmake -P)
#
# Finds the AppImage produced by linuxdeploy (NeurolingsCE-*.AppImage) in
# SEARCH_DIR and moves it to OUTPUT.
#
# Required variables (pass via -D):
#   SEARCH_DIR  - directory to search for NeurolingsCE-*.AppImage
#   OUTPUT      - destination path for the final AppImage

foreach(_var SEARCH_DIR OUTPUT)
  if(NOT DEFINED ${_var})
    message(FATAL_ERROR "cmake/RenameAppImage.cmake: ${_var} must be set via -D.")
  endif()
endforeach()

file(GLOB _candidates "${SEARCH_DIR}/NeurolingsCE-*.AppImage")

if(NOT _candidates)
  message(FATAL_ERROR "No NeurolingsCE-*.AppImage found in ${SEARCH_DIR}. linuxdeploy may have failed.")
endif()

list(GET _candidates 0 _src)
file(RENAME "${_src}" "${OUTPUT}")
message(STATUS "AppImage created: ${OUTPUT}")
