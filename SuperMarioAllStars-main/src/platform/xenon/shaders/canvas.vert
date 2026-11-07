#version 450
// The UI and the game picture as batched quads (src/platform/xenon/video_xenon.cpp): video_gles.cpp's kOverlayVS.
// Positions in pixels from the top left; Vulkan's clip space has y down, so no flip.
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_col;   // R8G8B8A8_UNORM: the bytes r, g, b, a by address
layout(location = 3) in vec4 a_prm;
layout(location = 4) in float a_mode;
layout(push_constant) uniform Push { vec4 screen; } pc;  // 2/W, 2/H
layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_col;
layout(location = 2) out vec4 v_prm;
layout(location = 3) out float v_mode;

void main()
{
	gl_Position = vec4(a_pos.x * pc.screen.x - 1.0, a_pos.y * pc.screen.y - 1.0, 0.0, 1.0);
	v_uv = a_uv;
	v_col = a_col;
	v_prm = a_prm;
	v_mode = a_mode;
}
