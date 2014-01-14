/*! 
 * \file font_curve_analytic_nonlinear.vert.wrath-shader.glsl
 * \brief file font_curve_analytic_nonlinear.vert.wrath-shader.glsl
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

shader_out mediump vec2 wrath_CurveAnalyticBottomLeft;
shader_out mediump float wrath_CurveAnalyticGlyphIndex;

void pre_compute_glyph(in vec2 glyph_bottom_left,
                       in vec2 glyph_size,
                       in float glyph_custom_data[1])
{
  wrath_CurveAnalyticBottomLeft=glyph_bottom_left;
  wrath_CurveAnalyticGlyphIndex=glyph_custom_data[0];
}
