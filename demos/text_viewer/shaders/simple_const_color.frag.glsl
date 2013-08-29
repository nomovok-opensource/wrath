/*! 
 * \file simple_const_color.frag.glsl
 * \brief file simple_const_color.frag.glsl
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
uniform mediump float animation_fx_interpol;

void
shader_main(void)
{
  gl_FragColor=vec4(tex_color.rgb, tex_color.a*(1.0-2.0*animation_fx_interpol));
}
