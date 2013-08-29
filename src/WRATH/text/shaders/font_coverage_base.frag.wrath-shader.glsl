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



uniform mediump sampler2D CoverageTexture;

mediump float
compute_coverage(void)
{
  mediump float v;

  v=texture2D(CoverageTexture, GlyphTextureCoordinate).r;

  return v;
}

mediump float 
is_covered(void)
{
  return step(0.5, compute_coverage());
}
