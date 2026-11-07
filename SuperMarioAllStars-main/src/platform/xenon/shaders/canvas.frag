#version 450
// video_gles.cpp's kOverlayFS with selects in place of its branches and returns (xost takes selects on data; a
// return in an arm it refuses). Every quad's corners carry the same mode and parameters, so they arrive unchanged.
// Modes: 0 solid, 1 glyph coverage (the atlas, R8), 2 rounded rectangle (uv = offset from the centre, prm = half
// width, half height, radius, stroke), 4 a picture, 5 a picture with its alpha.
// A picture is 0xAARRGGBB words, which a big-endian CPU lays out as the bytes AA, RR, GG, BB: read by byte as
// R8G8B8A8, the colour is (g, b, a) and the alpha r (screen.frag; docs/ENDIANNESS.md §3.1).
layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_col;
layout(location = 2) in vec4 v_prm;
layout(location = 3) in float v_mode;
layout(location = 0) out vec4 o_col;
layout(set = 0, binding = 0) uniform sampler2D u_atlas;
layout(set = 0, binding = 1) uniform sampler2D u_image;

void main()
{
	vec4 t = texture(u_image, v_uv);
	float glyph = texture(u_atlas, v_uv).r;
	vec2 q = abs(v_uv) - v_prm.xy + v_prm.z;
	float d = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - v_prm.z;
	float fill = clamp(0.5 - d, 0.0, 1.0);
	float ring = clamp(d + v_prm.w + 0.5, 0.0, 1.0);
	float shape = fill * (v_prm.w > 0.0 ? ring : 1.0);
	float a = v_mode > 1.5 ? shape : (v_mode > 0.5 ? glyph : 1.0);
	vec4 ui = vec4(v_col.rgb, v_col.a * a);
	vec4 picture = vec4(t.g, t.b, t.a, v_mode > 4.5 ? t.r * v_col.a : v_col.a);
	o_col = v_mode > 3.5 ? picture : ui;
}
