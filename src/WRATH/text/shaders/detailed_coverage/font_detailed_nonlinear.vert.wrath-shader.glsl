/*! 
 * \file font_detailed_nonlinear.vert.wrath-shader.glsl
 * \brief file font_detailed_nonlinear.vert.wrath-shader.glsl
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

shader_out mediump vec3 wrath_DetailedGlyphRecipSize_GlyphIndex;
void pre_compute_glyph(in vec2 glyph_position, 
                       in vec2 glyph_bottom_left,
                       in vec2 glyph_size,
                       in vec2 glyph_texture_reciprocal_size,
                       in float custom_data[1])
{
  wrath_DetailedGlyphRecipSize_GlyphIndex.xy=vec2(1.0f, 1.0f)/glyph_size;
  wrath_DetailedGlyphRecipSize_GlyphIndex.z=custom_data[0];
}
