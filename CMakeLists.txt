cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)
project(chia-plotter C CXX ASM)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fopenmp -Wall -Wno-unused-function")

include_directories(
	include
	src
)

IF (WIN32)
set(BLAKE3_SRC
    src/b3/blake3.c
    src/b3/blake3_portable.c
    src/b3/blake3_dispatch.c
    src/b3/blake3_avx2.c
    src/b3/blake3_avx512.c
    src/b3/blake3_sse41.c
)
ELSEIF(OSX_NATIVE_ARCHITECTURE STREQUAL "arm64")
set(BLAKE3_SRC
    src/b3/blake3.c
    src/b3/blake3_portable.c
    src/b3/blake3_dispatch.c
)
ELSE()
set(BLAKE3_SRC
    src/b3/blake3.c
    src/b3/blake3_portable.c
    src/b3/blake3_dispatch.c
    src/b3/blake3_avx2_x86-64_unix.S
    src/b3/blake3_avx512_x86-64_unix.S
    src/b3/blake3_sse41_x86-64_unix.S
)
ENDIF()

add_library(blake3 STATIC
	${BLAKE3_SRC}
)

add_library(chia_plotter SHARED
	src/chacha8.c
	src/phase1.cpp
)

target_link_libraries(chia_plotter blake3)

add_executable(test_disk_sort test/test_disk_sort.cpp)

add_executable(test_phase_1 test/test_phase_1.cpp)

target_link_libraries(test_phase_1 chia_plotter)
