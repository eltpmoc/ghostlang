# Testa ghost_api e ghost_ask
file(WRITE "${WORK}/cerebro.weights" "0.5 0.5 0.5 0.5\n")

execute_process(COMMAND "${API_BIN}" "1.0" "1.0" WORKING_DIRECTORY "${WORK}"
                OUTPUT_VARIABLE api_out RESULT_VARIABLE api_rc)
if(NOT api_rc EQUAL 0 OR NOT api_out MATCHES "STABLE_OR_IMPROVING")
    file(REMOVE "${WORK}/cerebro.weights")
    message(FATAL_ERROR "ghost_api falhou: rc=${api_rc}, out=${api_out}")
endif()

execute_process(COMMAND "${ASK_BIN}" "1.0" "1.0" WORKING_DIRECTORY "${WORK}"
                OUTPUT_VARIABLE ask_out RESULT_VARIABLE ask_rc)
file(REMOVE "${WORK}/cerebro.weights")
if(NOT ask_rc EQUAL 0 OR NOT ask_out MATCHES "Tendência Preditiva|Modulo de Inferencia")
    message(FATAL_ERROR "ghost_ask falhou: rc=${ask_rc}, out=${ask_out}")
endif()

message(STATUS "ghost_api e ghost_ask: OK")
