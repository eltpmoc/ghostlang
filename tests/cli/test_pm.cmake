# Teste automatizado do GhostPM (Gerenciador de Pacotes)
# Uso: cmake -DGHOST=<ghost.exe> -P test_pm.cmake

set(SANDBOX "${CMAKE_CURRENT_BINARY_DIR}/pm_test_sandbox")
file(REMOVE_RECURSE "${SANDBOX}")
file(MAKE_DIRECTORY "${SANDBOX}")

# 1. ghost pm init
execute_process(
    COMMAND "${GHOST}" pm init demo_pkg
    WORKING_DIRECTORY "${SANDBOX}"
    RESULT_VARIABLE rc1
    OUTPUT_VARIABLE out1
)
if(NOT rc1 EQUAL 0)
    message(FATAL_ERROR "Falha no 'ghost pm init':\n${out1}")
endif()

if(NOT EXISTS "${SANDBOX}/ghost.toml")
    message(FATAL_ERROR "ghost.toml nao foi criado!")
endif()
if(NOT EXISTS "${SANDBOX}/ghost.lock")
    message(FATAL_ERROR "ghost.lock nao foi criado!")
endif()

# 2. ghost pm add math 1.0.0
execute_process(
    COMMAND "${GHOST}" pm add math 1.0.0
    WORKING_DIRECTORY "${SANDBOX}"
    RESULT_VARIABLE rc2
    OUTPUT_VARIABLE out2
)
if(NOT rc2 EQUAL 0)
    message(FATAL_ERROR "Falha no 'ghost pm add':\n${out2}")
endif()

# 3. ghost pm add quantum 2.0.0
execute_process(
    COMMAND "${GHOST}" pm add quantum 2.0.0
    WORKING_DIRECTORY "${SANDBOX}"
    RESULT_VARIABLE rc3
    OUTPUT_VARIABLE out3
)
if(NOT rc3 EQUAL 0)
    message(FATAL_ERROR "Falha no 'ghost pm add quantum':\n${out3}")
endif()

# 4. ghost pm list
execute_process(
    COMMAND "${GHOST}" pm list
    WORKING_DIRECTORY "${SANDBOX}"
    RESULT_VARIABLE rc4
    OUTPUT_VARIABLE out4
)
if(NOT rc4 EQUAL 0)
    message(FATAL_ERROR "Falha no 'ghost pm list':\n${out4}")
endif()

# 5. ghost pm remove math
execute_process(
    COMMAND "${GHOST}" pm remove math
    WORKING_DIRECTORY "${SANDBOX}"
    RESULT_VARIABLE rc5
    OUTPUT_VARIABLE out5
)
if(NOT rc5 EQUAL 0)
    message(FATAL_ERROR "Falha no 'ghost pm remove':\n${out5}")
endif()

