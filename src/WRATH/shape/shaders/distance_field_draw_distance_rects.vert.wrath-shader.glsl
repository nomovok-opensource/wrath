/*! 
 * \file distance_field_draw_distance_rects.vert.wrath-shader.glsl
 * \brief file distance_field_draw_distance_rects.vert.wrath-shader.glsl
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
uniform mediump float distance_sign;

shader_in mediump vec3 vert;
shader_out mediump float z;

void
main(void)
{
  /*
    normalize vert.z*distance_sign which is [-1,1] to [0,1].
   */
  z=0.5 + 0.5*distance_sign*vert.z;
  gl_Position=pvm*vec4(vert.xy, vert.z, 1.0);
}

