#version 450
// The game's picture: the PPU writes each pixel as a 32-bit 0x00RRGGBB, which a big-endian CPU lays out as the bytes
// 00, RR, GG, BB. The texture is R8G8B8A8, read by byte (vulkan4xenon: tests/test_texture.c), so its red is the 00,
// green RR, blue GG and alpha BB: the colour is (g, b, a). docs/ENDIANNESS.md §3.1; the test pattern checks it.
layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_color;
layout(set = 0, binding = 0) uniform sampler2D u_frame;

void main()
{
	vec4 t = texture(u_frame, v_uv);
	o_color = vec4(t.g, t.b, t.a, 1.0);
}