# 6. ghost pm install sem o pacote local: deve falhar (nao ha registro remoto)
execute_process(
    COMMAND "${GHOST}" pm install
    WORKING_DIRECTORY "${SANDBOX}"
    RESULT_VARIABLE rc6
    OUTPUT_VARIABLE out6
    ERROR_VARIABLE err6
)
if(rc6 EQUAL 0)
    message(FATAL_ERROR "'ghost pm install' aceitou 'quantum' sem ghost_packages/quantum:
${out6}")
endif()
if(NOT err6 MATCHES "nao ha registro remoto")
    message(FATAL_ERROR "'ghost pm install' nao avisou da dependencia faltando:
${err6}")
endif()

# 7. com o pacote em ghost_packages/quantum, install passa
file(MAKE_DIRECTORY "${SANDBOX}/ghost_packages/quantum")
file(WRITE "${SANDBOX}/ghost_packages/quantum/quantum.g" "function int versao() { return 2; }
")
file(WRITE "${SANDBOX}/ghost_packages/quantum/ghost.toml" "[package]\nname = \"quantum\"\nversion = \"2.0.0\"\n")
execute_process(
    COMMAND "${GHOST}" pm install
    WORKING_DIRECTORY "${SANDBOX}"
    RESULT_VARIABLE rc7
    OUTPUT_VARIABLE out7
    ERROR_VARIABLE err7
)
if(NOT rc7 EQUAL 0)
    message(FATAL_ERROR "Falha no 'ghost pm install' com o pacote local:
${out7}${err7}")
endif()

macro(pm_ok rcvar)
  execute_process(COMMAND "${GHOST}" pm ${ARGN} WORKING_DIRECTORY "${SANDBOX}"
                  RESULT_VARIABLE ${rcvar} OUTPUT_VARIABLE pm_out ERROR_VARIABLE pm_err)
endmacro()

# 8. semver: ^2.0 aceita 2.0.0; ^3.0 recusa
pm_ok(rc8 add quantum ^2.0)
pm_ok(rc8b install)
if(NOT rc8b EQUAL 0)
  message(FATAL_ERROR "^2.0 deveria aceitar 2.0.0:\n${pm_out}${pm_err}")
endif()
pm_ok(rc9 add quantum ^3.0)
pm_ok(rc9b install)
if(rc9b EQUAL 0 OR NOT pm_err MATCHES "nao atende")
  message(FATAL_ERROR "^3.0 deveria recusar 2.0.0:\n${pm_out}${pm_err}")
endif()
pm_ok(rc9c add quantum ~2.0.0)

# 9. path: copia a pasta para ghost_packages/<nome> e o programa importa o pacote
file(MAKE_DIRECTORY "${SANDBOX}/fontes/geometria")
file(WRITE "${SANDBOX}/fontes/geometria/ghost.toml" "[package]\nname = \"geometria\"\nversion = \"1.4.0\"\n")
file(WRITE "${SANDBOX}/fontes/geometria/geometria.g" "function double area_quadrado(double l) {\n    return l * l;\n}\n")
pm_ok(rc10 add geometria --path fontes/geometria)
pm_ok(rc10b install)
if(NOT rc10b EQUAL 0 OR NOT EXISTS "${SANDBOX}/ghost_packages/geometria/geometria.g")
  message(FATAL_ERROR "install de path: falhou:\n${pm_out}${pm_err}")
endif()
file(WRITE "${SANDBOX}/usa.g" "import \"ghost_packages/geometria/geometria.g\"\nfunction main() {\n    print(\"area: \", area_quadrado(3.0));\n}\n")
execute_process(COMMAND "${GHOST}" run usa.g WORKING_DIRECTORY "${SANDBOX}" RESULT_VARIABLE rc11 OUTPUT_VARIABLE o11 ERROR_VARIABLE e11)
if(NOT rc11 EQUAL 0 OR NOT o11 MATCHES "area: 9")
  message(FATAL_ERROR "programa nao usou o pacote instalado:\n${o11}${e11}")
endif()

# 10. git: clona um repositorio (local, file://) com tag e grava o commit no ghost.lock
find_program(GIT git)
if(NOT GIT)
  message(STATUS "git nao encontrado: parte git: do teste pulada")
endif()
if(GIT)
  message(STATUS "testando git: com ${GIT}")
  set(repo "${SANDBOX}/repo_cores")
  file(MAKE_DIRECTORY "${repo}")
  file(WRITE "${repo}/ghost.toml" "[package]\nname = \"cores\"\nversion = \"0.3.0\"\n")
  file(WRITE "${repo}/cores.g" "function string cor() {\n    return \"azul\";\n}\n")
  foreach(cmd "init -q" "add ." "-c user.name=t -c user.email=t@t commit -q -m inicial" "tag v0.3.0")
    separate_arguments(args NATIVE_COMMAND "${cmd}")
    execute_process(COMMAND "${GIT}" ${args} WORKING_DIRECTORY "${repo}" RESULT_VARIABLE grc)
    if(NOT grc EQUAL 0)
      message(FATAL_ERROR "git ${cmd} falhou no repositorio de teste")
    endif()
  endforeach()
  pm_ok(rc12 add cores --git "file:///${repo}" --ref v0.3.0)
  pm_ok(rc12b install)
  if(NOT rc12b EQUAL 0 OR NOT EXISTS "${SANDBOX}/ghost_packages/cores/cores.g")
    message(FATAL_ERROR "install de git: falhou:\n${pm_out}${pm_err}")
  endif()
  file(READ "${SANDBOX}/ghost.lock" lock)
  if(NOT lock MATCHES "commit = \"[0-9a-f]+\"" OR NOT lock MATCHES "version = \"0.3.0\"")
    message(FATAL_ERROR "ghost.lock sem commit ou versao do pacote git:\n${lock}")
  endif()
  # URL com metacaractere de shell e recusada antes de chamar o git
  pm_ok(rc13 add mal --git "file:///x\"&calc")
  pm_ok(rc13b install)
  if(rc13b EQUAL 0 OR NOT pm_err MATCHES "nao permitidos")
    message(FATAL_ERROR "URL perigosa nao foi recusada:\n${pm_out}${pm_err}")
  endif()
endif()

# Limpeza
file(REMOVE_RECURSE "${SANDBOX}")
message(STATUS "GhostPM validado (init, add, list, remove, semver, install de pasta e de git).")
