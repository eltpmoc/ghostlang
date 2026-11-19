# Sem ghost-runtime.conf, o ghost acha a libghost_runtime.a ao lado do executavel e os
# cabecalhos em <raiz>/include (layout do pacote portatil: raiz/bin/ghost.exe).
# Uso: cmake -DGHOST=<ghost.exe> -DRUNTIME=<libghost_runtime.a> -DROOT=<fonte> -DWORK=<pasta> -P test_runtime_conf.cmake
get_filename_component(WORK "${WORK}" ABSOLUTE)
get_filename_component(ROOT "${ROOT}" ABSOLUTE)
set(pkg "${WORK}/pacote_runtime")
file(REMOVE_RECURSE "${pkg}")
file(MAKE_DIRECTORY "${pkg}/bin")
file(COPY "${GHOST}" DESTINATION "${pkg}/bin")
file(COPY "${RUNTIME}" DESTINATION "${pkg}/bin")
file(COPY "${ROOT}/include" DESTINATION "${pkg}")
if(EXISTS "${pkg}/bin/ghost-runtime.conf")
  message(FATAL_ERROR "o teste precisa de uma pasta sem ghost-runtime.conf")
endif()
file(WRITE "${pkg}/runtime_conf_prog.g" "function main() {\n    gate m(2, 2);\n    m(0, 1) = 3.0;\n    gate t(2, 2);\n    t = transpose(m);\n    show(t);\n    print(\"runtime_conf: OK\");\n}\n")
get_filename_component(ghost_name "${GHOST}" NAME)
execute_process(COMMAND "${pkg}/bin/${ghost_name}" run runtime_conf_prog.g WORKING_DIRECTORY "${pkg}"
                RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0 OR NOT out MATCHES "runtime_conf: OK" OR NOT out MATCHES "3[+]0g")
  message(FATAL_ERROR "ghost fora da pasta de build nao ligou o runtime (rc=${rc}):\n${out}${err}")
endif()
file(REMOVE_RECURSE "${pkg}")
message(STATUS "Runtime sem conf OK")
