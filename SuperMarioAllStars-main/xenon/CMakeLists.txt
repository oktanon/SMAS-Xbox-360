# Build del port a Xbox 360 con xsdk (AGENTS.md §3). Desde la raíz del repositorio, en WSL:
#   cmake -S SuperMarioAllStars-main/xenon -B build-xenon -GNinja -DCMAKE_TOOLCHAIN_FILE=$PWD/xsdk-main/cmake/xsdk.cmake
#   cmake --build build-xenon
# Hoy: smas_hito2.xex (-O2) y smas_hito2_o3.xex (-O3), el emulador sin ventana con el guion de xenon/qemu más
# benchmarks (hitos 2 y 2b). El juego se añade aquí después.
cmake_minimum_required(VERSION 3.25)
project(SMASXenon CXX)

set(GAME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(SRC ${GAME_DIR}/src)

# El núcleo y los juegos, los mismos ficheros que el build de Windows, Linux y Android (CORE_SOURCES de upstream).
set(CORE_SOURCES
  ${SRC}/core/cpu65816.cpp
  ${SRC}/core/apu.cpp
  ${SRC}/core/dsp.cpp
  ${SRC}/core/ppu.cpp
  ${SRC}/core/snes.cpp
  ${SRC}/core/disasm.cpp
  ${SRC}/core/png.cpp
  ${SRC}/core/fileio.cpp
  ${SRC}/core/rewind.cpp
  ${SRC}/core/msu1.cpp
  ${SRC}/game/achievements.cpp
  ${SRC}/game/smas.cpp
  ${SRC}/game/smb1.cpp
  ${SRC}/game/smb1coop.cpp
  ${SRC}/game/palette.cpp
  ${SRC}/game/smb3.cpp
  ${SRC}/game/smb3coop.cpp
  ${SRC}/game/smb2.cpp
)

# smascore<sufijo> y smas_hito2<sufijo>.xex con las opciones dadas. Siempre, como en la comparación de qemu
# (xenon/qemu/build.sh): -ffp-contract=off (GCC en PowerPC funde a*b+c en fmadd) y -fno-strict-aliasing.
function(smas_hito2_variant suffix opt)
  add_library(smascore${suffix} STATIC ${CORE_SOURCES})
  target_include_directories(smascore${suffix} PUBLIC ${SRC})
  set_target_properties(smascore${suffix} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_compile_options(smascore${suffix} PUBLIC ${opt} -ffp-contract=off -fno-strict-aliasing)
  target_compile_definitions(smascore${suffix} PUBLIC SMAS_XENON SMAS_BIG_ENDIAN)

  # smastool con su main renombrado y un main de Xbox (src/platform/xenon/hito2_main.cpp)
  add_library(smastool_obj${suffix} OBJECT ${GAME_DIR}/tools/smastool.cpp)
  target_link_libraries(smastool_obj${suffix} PRIVATE smascore${suffix})
  target_compile_definitions(smastool_obj${suffix} PRIVATE main=smastool_main)
  set_target_properties(smastool_obj${suffix} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

  add_executable(smas_hito2${suffix} ${SRC}/platform/xenon/hito2_main.cpp $<TARGET_OBJECTS:smastool_obj${suffix}>)
  target_link_libraries(smas_hito2${suffix} PRIVATE smascore${suffix})
  set_target_properties(smas_hito2${suffix} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  xsdk_add_xex(smas_hito2${suffix} NAME "SMAS hito 2${suffix}")
endfunction()

smas_hito2_variant("" -O2)
smas_hito2_variant("_o3" -O3)

# El guion de la consola: el de qemu tal cual, más los benchmarks al final (no imprimen líneas que se comparen).
set(GUION_CONSOLA ${CMAKE_CURRENT_BINARY_DIR}/guion.txt)
add_custom_command(OUTPUT ${GUION_CONSOLA}
  COMMAND ${CMAKE_COMMAND} -E cat ${CMAKE_CURRENT_SOURCE_DIR}/qemu/guion.txt
          ${CMAKE_CURRENT_SOURCE_DIR}/hito2_bench.txt > ${GUION_CONSOLA}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/qemu/guion.txt ${CMAKE_CURRENT_SOURCE_DIR}/hito2_bench.txt
  VERBATIM)
add_custom_target(guion_consola ALL DEPENDS ${GUION_CONSOLA})
