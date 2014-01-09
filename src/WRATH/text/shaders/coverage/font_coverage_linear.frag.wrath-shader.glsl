/*! 
 * \file font_coverage_linear.frag.wrath-shader.glsl
 * \brief file font_coverage_linear.frag.wrath-shader.glsl
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






shader_in mediump vec2 wrath_CoverageFieldTexCoord;
shader_in mediump vec2 wrath_CoverageFieldPosition;
uniform mediump sampler2D wrath_CoverageField;

mediump float 
is_covered(void)
{
  mediump float rr;

  rr=texture2D(wrath_CoverageField, wrath_CoverageFieldTexCoord).r;
  return step(0.5, rr);
}

mediump float
compute_coverage(void)
{
  mediump float rr;

  rr=texture2D(wrath_CoverageField, wrath_CoverageFieldTexCoord).r;
  return rr;
}

















