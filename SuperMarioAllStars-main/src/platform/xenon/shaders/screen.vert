#version 450
// The game's picture on the screen (src/platform/xenon/video_vk.cpp): a unit quad (corners 0..1) scaled and moved to
// the picture's rectangle by the push constants, its texture coordinates the same corners scaled and moved to the
// part of the frame buffer shown. As vulkan4xenon's tests/shaders/push.vert does, which passed on the console.
layout(location = 0) in vec4 a_position;
layout(location = 0) out vec2 v_uv;
layout(push_constant) uniform Push { vec4 scale; vec4 offset; vec4 uv; } pc;

void main()
{
	gl_Position = a_position * pc.scale + pc.offset;
	v_uv = a_position.xy * pc.uv.xy + pc.uv.zw;
}
