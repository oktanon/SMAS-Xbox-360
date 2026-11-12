# Build del port a Xbox 360 con xsdk (AGENTS.md §3). Desde la raíz del repositorio, en WSL:
#   cmake -S SuperMarioAllStars-main/xenon -B build-xenon -GNinja -DCMAKE_TOOLCHAIN_FILE=$PWD/xsdk-main/cmake/xsdk.cmake
#   cmake --build build-xenon
# SuperMarioAllStars.xex: el juego (hito 5, al final). Y las pruebas de los hitos: smas_hito2.xex (-O3, el emulador sin
# ventana con el guion de xenon/qemu más benchmarks), smas_video.xex (hito 3), smas_jugar.xex (hito 4).
cmake_minimum_required(VERSION 3.25)
project(SMASXenon C CXX)

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
  target_compile_definitions(smascore${suffix} PUBLIC SMAS_XENON SMAS_BIG_ENDIAN SMAS_PPU_FAST SMAS_CPU_FAST SMAS_APU_THREAD SMAS_PORT_FIXES)

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

# (-O2 y -O3 medidos en la consola, runs 010 y 012: el núcleo va con -O3)
smas_hito2_variant("" -O3)

# El guion de la consola: dibujar con 2 hilos (hito2_hilos.txt), el de qemu tal cual (sus líneas se comparan con
# la run 004: el dibujo en hilos debe dar lo mismo) y los benchmarks al final (no imprimen líneas que se comparen).
set(GUION_CONSOLA ${CMAKE_CURRENT_BINARY_DIR}/guion.txt)
add_custom_command(OUTPUT ${GUION_CONSOLA}
  COMMAND ${CMAKE_COMMAND} -E cat ${CMAKE_CURRENT_SOURCE_DIR}/hito2_hilos.txt ${CMAKE_CURRENT_SOURCE_DIR}/qemu/guion.txt
          ${CMAKE_CURRENT_SOURCE_DIR}/hito2_bench.txt > ${GUION_CONSOLA}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hito2_hilos.txt ${CMAKE_CURRENT_SOURCE_DIR}/qemu/guion.txt
          ${CMAKE_CURRENT_SOURCE_DIR}/hito2_bench.txt
  VERBATIM)
add_custom_target(guion_consola ALL DEPENDS ${GUION_CONSOLA})

# Hito 3: smas_video.xex, el vídeo con vulkan4xenon (src/platform/xenon/video_vk.cpp, video_main.cpp). Los shaders de
# src/platform/xenon/shaders se compilan con vulkan4xenon-main/tools/build_shaders.py a screen_shaders.c/.h, que se
# suben al repositorio (como hace vulkan4xenon con los suyos): ver AGENTS.md §3.
set(V4X_DIR ${GAME_DIR}/../vulkan4xenon-main CACHE PATH "vulkan4xenon (compilado: build/libvulkan4xenon.a)")
add_executable(smas_video
  ${SRC}/platform/xenon/video_main.cpp
  ${SRC}/platform/xenon/video_vk.cpp
  ${SRC}/platform/xenon/screen_shaders.c
  $<TARGET_OBJECTS:smastool_obj>)
target_include_directories(smas_video PRIVATE ${V4X_DIR}/include ${V4X_DIR}/third_party/vulkan-headers/include
  ${V4X_DIR}/tests)
target_link_libraries(smas_video PRIVATE smascore ${V4X_DIR}/build/libvulkan4xenon.a)
set_target_properties(smas_video PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON C_STANDARD 17 C_EXTENSIONS ON)
xsdk_add_xex(smas_video NAME "SMAS hito 3 video")

# Su guion: dibujar con 2 hilos, el guion de comparación (líneas comparadas con la run 004) y la demostración.
set(GUION_VIDEO ${CMAKE_CURRENT_BINARY_DIR}/guion_video.txt)
add_custom_command(OUTPUT ${GUION_VIDEO}
  COMMAND ${CMAKE_COMMAND} -E cat ${CMAKE_CURRENT_SOURCE_DIR}/hito2_hilos.txt ${CMAKE_CURRENT_SOURCE_DIR}/qemu/guion.txt
          ${CMAKE_CURRENT_SOURCE_DIR}/hito3_demo.txt > ${GUION_VIDEO}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/hito2_hilos.txt ${CMAKE_CURRENT_SOURCE_DIR}/qemu/guion.txt
          ${CMAKE_CURRENT_SOURCE_DIR}/hito3_demo.txt
  VERBATIM)
add_custom_target(guion_video ALL DEPENDS ${GUION_VIDEO})

