# Usage:
# cmake -DINPUT=file.xml -DOUTPUT=file.cc -P bin2cpp.cmake

if(NOT DEFINED INPUT)
    message(FATAL_ERROR "INPUT not defined")
endif()

if(NOT DEFINED OUTPUT)
    message(FATAL_ERROR "OUTPUT not defined")
endif()

get_filename_component(var "${OUTPUT}" NAME_WE)

file(READ "${INPUT}" HEX_CONTENT HEX)

string(REGEX REPLACE "(..)" "\\\\x\\1" HEX_CONTENT "${HEX_CONTENT}")

file(WRITE "${OUTPUT}" "#include <cstddef>\n")
file(APPEND "${OUTPUT}" "static const char ${var}[] =\n")

string(LENGTH "${HEX_CONTENT}" len)
math(EXPR lines "${len} / 32")

foreach(i RANGE 0 ${lines})
    math(EXPR start "${i} * 32")
    string(SUBSTRING "${HEX_CONTENT}" ${start} 32 chunk)
    if(chunk)
        file(APPEND "${OUTPUT}" "    \"${chunk}\"\n")
    endif()
endforeach()

file(APPEND "${OUTPUT}" ";\n")

file(SIZE "${INPUT}" file_size)
file(APPEND "${OUTPUT}" "static const size_t ${var}_len = ${file_size};\n")
