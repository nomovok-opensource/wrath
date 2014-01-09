/*! 
 * \file font_coverage_base.frag.wrath-shader.glsl
 * \brief file font_coverage_base.frag.wrath-shader.glsl
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







uniform mediump sampler2D wrath_CoverageField;
shader_in mediump vec2 wrath_CoverageFieldBottomLeft;

mediump float 
is_covered(in vec2 glyph_position, in vec2 glyph_recirpocal_size)
{
  mediump float rr;
  mediump vec2 tt;

  tt=(glyph_position + wrath_CoverageFieldBottomLeft)*glyph_recirpocal_size;
  rr=texture2D(wrath_CoverageField, tt).r;
  return step(0.5, rr);
}

mediump float
compute_coverage(in vec2 glyph_position, in vec2 glyph_recirpocal_size)
{
  mediump float rr;
  mediump vec2 tt;

  tt=(glyph_position + wrath_CoverageFieldBottomLeft)*glyph_recirpocal_size;
  rr=texture2D(wrath_CoverageField, tt).r;
  return rr;

}

















