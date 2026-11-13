# Gerador de C++ pela AST (c40, etapa 2): para cada programa do projeto que o gerador cobre, o C++ precisa
# ser o mesmo do Parser antigo (mesmos tokens e mesma sequência de #line). Também falha se a cobertura cair.
# Uso: cmake -DGHOST=<ghost.exe> -DROOT=<raiz> -DMIN_IGUAIS=<n> -P run_paridade.cmake
file(GLOB progs RELATIVE "${ROOT}" "${ROOT}/tests/ghost/*.g" "${ROOT}/libs/*.g")
list(FILTER progs EXCLUDE REGEX "(tests/ghost/error\.g|libs/time\.g)$")  # erro de propósito / nunca compilou
execute_process(COMMAND "${GHOST}" cpp --comparar ${progs} WORKING_DIRECTORY "${ROOT}"
                OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc ENCODING UTF-8)
message(STATUS "${out}")
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "C++ da AST diferente do Parser:\n${out}\n${err}")
endif()
string(REGEX MATCH "([0-9]+) igual" m "${out}")
set(iguais "${CMAKE_MATCH_1}")
if(iguais LESS MIN_IGUAIS)
  message(FATAL_ERROR "Cobertura caiu: ${iguais} programas gerados pela AST (minimo ${MIN_IGUAIS})")
endif()
message(STATUS "paridade OK: ${iguais} programas")
