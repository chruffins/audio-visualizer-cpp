#ifdef GL_ES
precision mediump float;
#endif

uniform bool al_alpha_test;
uniform int al_alpha_func;
uniform float al_alpha_test_val;

uniform vec3 tint;
uniform float time;

varying vec4 varying_color;

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
   vec3 base = varying_color.rgb * tint;

   // Screen-space phase keeps the effect stable on primitive rectangles.
   float phase = (gl_FragCoord.x + gl_FragCoord.y) * 0.02 + (time * 0.8);
   float angle = 6.2831853 * fract(phase);
   float s = sin(angle);
   float c = cos(angle);

   mat3 rgbToYiq = mat3(
      0.299,  0.587,  0.114,
      0.596, -0.274, -0.322,
      0.211, -0.523,  0.312
   );

   mat3 yiqToRgb = mat3(
      1.0,  0.956,  0.621,
      1.0, -0.272, -0.647,
      1.0, -1.106,  1.703
   );

   vec3 yiq = rgbToYiq * base;
   mat2 chromaRotation = mat2(c, -s, s, c);
   yiq.yz = chromaRotation * yiq.yz;

   vec3 rotated = clamp(yiqToRgb * yiq, 0.0, 1.0);
   float shimmer = 0.92 + 0.08 * sin(phase * 8.0);
   vec4 outColor = vec4(rotated * shimmer, varying_color.a);

   if (!al_alpha_test || alpha_test_func(outColor.a, al_alpha_func, al_alpha_test_val))
      gl_FragColor = outColor;
   else
      discard;
}