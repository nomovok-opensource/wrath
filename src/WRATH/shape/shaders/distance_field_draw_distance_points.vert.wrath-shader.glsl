/*! 
 * \file distance_field_draw_distance_points.vert.wrath-shader.glsl
 * \brief file distance_field_draw_distance_points.vert.wrath-shader.glsl
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



uniform mediump mat4 pvm;
shader_in mediump vec4 pos;
shader_out mediump vec2 st;

void
main(void)
{
  st=pos.zw;
  gl_Position=pvm*vec4(pos.xy, 0.0, 1.0);
}

               
