cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
project(chia-plotter C CXX ASM)

add_subdirectory(lib/bls-signatures)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wno-unused-function")

include_directories(
	lib
	include
	lib/bls-signatures/src/
)

IF (WIN32)
set(BLAKE3_SRC
    lib/b3/blake3.c
    lib/b3/blake3_portable.c
    lib/b3/blake3_dispatch.c
    lib/b3/blake3_avx2.c
    lib/b3/blake3_avx512.c
    lib/b3/blake3_sse41.c
)
ELSEIF(OSX_NATIVE_ARCHITECTURE STREQUAL "arm64")
set(BLAKE3_SRC
    lib/b3/blake3.c
    lib/b3/blake3_portable.c
    lib/b3/blake3_dispatch.c
)
ELSE()
set(BLAKE3_SRC
    lib/b3/blake3.c
    lib/b3/blake3_portable.c
    lib/b3/blake3_dispatch.c
    lib/b3/blake3_avx2_x86-64_unix.S
    lib/b3/blake3_avx512_x86-64_unix.S
    lib/b3/blake3_sse41_x86-64_unix.S
)
ENDIF()

set(FSE_LIB lib/FSE/lib)
set(FSE_FILES
    ${FSE_LIB}/fse_compress.c
    ${FSE_LIB}/fse_decompress.c
    ${FSE_LIB}/entropy_common.c
    ${FSE_LIB}/hist.c
)

add_library(blake3 STATIC ${BLAKE3_SRC})

add_library(fse STATIC ${FSE_FILES})

add_library(chia_plotter STATIC
	lib/chacha8.c
)

target_link_libraries(chia_plotter blake3 fse)

add_executable(test_disk_sort test/test_disk_sort.cpp)

add_executable(test_phase_1 test/test_phase_1.cpp)
add_executable(test_phase_2 test/test_phase_2.cpp)
add_executable(test_phase_3 test/test_phase_3.cpp)
add_executable(test_phase_4 test/test_phase_4.cpp)

add_executable(check_phase_1 test/check_phase_1.cpp)

add_executable(chia_plot src/chia_plot.cpp)

target_link_libraries(test_phase_1 chia_plotter)
target_link_libraries(test_phase_2 chia_plotter)
target_link_libraries(test_phase_3 chia_plotter)
target_link_libraries(test_phase_4 chia_plotter)

target_link_libraries(check_phase_1 chia_plotter)

target_link_libraries(chia_plot chia_plotter bls stdc++fs)
