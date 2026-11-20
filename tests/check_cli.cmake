execute_process(
    COMMAND "${OPTILANG}" "${INPUT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE errors
    TIMEOUT 10
)

if(NOT "${result}" STREQUAL "${EXPECTED_EXIT}")
    message(FATAL_ERROR
        "Expected exit ${EXPECTED_EXIT}, got ${result}\n${output}\n${errors}")
endif()

if(NOT EXPECTED_EXIT EQUAL 1)
    if(NOT errors STREQUAL "")
        message(FATAL_ERROR "Unexpected stderr:\n${errors}")
    endif()
    set(actual "${output}")
else()
    if(output MATCHES "Linear IR generated|Status:")
        message(FATAL_ERROR "Rejected input reached compilation or solving:\n${output}")
    endif()
    set(actual "${errors}")
endif()

foreach(fragment IN LISTS EXPECTED_TEXT)
    string(FIND "${actual}" "${fragment}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "Missing '${fragment}' in:\n${actual}")
    endif()
endforeach()

if(NOT EXPECTED_STATUS STREQUAL "")
    if(NOT output MATCHES "(^|\n)Status: ${EXPECTED_STATUS}(\r?\n|$)")
        message(FATAL_ERROR "Expected status ${EXPECTED_STATUS}:\n${output}")
    endif()
elseif(output MATCHES "(^|\n)Status:")
    message(FATAL_ERROR "Unexpected solver status:\n${output}")
endif()

set(number "[+-]?[0-9]+(\\.[0-9]*)?([eE][+-]?[0-9]+)?")
foreach(expectation IN LISTS EXPECTED_VALUES)
    string(REPLACE "," ";" fields "${expectation}")
    list(GET fields 0 name)
    list(GET fields 1 lower)
    list(GET fields 2 upper)
    if(NOT output MATCHES "(^|\n)${name} = (${number})(\r?\n|$)")
        message(FATAL_ERROR "Missing numeric value for ${name}:\n${output}")
    endif()
    set(value "${CMAKE_MATCH_2}")
    if(value LESS lower OR value GREATER upper)
        message(FATAL_ERROR "${name} = ${value} is outside [${lower}, ${upper}]")
    endif()
endforeach()

if(NOT OMIT_VALUE STREQUAL "" AND output MATCHES "(^|\n)${OMIT_VALUE} = ")
    message(FATAL_ERROR "${OMIT_VALUE} was not requested by print:\n${output}")
endif()
