# Compara a árvore de "ghost ast" com a esperada (c40).
# Uso: cmake -DGHOST=<ghost.exe> -DSOURCE=<arquivo.g> -DEXPECT=<arvore esperada> -P run_ast_test.cmake
get_filename_component(srcdir "${SOURCE}" DIRECTORY)
get_filename_component(name "${SOURCE}" NAME)
execute_process(COMMAND "${GHOST}" ast "${name}" WORKING_DIRECTORY "${srcdir}"
                OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc ENCODING UTF-8)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "ghost ast falhou (${rc}):\n${err}")
endif()
file(READ "${EXPECT}" expected ENCODING UTF-8)
string(REPLACE "\r" "" out "${out}")
string(REPLACE "\r" "" expected "${expected}")
if(NOT out STREQUAL expected)
  message(FATAL_ERROR "Arvore diferente da esperada.\n--- obtida ---\n${out}")
endif()
message(STATUS "ast ${name}: OK")
