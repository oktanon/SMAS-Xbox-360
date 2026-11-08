// The game picture with its filters and colour settings (VideoParams): video_gles.cpp's kGameFS, written for xost,
// included by each of shaders/game/*.frag, which say which variant they are:
//   FILTER  0 Nearest, 1 Sharp Bilinear, 2 Bilinear, 3 CRT, 4 CRT with an aperture grille, 5 LCD Grid, 6 the
//           Side Borders backdrop (video_gles.cpp's pass 1: the small picture video_xenon.cpp averages on the CPU,
//           stretched, darkened and vignetted);
//   COLOR   1: the colour settings (brightness, contrast, saturation, gamma, profile, temperature, colour blindness)
//           from a table, u_lut: video_gles.cpp's applyColor is a function of the colour alone, so video_xenon.cpp
//           works it out on the CPU for 32 x 32 x 32 colours when the settings change (32 tiles of 32 x 32 side by
//           side, blue choosing the tile), and a pixel reads it twice; 0: all at their defaults, left out;
//   CURVE   1: the screen's curvature.
// One shader with them all, each chosen by an if, ran the Xbox at 30 frames a second even on Nearest (run 030): its
// registers, the most any arm needs, leave Xenos few pixels in flight to hide the texture reads. Each variant holds
// only its own work. Helpers with control flow are inlined into main (xost refuses it in a helper), ints are floats.
// The quad's uv runs 0..1 over the destination; the texture holds the picture in its top left, texSize.xy pixels of
// it (the PPU's line is wider than the source), and texSize.zw is one over the texture's own size.
// A picture is 0xAARRGGBB words, read by byte as R8G8B8A8: the colour is (g, b, a) (canvas.frag, screen.frag).
layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_col;
layout(set = 0, binding = 0) uniform sampler2D u_texL;  // the picture, linear (the canvas's atlas binding)
layout(set = 0, binding = 1) uniform sampler2D u_lut;   // the colour table (COLOR), linear
layout(push_constant) uniform Push {
	vec4 screen;   // (canvas.vert's)
	vec4 texSize;  // source w, h; 1 / texture w, 1 / texture h
	vec4 outSize;  // destination w, h, 1/w, 1/h
	vec4 f0;       // filter, scanlines, curve, pass
	vec4 f1;       // brightness, contrast, saturation, gamma
	vec4 f2;       // profile, colorblind, src w, src h
	vec4 f3;       // fade, pillarbox, temperature, 0
} pc;

vec3 rgbOf(vec4 t) { return vec3(t.g, t.b, t.a); }
vec2 texUv(vec2 texel) { return clamp(texel, vec2(0.5), pc.texSize.xy - 0.5) * pc.texSize.zw; }
vec3 texL(vec2 texel) { return rgbOf(texture(u_texL, texUv(texel))); }
vec3 texN(vec2 texel) { return texL(floor(texel) + 0.5); }  // (a texel's centre: linear filtering returns it alone)
vec3 lut(vec3 c) {
	vec3 x = clamp(c, 0.0, 1.0) * 31.0;
	float b0 = floor(x.z);
	float b1 = min(b0 + 1.0, 31.0);
	vec2 rg = (x.xy + 0.5) / vec2(1024.0, 32.0);
	vec3 c0 = rgbOf(texture(u_lut, vec2(rg.x + b0 / 32.0, rg.y)));
	vec3 c1 = rgbOf(texture(u_lut, vec2(rg.x + b1 / 32.0, rg.y)));
	return mix(c0, c1, x.z - b0);
}

vec3 texSharp(vec2 uv) {
	vec2 texel = uv * pc.texSize.xy;
	vec2 scale = max(floor(pc.outSize.xy / pc.f2.zw), vec2(1.0));
	vec2 tf = floor(texel);
	vec2 cd = fract(texel) - 0.5;
	vec2 region = 0.5 - 0.5 / scale;
	vec2 f = (cd - clamp(cd, -region, region)) * scale + 0.5;
	return texL(tf + f);
}

vec3 texRow(float x, float row) {
	float scale = max(floor(pc.outSize.x / pc.f2.z), 1.0);
	float tf = floor(x);
	float cd = fract(x) - 0.5;
	float region = 0.5 - 0.5 / scale;
	float f = (cd - clamp(cd, -region, region)) * max(scale * 0.6, 1.0) + 0.5;
	return texL(vec2(tf + f, row + 0.5));
}

vec3 lcd(vec2 uv) {
	vec2 texel = uv * pc.texSize.xy;
	vec2 f = fract(texel);
	vec3 c = texN(texel);
	vec2 scale = pc.outSize.xy / pc.f2.zw;
	vec2 edge = smoothstep(vec2(0.0), 1.5 / scale, f) * smoothstep(vec2(0.0), 1.5 / scale, 1.0 - f);
	return c * mix(1.0, edge.x * edge.y, 0.55 * pc.f0.y + 0.2);
}

void main()
{
	vec2 uv = v_uv;
	vec3 c;
	float edgeMask = 1.0;
#if FILTER == 6
	c = texL(uv * pc.texSize.xy);
#else
#if CURVE
	{
		vec2 cc = uv * 2.0 - 1.0;
		cc *= 1.0 + (cc.yx * cc.yx) * pc.f0.z * 0.12;
		uv = cc * 0.5 + 0.5;
		vec2 inside = step(vec2(0.0), uv) * step(uv, vec2(1.0));
		edgeMask = clamp((1.0 - cc.x * cc.x) * (1.0 - cc.y * cc.y) * 12.0, 0.0, 1.0) * inside.x * inside.y;
	}
#endif
#if FILTER == 0
	c = texN(uv * pc.texSize.xy);
#elif FILTER == 1
	c = texSharp(uv);
#elif FILTER == 2
	c = texL(uv * pc.texSize.xy);
#elif FILTER == 3 || FILTER == 4
	{
		vec2 texel = uv * pc.texSize.xy;
		float y = texel.y - 0.5;
		float yf = floor(y);
		float t = y - yf;
		vec3 c0 = texRow(texel.x, yf);
		vec3 c1 = texRow(texel.x, yf + 1.0);
		c0 = c0 * c0;
		c1 = c1 * c1;
		float strength = pc.f0.y;
		float k = mix(3.0, 12.0, strength);
		vec3 w0 = exp(-t * t * k / (0.6 + 0.8 * sqrt(c0)));
		vec3 w1 = exp(-(1.0 - t) * (1.0 - t) * k / (0.6 + 0.8 * sqrt(c1)));
		vec3 col = (c0 * w0 + c1 * w1) * mix(1.0, 1.55, strength);
#if FILTER == 4
		float m = mod(floor(gl_FragCoord.x), 3.0);
		vec3 mask = m < 0.5 ? vec3(1.0, 0.7, 0.7) : (m < 1.5 ? vec3(0.7, 1.0, 0.7) : vec3(0.7, 0.7, 1.0));
		col *= mix(vec3(1.0), mask * 1.25, strength);
#endif
		c = sqrt(max(col, 0.0));
	}
#else
	c = lcd(uv);
#endif
#endif

#if COLOR
	c = lut(c);
#endif

#if FILTER == 6
	float vign = 1.0 - 0.5 * pow(abs(v_uv.x * 2.0 - 1.0), 3.0);
	o_col = vec4(c * (0.32 * vign * pc.f3.x), 1.0);
#else
	o_col = vec4(c * (edgeMask * pc.f3.x), 1.0);
#endif
}
