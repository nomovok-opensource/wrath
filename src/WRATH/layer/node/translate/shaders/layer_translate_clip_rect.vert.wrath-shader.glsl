/*! 
 * \file layer_translate_clip_rect.vert.wrath-shader.glsl
 * \brief file layer_translate_clip_rect.vert.wrath-shader.glsl
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



uniform mediump vec2 p;
uniform mediump vec2 q;
uniform mediump mat4 pvm;

shader_in vec2 in_normalized_pts;

void
main(void)
{
  mediump vec2 pt;

  pt= p + (q-p)*in_normalized_pts;
  gl_Position=pvm*vec4(pt, -1.0, 1.0);
  
}
