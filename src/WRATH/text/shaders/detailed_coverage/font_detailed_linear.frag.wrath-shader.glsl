
/*! 
 * \file font_detailed_linear.frag.wrath-shader.glsl
 * \brief file font_detailed_linear.frag.wrath-shader.glsl
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


shader_in mediump vec4 wrath_DetailedNormalizedCoord_Position;
shader_in mediump float wrath_DetailedGlyphIndex;

mediump float 
wrath_glyph_is_covered(void)
{
  return wrath_detailed_wrath_glyph_is_covered(wrath_DetailedNormalizedCoord_Position.zw,
                                   wrath_DetailedNormalizedCoord_Position.xy,
                                   wrath_DetailedGlyphIndex);
}

mediump float
wrath_glyph_compute_coverage(void)
{
  return wrath_detailed_wrath_glyph_compute_coverage(wrath_DetailedNormalizedCoord_Position.zw,
                                         wrath_DetailedNormalizedCoord_Position.xy,
                                         wrath_DetailedGlyphIndex);
}
