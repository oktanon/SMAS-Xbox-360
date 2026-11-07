// The launcher's and menus' fonts, built into the .xex (video_xenon.cpp reads them with stb_truetype): Luckiest Guy
// (the titles; third_party/fonts/LICENSE.txt, Apache 2.0) and DejaVu Sans and Sans Bold (the rest; the Linux build
// takes them from the system; third_party/fonts/DejaVu-LICENSE.txt). SMAS_FONT_DIR: third_party/fonts, from CMake.
#ifndef SMAS_FONT_DIR
#error "SMAS_FONT_DIR: the folder of the fonts (xenon/CMakeLists.txt)"
#endif

#define SMAS_FONT(name, file)                          \
  ".section .rodata\n"                                 \
  ".balign 4\n"                                        \
  ".globl " name "\n" name ":\n"                       \
  ".incbin \"" SMAS_FONT_DIR "/" file "\"\n"           \
  ".globl " name "_end\n" name "_end:\n"               \
  ".byte 0\n"                                          \
  ".previous\n"

__asm__(SMAS_FONT("smas_font_title", "LuckiestGuy-Regular.ttf"));
__asm__(SMAS_FONT("smas_font_sans", "DejaVuSans.ttf"));
__asm__(SMAS_FONT("smas_font_bold", "DejaVuSans-Bold.ttf"));
