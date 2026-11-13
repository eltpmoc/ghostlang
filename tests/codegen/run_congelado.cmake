# Oráculo congelado do gerador de C++ (c40, etapa 3): o C++ da AST, normalizado (tokens sem espaços e
# sequência de #line, a mesma régua de ghost_cpp_equivalent), precisa ser igual ao gravado em
# tests/codegen/esperado/ (raiz) ou tests/codegen/esperado_ia/ (ia_propria). Esses arquivos foram
# gerados pelo Parser antigo em 08/11/2026 e substituem o Parser como referência.
# Uma mudança intencional no C++ gerado exige regenerar os arquivos (ghost cpp --congelar) e explicar
# em docs/NUCLEO.md, seção "Geração de C++ pela AST".
# Uso: cmake -DGHOST=<ghost.exe> -DROOT=<raiz> -DMIN_IGUAIS=<n> [-DIA=1] -P run_congelado.cmake
if(IA)
  set(work "${ROOT}/ia_propria")
  set(esperado "${ROOT}/tests/codegen/esperado_ia")
  set(progs v5/src/main.g v4/src/main.g src/main.g v2/src/main.g v3/src/main.g src/modelo.g v3/src/conhecimento.g)
else()
  set(work "${ROOT}")
  set(esperado "${ROOT}/tests/codegen/esperado")
  file(GLOB progs RELATIVE "${ROOT}" "${ROOT}/tests/ghost/*.g" "${ROOT}/libs/*.g")
  list(FILTER progs EXCLUDE REGEX "tests/ghost/error\\.g$")  # erro de propósito
  list(SORT progs)
endif()
execute_process(COMMAND "${GHOST}" cpp --comparar-com "${esperado}" ${progs} WORKING_DIRECTORY "${work}"
                OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc ENCODING UTF-8)
message(STATUS "${out}")
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "C++ da AST diferente do congelado:\n${out}\n${err}")
endif()
string(REGEX MATCH "([0-9]+) igual" m "${out}")
set(iguais "${CMAKE_MATCH_1}")
if(iguais LESS MIN_IGUAIS)
  message(FATAL_ERROR "Cobertura caiu: ${iguais} programas iguais ao congelado (minimo ${MIN_IGUAIS})")
endif()
message(STATUS "congelado OK: ${iguais} programas")
