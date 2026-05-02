#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D al_tex;
uniform sampler2D history_tex;
uniform bool al_use_tex;
uniform bool al_alpha_test;
uniform int al_alpha_func;
uniform float al_alpha_test_val;

uniform vec2 echo_center;
uniform vec2 texel_size;
uniform float echo_decay;
uniform float echo_speed;
uniform float audio_peak;
uniform float audio_transient;

varying vec4 varying_color;
varying vec2 varying_texcoord;

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

void main()
{
  vec2 uv = varying_texcoord;
  vec2 delta = uv - echo_center;
  float dist = length(delta);
  vec2 dir = (dist > 0.0001) ? (delta / dist) : vec2(0.0, 0.0);

  float burst = smoothstep(0.02, 0.60, audio_peak) + smoothstep(0.01, 0.30, audio_transient) * 0.75;
  float speed = echo_speed * (1.0 + burst * 1.6);
  vec2 historyUv = uv - dir * speed;

  vec4 current = varying_color;
  if (al_use_tex) {
    current *= texture2D(al_tex, uv);
  }

  vec4 historyCore = texture2D(history_tex, historyUv);
  vec4 history = historyCore * 0.55;
  history += texture2D(history_tex, historyUv + vec2(texel_size.x, 0.0)) * 0.1125;
  history += texture2D(history_tex, historyUv - vec2(texel_size.x, 0.0)) * 0.1125;
  history += texture2D(history_tex, historyUv + vec2(0.0, texel_size.y)) * 0.1125;
  history += texture2D(history_tex, historyUv - vec2(0.0, texel_size.y)) * 0.1125;
  float radiusFade = 1.0 - smoothstep(0.0, 1.1, dist);

  vec3 echoColor = history.rgb;
  echoColor *= mix(vec3(1.0), vec3(0.82, 0.82, 0.82), radiusFade * 0.35);

  vec4 outColor;
  outColor.rgb = clamp(current.rgb + (echoColor * echo_decay), 0.0, 1.0);
  outColor.a = clamp(max(current.a, history.a * echo_decay), 0.0, 1.0);

  if (!al_alpha_test || alpha_test_func(outColor.a, al_alpha_func, al_alpha_test_val))
    gl_FragColor = outColor;
  else
    discard;
}
