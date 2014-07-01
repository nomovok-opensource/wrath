/*! 
 * \file font_animated.frag.glsl
 * \brief file font_animated.frag.glsl
 * 
 * Copyright 2013 by Nomovok Ltd.
 * 
 * Contact: info@nomovok.com
 * 
 * This Source Code Form is subject to the
 * terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with
 * this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 * 
 * \author Kevin Rogovin <kevin.rogovin@nomovok.com>
 * 
 */


shader_in mediump vec4 tex_color;
shader_in mediump vec2 giggles;

uniform mediump float animation_fx_interpol;
void
shader_main(void)
{
  mediump float d, f, a, b, ca, cb, caa, cbb, q;
  mediump vec2 ff;
  int II;

  /*
#ifdef NO_AA
  d=wrath_glyph_is_covered()*tex_color.a*(1.0-animation_fx_interpol);
#else
  d=wrath_glyph_compute_coverage()*tex_color.a*(1.0-animation_fx_interpol);
#endif
  */
  d=wrath_glyph_is_covered();

  /*
#if defined(WRATH_IS_OPAQUE_PASS)
  //if texel is too translucent, then the opaque pass discards the texel
  if(d<float(WRATH_TRANSLUCENT_THRESHOLD))
    discard;
#elif defined(WRATH_IS_TRANSLUCENT_PASS)
  if(d>=float(WRATH_TRANSLUCENT_THRESHOLD))
    d=0.0;
#endif
  */

  ff=fract(giggles);
  ff=step(ff, vec2(0.005)) + step(vec2(0.995), ff);
  f=max(ff.x, ff.y);
  a=step(length(wrath_curve_analytic_pa_pb.xy), 0.05);
  b=step(length(wrath_curve_analytic_pa_pb.zw), 0.05);
  ca=step(abs(wrath_curve_analytic_sigma_ab.x), 0.1);
  cb=step(abs(wrath_curve_analytic_sigma_ab.y), 0.1);

  II=int(wrath_curve_analytic_idx);
  
  if(II%2==0)
    {
      q=1.0;
    }
  else
    {
      q=0.0;
    }
  caa=mix(ca, cb, q);
  cbb=mix(cb, ca, q);

  wrath_FragColor=vec4(d*0.25 + f*0.25 + a*0.5, caa*0.5, cbb*0.5, 1.0);

}
