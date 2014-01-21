/*! 
 * \file distance_field_draw_distance_rects.frag.wrath-shader.glsl
 * \brief file distance_field_draw_distance_rects.frag.wrath-shader.glsl
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



shader_in mediump float z;
void
main(void)
{
  wrath_FragColor.rgba=vec4(z, z, z, z);
}
