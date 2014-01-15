/*! 
 * \file font_mix_linear.frag.wrath-shader.glsl
 * \brief file font_mix_linear.frag.wrath-shader.glsl
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


mediump float 
wrath_glyph_is_covered(in vec2 wrath_mix_font_glyph_position)
{
  #if defined(WRATH_DERIVATIVES_SUPPORTED)
  {
    mediump vec2 dpx, dpy;
    mediump float sc;

    dpx=dFdx(wrath_mix_font_glyph_position);
    dpy=dFdy(wrath_mix_font_glyph_position);
    sc=(dot(dpx,dpx) + dot(dpy,dpy))/2.0;

    #ifdef WRATH_GPU_CONFIG_FRAGMENT_SHADER_POOR_BRANCHING
    {
      mediump float f1, f2;

      f1=wrath_native_wrath_glyph_is_covered(wrath_mix_font_glyph_position);
      f2=wrath_minified_wrath_glyph_is_covered(wrath_mix_font_glyph_position);
      
      return (sc<wrath_mix_font_thresh_squared)?
        f1: f2;

    }
    #else
    {
      if(sc<wrath_mix_font_thresh_squared)
        {
          return wrath_native_wrath_glyph_is_covered(wrath_mix_font_glyph_position);
        }
      else
        {
          return wrath_minified_wrath_glyph_is_covered(wrath_mix_font_glyph_position);
        }
    }
    #endif 
  }
  #else
  {
    return wrath_native_wrath_glyph_is_covered(wrath_mix_font_glyph_position);
  }
  #endif 
}

mediump float
wrath_glyph_compute_coverage(in vec2 wrath_mix_font_glyph_position)
{
  #if defined(WRATH_DERIVATIVES_SUPPORTED)
  {
    mediump vec2 dpx, dpy;
    mediump float sc;

    dpx=dFdx(wrath_mix_font_glyph_position);
    dpy=dFdy(wrath_mix_font_glyph_position);
    sc=(dot(dpx,dpx) + dot(dpy,dpy))/2.0;

    #ifdef WRATH_GPU_CONFIG_FRAGMENT_SHADER_POOR_BRANCHING
    {
      mediump float f1, f2;

      f1=wrath_native_wrath_glyph_compute_coverage(wrath_mix_font_glyph_position);
      f2=wrath_minified_wrath_glyph_compute_coverage(wrath_mix_font_glyph_position);
      
      return (sc<wrath_mix_font_thresh_squared)?
        f1:f2;

    }
    #else
    {
      if(sc<wrath_mix_font_thresh_squared)
        {
          return wrath_native_wrath_glyph_compute_coverage(wrath_mix_font_glyph_position);
        }
      else
        {
          return wrath_minified_wrath_glyph_compute_coverage(wrath_mix_font_glyph_position);
        }
    }
    #endif
  }
  #else
  {
    return wrath_native_wrath_glyph_compute_coverage(wrath_mix_font_glyph_position);
  }
  #endif
    
}
