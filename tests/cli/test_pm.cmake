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
if(NOT err6 MATCHES "falta")
    message(FATAL_ERROR "'ghost pm install' nao avisou da dependencia faltando:
${err6}")
endif()

# 7. com o pacote em ghost_packages/quantum, install passa
file(MAKE_DIRECTORY "${SANDBOX}/ghost_packages/quantum")
file(WRITE "${SANDBOX}/ghost_packages/quantum/quantum.g" "function int versao() { return 2; }
")
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

# Limpeza
file(REMOVE_RECURSE "${SANDBOX}")
message(STATUS "GhostPM validado (init, add, list, remove, install local).")
