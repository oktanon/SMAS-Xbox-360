#version 450
// The game picture with its filters and colour settings (VideoParams): video_gles.cpp's kGameFS, written for xost.
// Its helpers with an if or a return are inlined into main (xost refuses control flow in a helper), its matrices
// written as dot products, its ints as floats (no integer reaches the GPU). Every if reads only push constants, so on
// Xenos the arm not taken runs no instructions (vulkan4xenon docs/SUPPORT.md, "if/else"): a filter costs only itself.
// The quad's uv runs 0..1 over the destination; the texture holds the picture in its top left, texSize.xy pixels of
// it (the PPU's line is wider than the source), and texSize.zw is one over the texture's own size.
// A picture is 0xAARRGGBB words, read by byte as R8G8B8A8: the colour is (g, b, a) (canvas.frag, screen.frag).
// Pass 1 (f0.w) is Side Borders "Blurred Game": the small backdrop video_xenon.cpp averages on the CPU, stretched,
// coloured as the game, darkened and vignetted as video_gles.cpp's pass 1.
layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 o_col;
layout(set = 0, binding = 0) uniform sampler2D u_texL;  // the picture, linear (the canvas's atlas binding)
layout(set = 0, binding = 1) uniform sampler2D u_texP;  // ... and point sampled
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
vec3 texN(vec2 texel) { return rgbOf(texture(u_texP, texUv(texel))); }
vec3 texL(vec2 texel) { return rgbOf(texture(u_texL, texUv(texel))); }

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
	if (pc.f0.w > 0.5) {
		c = texL(uv * pc.texSize.xy);
	} else {
		if (pc.f0.z > 0.0) {
			vec2 cc = uv * 2.0 - 1.0;
			cc *= 1.0 + (cc.yx * cc.yx) * pc.f0.z * 0.12;
			uv = cc * 0.5 + 0.5;
			vec2 inside = step(vec2(0.0), uv) * step(uv, vec2(1.0));
			edgeMask = clamp((1.0 - cc.x * cc.x) * (1.0 - cc.y * cc.y) * 12.0, 0.0, 1.0) * inside.x * inside.y;
		}
		float filt = pc.f0.x;
		if (filt < 0.5) {
			c = texN(uv * pc.texSize.xy);
		} else if (filt < 1.5) {
			c = texSharp(uv);
		} else if (filt < 2.5) {
			c = texL(uv * pc.texSize.xy);
		} else if (filt < 4.5) {
			// CRT (3), and with an aperture grille (4)
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
			float m = mod(floor(gl_FragCoord.x), 3.0);
			vec3 mask = m < 0.5 ? vec3(1.0, 0.7, 0.7) : (m < 1.5 ? vec3(0.7, 1.0, 0.7) : vec3(0.7, 0.7, 1.0));
			vec3 grille = mix(vec3(1.0), mask * 1.25, strength);
			col *= filt > 3.5 ? grille : vec3(1.0);
			c = sqrt(max(col, 0.0));
		} else {                  // LCD Grid
			c = lcd(uv);
		}
	}

	// the colour settings: video_gles.cpp's applyColor
	c = pow(max(c, 0.0), vec3(1.0 / pc.f1.w));
	c = (c - 0.5) * pc.f1.y + 0.5;
	c *= pc.f1.x;
	float l = dot(c, vec3(0.299, 0.587, 0.114));
	c = clamp(mix(vec3(l), c, pc.f1.z), 0.0, 1.0);
	float prof = pc.f2.x;
	if (prof > 0.5) {
		l = dot(c, vec3(0.299, 0.587, 0.114));
		if (prof < 1.5) {         // Vivid
			c = (mix(vec3(l), c, 1.3) - 0.5) * 1.08 + 0.5;
		} else if (prof < 2.5) {  // CRT Television
			c = pow(max(c, 0.0), vec3(1.15));
			c = vec3(dot(c, vec3(1.02, 0.02, -0.04)), c.g, dot(c, vec3(-0.02, 0.03, 0.95)));
		} else if (prof < 3.5) {  // Muted
			c = (mix(vec3(l), c, 0.72) - 0.5) * 0.9 + 0.52;
		} else if (prof < 4.5) {  // Greyscale
			c = vec3(l);
		} else if (prof < 5.5) {  // Sepia
			c = vec3(dot(c, vec3(0.393, 0.769, 0.189)), dot(c, vec3(0.349, 0.686, 0.168)),
			         dot(c, vec3(0.272, 0.534, 0.131)));
		} else if (prof < 6.5) {  // Pocket (Green)
			float q = floor(clamp(l, 0.0, 1.0) * 3.999);
			c = q < 1.0 ? vec3(0.06, 0.22, 0.06)
			            : (q < 2.0 ? vec3(0.19, 0.38, 0.19) : (q < 3.0 ? vec3(0.55, 0.67, 0.06) : vec3(0.61, 0.74, 0.06)));
		} else if (prof < 7.5) {  // Virtual (Red)
			c = vec3(pow(clamp(l, 0.0, 1.0), 0.8), 0.0, 0.0);
		} else if (prof < 8.5) {  // Night (Warm)
			c = c * vec3(1.0, 0.86, 0.62);
		} else {                  // Console Accurate
			c = pow(max(c, 0.0), vec3(1.18)) * 1.04;
			c = mix(vec3(l * 1.02), c, 0.92);
		}
	}
	float tmp = pc.f3.z;
	if (tmp > 0.0) {
		c *= mix(vec3(1.0), vec3(1.0, 0.9, 0.74), tmp);
	} else {
		c *= mix(vec3(1.0), vec3(0.82, 0.92, 1.0), -tmp);
	}
	float cb = pc.f2.y;
	if (cb > 0.5) {
		// daltonize: video_gles.cpp's rgb2lms and lms2rgb, each column a dot product
		vec3 lms = vec3(dot(c, vec3(17.8824, 43.5161, 4.11935)), dot(c, vec3(3.45565, 27.1554, 3.86714)),
		                dot(c, vec3(0.0299566, 0.184309, 1.46709)));
		vec3 s1 = vec3(2.02344 * lms.y - 2.52581 * lms.z, lms.y, lms.z);
		vec3 s2 = vec3(lms.x, 0.494207 * lms.x + 1.24827 * lms.z, lms.z);
		vec3 s3 = vec3(lms.x, lms.y, -0.395913 * lms.x + 0.801109 * lms.y);
		vec3 s = cb < 1.5 ? s1 : (cb < 2.5 ? s2 : s3);
		vec3 sim = vec3(dot(s, vec3(0.0809444479, -0.130504409, 0.116721066)),
		                dot(s, vec3(-0.0102485335, 0.0540193266, -0.113614708)),
		                dot(s, vec3(-0.000365296938, -0.00412161469, 0.693511405)));
		vec3 err = c - sim;
		c = clamp(c + vec3(0.0, err.r * 0.7 + err.g, err.r * 0.7 + err.b), 0.0, 1.0);
	}
	c = clamp(c, 0.0, 1.0);

	float vign = 1.0 - 0.5 * pow(abs(v_uv.x * 2.0 - 1.0), 3.0);
	float k = pc.f0.w > 0.5 ? 0.32 * vign : edgeMask;
	o_col = vec4(c * (k * pc.f3.x), 1.0);
}
