/*! 
 * \file distance_image.frag.glsl
 * \brief file distance_image.frag.glsl
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


uniform mediump sampler2D utex;
shader_in mediump vec2 tex_coord;
shader_in mediump vec4 tex_color;
shader_in mediump float reciprocal_scale_factor;

uniform mediump float animation_fx_interpol;
void
shader_main(void)
{
  mediump float c, alpha;
  mediump float vv;

  c=texture2D(utex, tex_coord).a;
  vv=0.05*min(1.0, reciprocal_scale_factor);

  alpha=tex_color.a*smoothstep(0.50-vv, 0.50+vv, c);
  gl_FragColor=vec4(tex_color.rgb, alpha*(1.0-2.0*animation_fx_interpol));
}
