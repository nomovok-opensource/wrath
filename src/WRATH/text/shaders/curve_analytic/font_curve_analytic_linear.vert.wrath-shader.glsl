/*! 
 * \file font_curve_analytic_linear.vert.wrath-shader.glsl
 * \brief file font_curve_analytic_linear.vert.wrath-shader.glsl
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


shader_out mediump vec4 wrath_CurveAnalyticTexCoord_Position;
shader_out mediump float wrath_CurveAnalyticGlyphIndex;

void wrath_pre_compute_glyph(in vec2 glyph_position, 
                       in vec2 glyph_bottom_left,
                       in vec2 glyph_size,
                       in float glyph_custom_data[1])
{
  mediump vec2 pp, glyph_texture_reciprocal_size;

  glyph_texture_reciprocal_size=vec2(wrath_font_page_data(0),
                                     wrath_font_page_data(1));

  pp=glyph_bottom_left + glyph_position;
  wrath_CurveAnalyticTexCoord_Position.zw=glyph_position;
  wrath_CurveAnalyticTexCoord_Position.xy=pp*glyph_texture_reciprocal_size;
  wrath_CurveAnalyticGlyphIndex=glyph_custom_data[0];
}
