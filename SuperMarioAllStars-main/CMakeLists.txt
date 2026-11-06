cmake_minimum_required(VERSION 3.20)
project(SMASPort LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  add_compile_options(/W3 /O2 /Oi /GL /fp:fast /MP /utf-8 /wd4244 /wd4267 /wd4305 /wd4800 /wd4146)
  add_link_options(/LTCG)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CORE_SOURCES
  src/core/cpu65816.cpp
  src/core/apu.cpp
  src/core/dsp.cpp
  src/core/ppu.cpp
  src/core/snes.cpp
  src/core/disasm.cpp
  src/core/png.cpp
  src/core/fileio.cpp
  src/core/rewind.cpp
  src/game/achievements.cpp
  src/game/smas.cpp
  src/game/smb1.cpp
  src/game/smb1coop.cpp
  src/game/palette.cpp
  src/game/smb3.cpp
  src/game/smb3coop.cpp
  src/game/smb2.cpp
)

add_library(smascore STATIC ${CORE_SOURCES})
target_include_directories(smascore PUBLIC src)

add_executable(smastool tools/smastool.cpp)
target_link_libraries(smastool smascore)

add_executable(SuperMarioAllStars WIN32
  src/app/app.cpp
  src/app/app_win.cpp
  src/app/autotest_win.cpp
  src/app/sys_win.cpp
  src/app/config.cpp
  src/app/input.cpp
  src/app/audio.cpp
  src/app/video.cpp
  src/app/menu.cpp
  src/app/launcher.cpp
  src/app/canvas_d2d.cpp
  src/app/utf8.cpp
  src/app/achstore.cpp
  src/app/monitors.cpp
  src/app/music.cpp
  src/app/slang.cpp
  src/app/app.rc
)
target_link_libraries(SuperMarioAllStars smascore
  d3d11 dxgi d3dcompiler d2d1 dwrite xaudio2 xinput dinput8 dxguid winmm comdlg32 shell32 ole32 user32 gdi32)
set_target_properties(SuperMarioAllStars PROPERTIES OUTPUT_NAME "SuperMarioAllStars")
target_link_options(SuperMarioAllStars PRIVATE /MANIFEST:NO)
# librashader (slang shader presets), loaded at run time when present
add_custom_command(TARGET SuperMarioAllStars POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/third_party/librashader/librashader.dll $<TARGET_FILE_DIR:SuperMarioAllStars>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/third_party/fonts/LuckiestGuy-Regular.ttf $<TARGET_FILE_DIR:SuperMarioAllStars>
  # the bundled slang presets (libretro slang-shaders, with their licences) and the player README
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/third_party/slang-shaders $<TARGET_FILE_DIR:SuperMarioAllStars>/shaders
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/release/README.txt $<TARGET_FILE_DIR:SuperMarioAllStars>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/third_party/librashader/LICENSE.md $<TARGET_FILE_DIR:SuperMarioAllStars>/librashader-LICENSE.md)
