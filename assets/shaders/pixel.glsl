#ifdef GL_ES
precision lowp float;
#endif
uniform sampler2D al_tex;
uniform bool al_use_tex;
uniform bool al_alpha_test;
uniform int al_alpha_func;
uniform float al_alpha_test_val;
varying vec4 varying_color;
varying vec2 varying_texcoord;

uniform float time;
varying float varying_time;

uniform float audio_rms;
uniform float audio_peak;
uniform float audio_transient;

bool alpha_test_func(float x, int op, float compare);

vec3 palette(float t);

void main()
{
  vec4 c;
  c = varying_color;

  vec2 uv = varying_texcoord;
  vec2 center = uv - vec2(0.5, 0.5);
  float radius = length(center);

  float pulse = smoothstep(0.05, 0.85, audio_peak);
  float energy = smoothstep(0.02, 0.45, audio_rms);
  float hit = smoothstep(0.02, 0.35, audio_transient);

  float ripple = sin(radius * 28.0 - time * (2.0 + pulse * 4.0) + energy * 10.0);
  float distortion = ripple * (0.006 + pulse * 0.020 + hit * 0.025);
  vec2 direction = center;
  float directionLength = length(direction);
  if (directionLength > 0.0001) {
    direction /= directionLength;
  } else {
    direction = vec2(0.0, 0.0);
  }
  uv += direction * distortion;

  if (al_use_tex) {
    c *= texture2D(al_tex, uv);
  }

  float scan = 0.96 + 0.04 * sin((uv.y * 640.0) + time * 16.0);
  float vignette = 0.72 + 0.28 * (1.0 - smoothstep(0.20, 0.90, radius + pulse * 0.05));
  float halo = 1.0 - smoothstep(0.0, 0.42, abs(radius - (0.24 + energy * 0.18 + pulse * 0.10)));

  vec3 tint = palette(time * 0.12 + pulse * 0.75 + energy * 0.35);
  vec3 color = c.rgb;
  color = mix(color, tint, 0.18 + energy * 0.10);
  color *= scan;
  color = mix(color, color * tint, clamp(halo + hit * 0.35, 0.0, 1.0));
  color += tint * (halo * 0.35 + hit * 0.10);
  color += vec3(0.12, 0.14, 0.18) + energy * vec3(0.08, 0.10, 0.12);
  color *= vignette;

  if (!al_alpha_test || alpha_test_func(c.a, al_alpha_func, al_alpha_test_val))
    gl_FragColor = vec4(clamp(color, 0.0, 1.0), c.a);
  else
    discard;
}

bool alpha_test_func(float x, int op, float compare)
{
  if (op == 0) return false;
  else if (op == 1) return true;
  else if (op == 2) return x < compare;
  else if (op == 3) return x == compare;
  else if (op == 4) return x <= compare;
  else if (op == 5) return x > compare;
  else if (op == 6) return x != compare;
  else if (op == 7) return x >= compare;
  return false;
}

vec3 hueRotate(vec3 rgb, float angle)
{
  float s = sin(angle);
  float c = cos(angle);
  
  // RGB to YIQ transformation matrix
  mat3 rgbToYiq = mat3(
    0.299,  0.587,  0.114,
    0.596, -0.274, -0.322,
    0.211, -0.523,  0.312
  );
  
  // YIQ to RGB transformation matrix
  mat3 yiqToRgb = mat3(
    1.0,  0.956,  0.621,
    1.0, -0.272, -0.647,
    1.0, -1.106,  1.703
  );
  
  vec3 yiq = rgbToYiq * rgb;
  
  // Rotate chroma (I and Q components)
  mat2 chromaRotation = mat2(c, -s, s, c);
  yiq.yz = chromaRotation * yiq.yz;
  
  // Convert back to RGB and clamp
  vec3 rotated = clamp(yiqToRgb * yiq, 0.0, 1.0);
  return rotated;
}

vec3 palette(float t)
{
  vec3 a = vec3(0.10, 0.18, 0.32);
  vec3 b = vec3(0.22, 0.75, 0.95);
  vec3 c = vec3(0.92, 0.32, 0.76);
  float mixFactor = 0.5 + 0.5 * sin(t);
  return mix(a, mix(b, c, mixFactor), 0.65);
}