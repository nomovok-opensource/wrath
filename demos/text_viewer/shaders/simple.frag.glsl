/*! 
 * \file simple.frag.glsl
 * \brief file simple.frag.glsl
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


shader_in mediump vec2 tex_coord;
shader_in mediump vec4 tex_color;

uniform mediump sampler2D utex;
uniform mediump float animation_fx_interpol;
void
shader_main(void)
{
  mediump vec4 c;

  c=texture2D(utex, tex_coord).rgba;
  gl_FragColor=tex_color.rgba*vec4(c.rgb, c.a*(1.0-2.0*animation_fx_interpol));
}
