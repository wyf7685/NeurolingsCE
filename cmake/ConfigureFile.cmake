# ConfigureFile.cmake - Substitute @VAR@ variables in a template file
# Usage: cmake -DINPUT_FILE=<in> -DOUTPUT_FILE=<out> -DVERSION=<ver> -P ConfigureFile.cmake

file(READ "${INPUT_FILE}" FILE_CONTENT)

# Get version components from VERSION.txt if not provided
if(NOT DEFINED VERSION OR NOT DEFINED VERSION_MAJOR)
  file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../VERSION.txt" VERSION_LINES)
  foreach(LINE ${VERSION_LINES})
    if(LINE MATCHES "^VERSION=(.*)$")
      set(VERSION "${CMAKE_MATCH_1}")
    elseif(LINE MATCHES "^VERSION_MAJOR=(.*)$")
      set(VERSION_MAJOR "${CMAKE_MATCH_1}")
    elseif(LINE MATCHES "^VERSION_MINOR=(.*)$")
      set(VERSION_MINOR "${CMAKE_MATCH_1}")
    elseif(LINE MATCHES "^VERSION_PATCH=(.*)$")
      set(VERSION_PATCH "${CMAKE_MATCH_1}")
    elseif(LINE MATCHES "^RELEASE_DATE=(.*)$")
      set(RELEASE_DATE "${CMAKE_MATCH_1}")
    endif()
  endforeach()
  string(STRIP "${VERSION}" VERSION)
  string(STRIP "${VERSION_MAJOR}" VERSION_MAJOR)
  string(STRIP "${VERSION_MINOR}" VERSION_MINOR)
  string(STRIP "${VERSION_PATCH}" VERSION_PATCH)
  string(STRIP "${RELEASE_DATE}" RELEASE_DATE)
endif()

# Substitute @VAR@ placeholders
string(REPLACE "@VERSION@" "${VERSION}" FILE_CONTENT "${FILE_CONTENT}")
string(REPLACE "@VERSION_MAJOR@" "${VERSION_MAJOR}" FILE_CONTENT "${FILE_CONTENT}")
string(REPLACE "@VERSION_MINOR@" "${VERSION_MINOR}" FILE_CONTENT "${FILE_CONTENT}")
string(REPLACE "@VERSION_PATCH@" "${VERSION_PATCH}" FILE_CONTENT "${FILE_CONTENT}")
string(REPLACE "@RELEASE_DATE@" "${RELEASE_DATE}" FILE_CONTENT "${FILE_CONTENT}")

file(WRITE "${OUTPUT_FILE}" "${FILE_CONTENT}")
