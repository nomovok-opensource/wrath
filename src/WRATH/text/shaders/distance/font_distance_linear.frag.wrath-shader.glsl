/*! 
 * \file font_distance_linear.frag.wrath-shader.glsl
 * \brief file font_distance_linear.frag.wrath-shader.glsl
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






shader_in mediump vec2 wrath_DistanceFieldTexCoord;
shader_in mediump vec2 wrath_DistanceFieldPosition;
uniform mediump sampler2D wrath_DistanceField;

mediump float 
wrath_glyph_is_covered(void)
{
  mediump float rr;

  rr=texture2D(wrath_DistanceField, wrath_DistanceFieldTexCoord).r;
  return step(0.5, rr);
}

mediump float
wrath_glyph_compute_coverage(void)
{
  mediump float rr;

  rr=texture2D(wrath_DistanceField, wrath_DistanceFieldTexCoord).r;

  #if defined(WRATH_DERIVATIVES_SUPPORTED)
  {
    mediump vec2 dx, dy;
    mediump float scr;
    
    dx=dFdx(wrath_DistanceFieldPosition);
    dy=dFdy(wrath_DistanceFieldPosition);
    scr=sqrt( (dot(dx,dx) + dot(dy,dy))/2.0 );
  
    return smoothstep(0.5 - 0.2*scr,
                      0.5 + 0.2*scr,
                      rr);
  }
  #else
  {
    return step(0.5, rr);
  }
  #endif

}

















