/*! 
 * \file font_detailed_linear.vert.wrath-shader.glsl
 * \brief file font_detailed_linear.vert.wrath-shader.glsl
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


shader_out mediump vec4 wrath_DetailedNormalizedCoord_Position;
shader_out mediump float wrath_DetailedGlyphIndex;

void pre_compute_glyph(in vec2 glyph_position, 
		       in vec2 glyph_bottom_left,
		       in vec2 glyph_size,
		       in vec2 glyph_texture_reciprocal_size,
                       in float custom_data[1])
{
  mediump vec2 pp;

  pp=glyph_bottom_left + glyph_position;
  wrath_DetailedNormalizedCoord_Position.zw=pp;
  wrath_DetailedNormalizedCoord_Position.xy=glyph_position/glyph_size;
  wrath_DetailedGlyphIndex=custom_data[0];
}
