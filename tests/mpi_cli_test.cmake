if(NOT DEFINED MODE)
    set(MODE 1d)
endif()

set(prefix "${CMAKE_CURRENT_BINARY_DIR}/mpi-cli-${MODE}-${RANKS}")
set(serial_file "${prefix}-serial.bin")
set(mpi_file "${prefix}-${MODE}.bin")
set(launch "${MPIEXEC}" "${NUMPROC}" "${RANKS}" ${PREFLAGS} "${HEAT}" ${POSTFLAGS})

foreach(case 1 2 V)
    set(args --case ${case} --N 8 --r 0.2 --T 0.03)
    execute_process(COMMAND "${HEAT}" ${args} --out "${serial_file}"
                    RESULT_VARIABLE serial_status OUTPUT_VARIABLE serial_summary)
    execute_process(COMMAND ${launch} ${args} --decomp ${MODE} --out "${mpi_file}"
                    RESULT_VARIABLE mpi_status OUTPUT_VARIABLE summary ERROR_VARIABLE error)
    string(STRIP "${summary}" summary)
    if(NOT serial_status STREQUAL "0" OR NOT mpi_status STREQUAL "0" OR
       NOT summary MATCHES "^decomp=${MODE} P=${RANKS} case=${case} N=8 steps=[0-9]+ r=[^ ]+ dt=[^ ]+ T=[^ ]+ t_total=[^ ]+ t_comp=[^ ]+ t_comm=[^ ]+" OR
       summary MATCHES "\n")
        message(FATAL_ERROR "Unexpected MPI CLI result: ${mpi_status}, ${summary}, ${error}")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${serial_file}" "${mpi_file}"
                    RESULT_VARIABLE compared)
    if(NOT compared STREQUAL "0")
        message(FATAL_ERROR "MPI field differs from serial for case ${case}")
    endif()
    if(case STREQUAL "V")
        string(REGEX MATCH "err_inf=[^ \n]+ err_l2=[^ \n]+" serial_errors "${serial_summary}")
        string(REGEX MATCH "err_inf=[^ \n]+ err_l2=[^ \n]+" mpi_errors "${summary}")
        if(serial_errors STREQUAL "" OR NOT serial_errors STREQUAL mpi_errors)
            message(FATAL_ERROR "MPI verification errors differ from serial")
        endif()
    elseif(summary MATCHES "err_")
        message(FATAL_ERROR "Unexpected verification errors for case ${case}")
    endif()
endforeach()

if(RANKS EQUAL 2)
    execute_process(COMMAND ${launch} --case 1 --N 8 --r 0.3 --steps 1 --decomp ${MODE}
                    RESULT_VARIABLE status OUTPUT_VARIABLE summary ERROR_VARIABLE error)
    if(NOT status STREQUAL "1" OR NOT summary STREQUAL "" OR NOT error MATCHES "--r must be finite")
        message(FATAL_ERROR "Unexpected invalid-input result: ${status}, ${summary}, ${error}")
    endif()
    execute_process(COMMAND ${launch} --case 1 --N 8 --r 0.2 --steps 1 --decomp ${MODE}
                            --out "${mpi_file}/child.bin"
                    RESULT_VARIABLE status OUTPUT_VARIABLE summary ERROR_VARIABLE error)
    if(NOT status STREQUAL "1" OR NOT summary STREQUAL "" OR NOT error MATCHES "Cannot open output file")
        message(FATAL_ERROR "Unexpected output-failure result: ${status}, ${summary}, ${error}")
    endif()
endif()

if(MODE STREQUAL "2d" AND RANKS EQUAL 4)
    execute_process(COMMAND ${launch} --case 1 --N 2 --r 0.2 --steps 1 --decomp 2d
                    RESULT_VARIABLE status OUTPUT_VARIABLE summary ERROR_VARIABLE error)
    if(NOT status STREQUAL "1" OR NOT summary STREQUAL "" OR
       NOT error MATCHES "requires at least one interior cell per axis per rank")
        message(FATAL_ERROR "Unexpected empty-block result: ${status}, ${summary}, ${error}")
    endif()
endif()
