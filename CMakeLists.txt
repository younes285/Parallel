cmake_minimum_required(VERSION 3.16)
project(heat_mpi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

add_executable(heat
    src/main.cpp
    src/binary_field.cpp
    src/cli.cpp
    src/serial.cpp
    src/serial.hpp
    src/simulation_case.cpp
    src/verification.cpp
)
target_compile_options(heat PRIVATE -O2 -ffp-contract=off)

find_package(MPI QUIET COMPONENTS CXX)
if(MPI_CXX_FOUND)
    add_library(mpi_block STATIC src/mpi_block.cpp src/grid.cpp)
    target_include_directories(mpi_block PUBLIC src)
    target_link_libraries(mpi_block PUBLIC MPI::MPI_CXX)
    target_compile_options(mpi_block PRIVATE -O2 -ffp-contract=off)
    add_library(mpi_strip STATIC src/mpi_strip.cpp src/grid.cpp)
    target_include_directories(mpi_strip PUBLIC src)
    target_link_libraries(mpi_strip PUBLIC MPI::MPI_CXX)
    target_compile_options(mpi_strip PRIVATE -O2 -ffp-contract=off)
    target_link_libraries(heat PRIVATE mpi_strip mpi_block)
    target_compile_definitions(heat PRIVATE HEAT_WITH_MPI)
else()
    target_sources(heat PRIVATE src/grid.cpp)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME benchmark_python
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/benchmark_test.py)
    endif()
    if(TARGET mpi_strip AND MPIEXEC_EXECUTABLE)
        add_executable(mpi_block_test
            tests/mpi_block_test.cpp src/serial.cpp src/simulation_case.cpp
        )
        target_link_libraries(mpi_block_test PRIVATE mpi_block)
        target_compile_options(mpi_block_test PRIVATE -O2 -ffp-contract=off)
        add_executable(mpi_strip_test
            tests/mpi_strip_test.cpp src/serial.cpp src/simulation_case.cpp
        )
        target_link_libraries(mpi_strip_test PRIVATE mpi_strip)
        target_compile_options(mpi_strip_test PRIVATE -O2 -ffp-contract=off)
        foreach(ranks RANGE 1 4)
            add_test(NAME mpi_block_${ranks}
                COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${ranks}
                        ${MPIEXEC_PREFLAGS} $<TARGET_FILE:mpi_block_test> ${MPIEXEC_POSTFLAGS})
            set_tests_properties(mpi_block_${ranks} PROPERTIES PROCESSORS ${ranks} TIMEOUT 30)
            add_test(NAME mpi_strip_${ranks}
                COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${ranks}
                        ${MPIEXEC_PREFLAGS} $<TARGET_FILE:mpi_strip_test> ${MPIEXEC_POSTFLAGS})
            set_tests_properties(mpi_strip_${ranks} PROPERTIES PROCESSORS ${ranks} TIMEOUT 30)
            add_test(NAME mpi_cli_${ranks}
                COMMAND ${CMAKE_COMMAND} "-DHEAT=$<TARGET_FILE:heat>" "-DRANKS=${ranks}"
                        "-DMPIEXEC=${MPIEXEC_EXECUTABLE}" "-DNUMPROC=${MPIEXEC_NUMPROC_FLAG}"
                        "-DPREFLAGS=${MPIEXEC_PREFLAGS}" "-DPOSTFLAGS=${MPIEXEC_POSTFLAGS}"
                        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/mpi_cli_test.cmake)
            set_tests_properties(mpi_cli_${ranks} PROPERTIES PROCESSORS ${ranks} TIMEOUT 60)
            add_test(NAME mpi_cli_2d_${ranks}
                COMMAND ${CMAKE_COMMAND} "-DHEAT=$<TARGET_FILE:heat>" "-DRANKS=${ranks}" "-DMODE=2d"
                        "-DMPIEXEC=${MPIEXEC_EXECUTABLE}" "-DNUMPROC=${MPIEXEC_NUMPROC_FLAG}"
                        "-DPREFLAGS=${MPIEXEC_PREFLAGS}" "-DPOSTFLAGS=${MPIEXEC_POSTFLAGS}"
                        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/mpi_cli_test.cmake)
            set_tests_properties(mpi_cli_2d_${ranks} PROPERTIES PROCESSORS ${ranks} TIMEOUT 60)
        endforeach()
    endif()

    add_executable(instability_experiment
        tests/instability_experiment.cpp src/grid.cpp src/binary_field.cpp
    )
    target_include_directories(instability_experiment PRIVATE src)
    target_compile_options(instability_experiment PRIVATE -O2 -ffp-contract=off)

    add_executable(grid_test tests/grid_test.cpp src/grid.cpp)
    target_include_directories(grid_test PRIVATE src)
    add_test(NAME grid COMMAND grid_test)

    add_executable(simulation_case_test
        tests/simulation_case_test.cpp
        src/grid.cpp
        src/simulation_case.cpp
    )
    target_include_directories(simulation_case_test PRIVATE src)
    add_test(NAME simulation_cases COMMAND simulation_case_test)

    add_executable(heat_step_test
        tests/heat_step_test.cpp src/grid.cpp src/serial.cpp src/simulation_case.cpp
    )
    target_include_directories(heat_step_test PRIVATE src)
    target_compile_options(heat_step_test PRIVATE -O2 -ffp-contract=off)
    add_test(NAME heat_step COMMAND heat_step_test)

    add_executable(serial_test
        tests/serial_test.cpp src/grid.cpp src/serial.cpp src/simulation_case.cpp
    )
    target_include_directories(serial_test PRIVATE src)
    target_compile_options(serial_test PRIVATE -O2 -ffp-contract=off)
    add_test(NAME serial_simulation COMMAND serial_test)
    add_executable(verification_test tests/verification_test.cpp src/grid.cpp src/verification.cpp)
    target_include_directories(verification_test PRIVATE src)
    add_test(NAME verification COMMAND verification_test)
    add_test(NAME cli COMMAND ${CMAKE_COMMAND} -DHEAT=$<TARGET_FILE:heat>
             -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_test.cmake)
endif()
