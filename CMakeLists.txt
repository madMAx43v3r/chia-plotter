cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
find_package(Git)
execute_process(
  COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions("-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\"")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release"
    CACHE STRING "Possible values are empty, Debug, Release, RelWithDebInfo, MinSizeRel, ..."
    FORCE
  )
endif()

project(chia-plotter C CXX ASM)

include(FetchContent)

option(CHIA_PLOTTER_BUILD_TESTS "" off)

if(MSVC)
  enable_language(ASM_MASM)
  set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob2 /GL /DNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi /Ob0 /Od /RTC1")
  FetchContent_Declare(uint128_t
    GIT_REPOSITORY https://github.com/AmineKhaldi/uint128_t.git
    GIT_TAG "848d3c6ecff2b757c5e51cc943e9af7087313c79"
  )
  FetchContent_MakeAvailable(uint128_t)
endif()

set(TARGET_ARCH ${CMAKE_HOST_SYSTEM_PROCESSOR})
message(STATUS "${PROJECT_NAME} - Architecture: ${TARGET_ARCH}")

FetchContent_Declare(bls_signatures
  GIT_REPOSITORY https://github.com/Chia-Network/bls-signatures.git
  GIT_TAG "b3acfe5ecd23660f8ce21a2aafb9d14c3263fef7"
)
set(BUILD_BLS_PYTHON_BINDINGS "0" CACHE STRING "")
set(BUILD_BLS_TESTS "0" CACHE STRING "")
set(BUILD_BLS_BENCHMARKS "0" CACHE STRING "")
FetchContent_MakeAvailable(bls_signatures)

FetchContent_Declare(libbech32
  GIT_REPOSITORY https://github.com/AmineKhaldi/libbech32.git
  GIT_TAG "c722f30e8c7b0ea2d01988e02587993f5c9ab2bf"
)
set(LIBBECH32_BUILD_TESTS "off" CACHE STRING "")
set(LIBBECH32_BUILD_EXAMPLES "off" CACHE STRING "")
FetchContent_MakeAvailable(libbech32)

FetchContent_Declare(blake3
  GIT_REPOSITORY https://github.com/BLAKE3-team/BLAKE3.git
  GIT_TAG "7d8f7101f1c912fd3ecea3d2d0904700d81ef3d2"
)
FetchContent_MakeAvailable(blake3)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
endif()

include_directories(
  lib
  include
  ${blake3_SOURCE_DIR}/c
)

list(APPEND BLAKE3_SRC
  ${blake3_SOURCE_DIR}/c/blake3.c
  ${blake3_SOURCE_DIR}/c/blake3_portable.c
  ${blake3_SOURCE_DIR}/c/blake3_dispatch.c
)

if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
  if(MSVC)
    list(APPEND BLAKE3_SRC
      ${blake3_SOURCE_DIR}/c/blake3_sse2_x86-64_windows_msvc.asm
      ${blake3_SOURCE_DIR}/c/blake3_sse41_x86-64_windows_msvc.asm
      ${blake3_SOURCE_DIR}/c/blake3_avx2_x86-64_windows_msvc.asm
      ${blake3_SOURCE_DIR}/c/blake3_avx512_x86-64_windows_msvc.asm
    )
  else()
    list(APPEND BLAKE3_SRC
      ${blake3_SOURCE_DIR}/c/blake3_sse2_x86-64_windows_gnu.S
      ${blake3_SOURCE_DIR}/c/blake3_sse41_x86-64_windows_gnu.S
      ${blake3_SOURCE_DIR}/c/blake3_avx2_x86-64_windows_gnu.S
      ${blake3_SOURCE_DIR}/c/blake3_avx512_x86-64_windows_gnu.S
    )
  endif()
elseif(TARGET_ARCH STREQUAL "x86_64")
  list(APPEND BLAKE3_SRC
    ${blake3_SOURCE_DIR}/c/blake3_avx2_x86-64_unix.S
    ${blake3_SOURCE_DIR}/c/blake3_avx512_x86-64_unix.S
    ${blake3_SOURCE_DIR}/c/blake3_sse41_x86-64_unix.S
    ${blake3_SOURCE_DIR}/c/blake3_sse2_x86-64_unix.S
  )
endif()

add_library(blake3 ${BLAKE3_SRC})

add_library(fse
  lib/FSE/lib/fse_compress.c
  lib/FSE/lib/fse_decompress.c
  lib/FSE/lib/entropy_common.c
  lib/FSE/lib/hist.c
)

add_library(chia_plotter
  lib/chacha8.c
  src/settings.cpp
)

target_link_libraries(chia_plotter
  PRIVATE
    blake3
    fse
    bech32
)

if(MSVC)
  add_library(uint128 ${uint128_t_SOURCE_DIR}/uint128_t.cpp)
  target_include_directories(uint128
    PUBLIC
      ${uint128_t_SOURCE_DIR}
  )
  target_link_libraries(chia_plotter
    PUBLIC
      uint128
  )
else()
  find_package(Threads REQUIRED)
  target_link_libraries(chia_plotter
    PRIVATE
      Threads::Threads
  )
endif()

add_executable(chia_plot src/chia_plot.cpp)

target_include_directories(chia_plot
  PRIVATE
    ${libbech32_SOURCE_DIR}/include/libbech32
)

if(MSVC)
  set_source_files_properties(src/chia_plot.cpp
    PROPERTIES
      COMPILE_OPTIONS "/bigobj"
  )
endif()

target_link_libraries(chia_plot
  PRIVATE
    chia_plotter
    bls
)

add_executable(chia_plot_k34 src/chia_plot.cpp)

target_include_directories(chia_plot_k34
  PRIVATE
    ${libbech32_SOURCE_DIR}/include/libbech32
)

target_compile_definitions(chia_plot_k34
  PUBLIC
    CHIA_K34=1
)

target_link_libraries(chia_plot_k34
  PRIVATE
    chia_plotter
    bls
)

if(CHIA_PLOTTER_BUILD_TESTS)
  add_executable(test_copy test/test_copy.cpp)
  target_link_libraries(test_copy PRIVATE chia_plotter)

  add_executable(test_disk_sort test/test_disk_sort.cpp)
  target_link_libraries(test_disk_sort PRIVATE chia_plotter)

  add_executable(test_phase_1 test/test_phase_1.cpp)
  target_link_libraries(test_phase_1 PRIVATE chia_plotter)

  add_executable(test_phase_2 test/test_phase_2.cpp)
  target_link_libraries(test_phase_2 PRIVATE chia_plotter)

  add_executable(test_phase_3 test/test_phase_3.cpp)
  target_link_libraries(test_phase_3 PRIVATE chia_plotter)

  add_executable(test_phase_4 test/test_phase_4.cpp)
  target_link_libraries(test_phase_4 PRIVATE chia_plotter)

  add_executable(check_phase_1 test/check_phase_1.cpp)
  target_link_libraries(check_phase_1 PRIVATE chia_plotter)
endif()
