#ifdef GL_ES
precision lowp float;
#endif

uniform bool al_alpha_test;
uniform int al_alpha_func;
uniform float al_alpha_test_val;

varying vec4 varying_color;

uniform float time;

uniform vec2 vis_center;
uniform float vis_radius;

uniform float audio_rms;
uniform float audio_peak;
uniform float audio_transient;

bool alpha_test_func(float x, int op, float compare);

void main()
{
  vec2 p = gl_FragCoord.xy - vis_center;
  float dist = length(p);
  float angle = atan(p.y, p.x);

  float pulse = smoothstep(0.02, 0.72, audio_peak);
  float energy = smoothstep(0.01, 0.30, audio_rms);
  float hit = smoothstep(0.008, 0.22, audio_transient);

  float ringDelta = abs(dist - vis_radius);
  float innerBand = exp(-pow(ringDelta / (2.8 + energy * 8.5), 2.0));
  float outerGlow = exp(-ringDelta / (20.0 + energy * 34.0));
  float angularSweep = 0.5 + 0.5 * sin((angle * 8.0) - (time * (2.8 + pulse * 6.8)) + energy * 6.5);
  float spark = pow(max(0.0, sin((angle * 14.0) + (time * 11.0))), 9.0) * hit;

  vec3 base = varying_color.rgb;
  vec3 cool = vec3(0.08, 0.55, 1.0);
  vec3 hot = vec3(0.85, 0.20, 1.0);
  vec3 plasma = mix(cool, hot, 0.25 + 0.75 * pulse);

  vec3 color = base * (0.24 + 0.55 * energy);
  color += plasma * (innerBand * (0.85 + 1.10 * angularSweep));
  color += vec3(0.70, 0.94, 1.0) * outerGlow * (0.38 + 0.72 * pulse);
  color += vec3(1.0, 0.98, 0.85) * spark * (0.95 + 1.25 * pulse);
  color = clamp(color, 0.0, 1.0);

  float alphaBoost = clamp(innerBand * 1.05 + outerGlow * 0.55 + spark * 1.15, 0.38, 1.0);
  vec4 outColor = vec4(color, varying_color.a * alphaBoost);

  if (!al_alpha_test || alpha_test_func(outColor.a, al_alpha_func, al_alpha_test_val))
    gl_FragColor = outColor;
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