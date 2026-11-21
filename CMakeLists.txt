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
    src/grid.cpp
    src/simulation_case.cpp
    src/verification.cpp
)
target_compile_options(heat PRIVATE -O2 -ffp-contract=off)

include(CTest)
if(BUILD_TESTING)
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
