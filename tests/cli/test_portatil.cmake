# Pacote portátil (dist/GhostLang-Portable-x64): numa "máquina limpa", com o PATH reduzido
# ao Windows (sem MSYS2/MinGW), o ghost do pacote compila e roda um programa com o GCC
# embutido em bin/compiler e o runtime ao lado do ghost.exe; o REPL do pacote também roda.
# Uso: cmake -DPKG=<dist/GhostLang-Portable-x64> -DWORK=<pasta> -P test_portatil.cmake
if(NOT EXISTS "${PKG}/bin/ghost.exe")
  message(FATAL_ERROR "Pacote nao encontrado em ${PKG}: gere com scripts/package_release.ps1")
endif()
if(NOT EXISTS "${PKG}/bin/compiler/bin/g++.exe")
  message(FATAL_ERROR "O pacote nao tem o compilador embutido (bin/compiler/bin/g++.exe)")
endif()
set(dir "${WORK}/portatil_teste")
file(REMOVE_RECURSE "${dir}")
file(MAKE_DIRECTORY "${dir}")
file(WRITE "${dir}/portatil_prog.g" "import \"libs/math.g\"\nfunction main() {\n    gate m(2, 2);\n    m(1, 0) = 7.0;\n    gate t(2, 2);\n    t = transpose(m);\n    show(t);\n    print(\"portatil: OK \", blas_backend());\n}\n")
set(limpo "PATH=C:/Windows/System32;C:/Windows")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "${limpo}" "${PKG}/bin/ghost.exe" run portatil_prog.g
                WORKING_DIRECTORY "${dir}" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 300)
if(NOT rc EQUAL 0 OR NOT out MATCHES "portatil: OK" OR NOT out MATCHES "7[+]0g")
  message(FATAL_ERROR "ghost do pacote falhou sem o MinGW no PATH (rc=${rc}):\n${out}\n${err}")
endif()
file(WRITE "${dir}/repl.ghost" "print(\"repl portatil: \", 6 * 7);\nexit\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "${limpo}" "${PKG}/bin/ghost.exe"
                INPUT_FILE "${dir}/repl.ghost" WORKING_DIRECTORY "${dir}" RESULT_VARIABLE rc2 OUTPUT_VARIABLE out2 ERROR_VARIABLE err2 TIMEOUT 60)
if(NOT out2 MATCHES "repl portatil: 42")
  message(FATAL_ERROR "REPL do pacote falhou (rc=${rc2}):\n${out2}\n${err2}")
endif()
file(REMOVE_RECURSE "${dir}")
message(STATUS "Pacote portatil OK: ${out}")