# Hito 4: smas_jugar.xex, el cartucho jugable sin la App (mando y sonido por SDL3, vídeo por VideoVk), para medir el
# juego solo (runs 017-022).
set(SDL3_DIR ${GAME_DIR}/../SDL-xenon-xenon/build-xsdk CACHE PATH "SDL-xenon compilado (SDL3Config.cmake)")
find_package(SDL3 REQUIRED CONFIG)
add_executable(smas_jugar
  ${SRC}/platform/xenon/play_main.cpp
  ${SRC}/platform/xenon/video_vk.cpp
  ${SRC}/platform/xenon/screen_shaders.c
  ${SRC}/platform/xenon/audio_sdl3.cpp
  ${SRC}/platform/xenon/input_sdl3.cpp
  ${SRC}/app/config.cpp
  ${SRC}/app/utf8.cpp
  ${SRC}/app/sys_posix.cpp)
target_include_directories(smas_jugar PRIVATE ${V4X_DIR}/include ${V4X_DIR}/third_party/vulkan-headers/include
  ${V4X_DIR}/tests)
target_link_libraries(smas_jugar PRIVATE smascore SDL3::SDL3-static ${V4X_DIR}/build/libvulkan4xenon.a)
set_target_properties(smas_jugar PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON C_STANDARD 17 C_EXTENSIONS ON)
xsdk_add_xex(smas_jugar NAME "SMAS hito 4")

# Hito 5: SuperMarioAllStars.xex, el juego: la App de upstream entera (src/app: launcher, menús, logros, música) como
# la compila Linux (CMakeLists.txt de upstream, UNIX AND NOT ANDROID), con las capas de Xbox (src/platform/xenon:
# app_xenon.cpp, video_xenon.cpp con su Canvas, audio_sdl3.cpp, input_sdl3.cpp) en lugar de las de SDL2 / OpenGL ES.
# Con su ID de título y su nombre (AGENTS.md §7): un solo sitio para los dos, SMAS_TITLE_ID / SMAS_TITLE_NAME.
set(SMAS_TITLE_ID 534D4153)  # "SMAS"
set(SMAS_TITLE_NAME "Super Mario All-Stars")
set(SMAS_VERSION 1.0.0.0)    # (hito 10: la primera versión publicada)
set(SMAS_FONTS ${GAME_DIR}/third_party/fonts)
add_executable(SuperMarioAllStars
  ${SRC}/app/app.cpp
  ${SRC}/app/config.cpp
  ${SRC}/app/menu.cpp
  ${SRC}/app/launcher.cpp
  ${SRC}/app/select_covers.cpp
  ${SRC}/app/file_select.cpp
  ${SRC}/app/music.cpp
  ${SRC}/app/achstore.cpp
  ${SRC}/app/utf8.cpp
  ${SRC}/app/sys_posix.cpp
  ${SRC}/app/imageload.cpp
  ${SRC}/app/msupack.cpp
  ${SRC}/app/achexport.cpp
  ${SRC}/app/updatecheck.cpp
  ${SRC}/platform/android/touch.cpp
  ${SRC}/platform/xenon/app_xenon.cpp
  ${SRC}/platform/xenon/video_xenon.cpp
  ${SRC}/platform/xenon/canvas_shaders.c
  ${SRC}/platform/xenon/fonts_xenon.cpp
  ${SRC}/platform/xenon/audio_sdl3.cpp
  ${SRC}/platform/xenon/input_sdl3.cpp)
target_include_directories(SuperMarioAllStars PRIVATE ${GAME_DIR}/third_party/stb ${V4X_DIR}/include
  ${V4X_DIR}/third_party/vulkan-headers/include)
target_link_libraries(SuperMarioAllStars PRIVATE smascore SDL3::SDL3-static ${V4X_DIR}/build/libvulkan4xenon.a)
target_compile_options(SuperMarioAllStars PRIVATE -O2 -Wno-unused-parameter -Wno-sign-compare)
target_compile_definitions(SuperMarioAllStars PRIVATE SMAS_TITLE_ID_HEX="${SMAS_TITLE_ID}"
  SMAS_TITLE_NAME="${SMAS_TITLE_NAME}")
# (las fuentes van dentro del .xex: fonts_xenon.cpp las incluye con .incbin)
set_source_files_properties(${SRC}/platform/xenon/fonts_xenon.cpp PROPERTIES
  COMPILE_DEFINITIONS "SMAS_FONT_DIR=\"${SMAS_FONTS}\""
  OBJECT_DEPENDS "${SMAS_FONTS}/LuckiestGuy-Regular.ttf;${SMAS_FONTS}/DejaVuSans.ttf;${SMAS_FONTS}/DejaVuSans-Bold.ttf")
set_target_properties(SuperMarioAllStars PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON C_STANDARD 17 C_EXTENSIONS ON)
# Con xsdk_add_xex tal cual, como Stick Fight, cuyo nombre sí sale en Aurora: el XDBF de xsdk (XSTC y XSTR). El de
# la tarea 0022 (XTHD y XACH de más, xenon/tools/elf2xex_spa.py, ya quitado) no cambió nada (run 023); lo más probable
# es que Aurora guardó la entrada de la carpeta antes de que el .xex tuviera nombre (tarea 0046).
xsdk_add_xex(SuperMarioAllStars TITLE_ID ${SMAS_TITLE_ID} NAME "${SMAS_TITLE_NAME}" VERSION ${SMAS_VERSION})

