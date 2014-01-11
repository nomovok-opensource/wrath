/*! 
 * \file font_mix_base.frag.wrath-shader.glsl
 * \brief file font_mix_base.frag.wrath-shader.glsl
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
is_covered(void)
{
  #if defined(WRATH_DERIVATIVES_SUPPORTED)
  {
    mediump vec2 dpx, dpy;
    mediump float sc;

    dpx=dFdx(PrimaryGlyphCoordinate);
    dpy=dFdy(PrimaryGlyphCoordinate);
    sc=(dot(dpx,dpx) + dot(dpy,dpy))/2.0;

    #ifdef WRATH_GPU_CONFIG_FRAGMENT_SHADER_POOR_BRANCHING
    {
      mediump float f1, f2;

      f1=native_is_covered();
      f2=minified_is_covered();
      
      return (sc<MIX_FONT_SHADER)?
        f1:f2;

    }
    #else
    {
      if(sc<MIX_FONT_SHADER*MIX_FONT_SHADER)
        {
          return native_is_covered();
        }
      else
        {
          return minified_is_covered();
        }
    }
    #endif 
  }
  #else
  {
    return native_is_covered();
  }
  #endif 
}

mediump float
compute_coverage(void)
{
  #if defined(WRATH_DERIVATIVES_SUPPORTED)
  {
    mediump vec2 dpx, dpy;
    mediump float sc;

    dpx=dFdx(PrimaryGlyphCoordinate);
    dpy=dFdy(PrimaryGlyphCoordinate);
    sc=sqrt( (dot(dpx,dpx) + dot(dpy,dpy))/2.0 );

    #ifdef WRATH_GPU_CONFIG_FRAGMENT_SHADER_POOR_BRANCHING
    {
      mediump float f1, f2;

      f1=native_compute_coverage();
      f2=minified_compute_coverage();
      
      return (sc<MIX_FONT_SHADER)?
        f1:f2;

    }
    #else
    {
      if(sc<MIX_FONT_SHADER)
        {
          return native_compute_coverage();
        }
      else
        {
          return minified_compute_coverage();
        }
    }
    #endif
  }
  #else
  {
    return native_is_covered();
  }
  #endif
    
}
