# Embute a libopenblas.a dentro da libghost_runtime.a (chamado no POST_BUILD do ghost_runtime).
# Assim o programa gerado e o REPL ligam o OpenBLAS sem mudar a linha do g++.
# Uso: cmake -DRUNTIME=<libghost_runtime.a> -DBLAS=<libopenblas.a> -DAR=<ar> -P merge_blas.cmake
get_filename_component(dir "${RUNTIME}" DIRECTORY)
set(tmp "${dir}/libghost_runtime_blas.tmp.a")
set(mri "${dir}/merge_blas.mri")
file(REMOVE "${tmp}")
file(WRITE "${mri}" "create ${tmp}\naddlib ${RUNTIME}\naddlib ${BLAS}\nsave\nend\n")
execute_process(COMMAND "${AR}" -M INPUT_FILE "${mri}" RESULT_VARIABLE rc ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "Falha ao embutir o OpenBLAS no runtime: ${err}")
endif()
file(RENAME "${tmp}" "${RUNTIME}")
file(REMOVE "${mri}")
