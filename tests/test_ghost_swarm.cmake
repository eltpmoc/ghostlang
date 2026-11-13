# Invoca test_swarm.py para coordenar o no de borda e o swarm federado
execute_process(
    COMMAND python "${CMAKE_CURRENT_LIST_DIR}/test_swarm.py" "${NODE_BIN}" "${SWARM_BIN}" "${WORK}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
)

if(NOT rc EQUAL 0)
    message(FATAL_ERROR "Falha no teste do Swarm Federado:\n${out}\n${err}")
endif()

message(STATUS "${out}")
