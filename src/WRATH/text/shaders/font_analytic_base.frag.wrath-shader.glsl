/*! 
 * \file font_analytic_base.frag.wrath-shader.glsl
 * \brief file font_analytic_base.frag.wrath-shader.glsl
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



uniform mediump sampler2D NormalTexture;
uniform mediump sampler2D PositionTexture;



#ifdef PIXEL_RELATIVE_COORDINATES

#ifndef MAX_GLYPH_NORMALIZED_SIZE 
#define MAX_GLYPH_NORMALIZED_SIZE 1.0
#endif

#endif


/*
  needed if this font shader is used twice for a mix
  font with the saem type to make sure that each
  notion of size, the function compute_distance
  is unique
 */
#ifndef ANALYTIC_PRESENT
#define ANALYTIC_PRESENT
#else
#define compute_distance compute_distance_again
#endif


/*
  tex_v=texture2D(texture_unit0, tex_coord);
  tex_o=texture2D(texture_unit1, tex_coord);
  prelative_coord= relative_coord attribute
 */
mediump float
compute_distance(void)
{
  mediump vec4 tex_v, X_Y;  
  mediump vec2 p, Cv, dL_dM;
  mediump float d;

#ifdef PIXEL_RELATIVE_COORDINATES
  mediump vec2 offsetL_M;
  mediump vec4 tex_o;
#else
  mediump vec2 tex_o;
  #define offsetL_M tex_o
#endif

#define X X_Y.xy
#define Y X_Y.zw
#define Lv X_Y.xz
#define Mv X_Y.yw
#define offsetL offsetL_M.x
#define offsetM offsetL_M.y
#define dL dL_dM.x
#define dM dL_dM.y

  tex_v=texture2D(NormalTexture, GlyphTextureCoordinate);

  #ifdef PIXEL_RELATIVE_COORDINATES
    tex_o=texture2D(PositionTexture, GlyphTextureCoordinate);
  #elif defined(USE_LA_LOOKUP)
    tex_o=texture2D(PositionTexture, GlyphTextureCoordinate).ra;
  #else
    tex_o=texture2D(PositionTexture, GlyphTextureCoordinate).rg;
  #endif


#ifdef PIXEL_RELATIVE_COORDINATES
  p=GlyphCoordinate - 255.0*tex_o.zw;
  p=clamp(p, 0.0, MAX_GLYPH_NORMALIZED_SIZE);
  //p=max( vec2(0.0,0.0), min(vec2(MAX_GLYPH_NORMALIZED_SIZE, MAX_GLYPH_NORMALIZED_SIZE), p));
#else
  p=GlyphCoordinate;
#endif

  X_Y=(2.0)*(255.0/254.0)*tex_v.xzyw - vec4(1.0, 1.0, 1.0, 1.0);
  
  Cv= Lv.xy*Mv.yx;

#ifdef PIXEL_RELATIVE_COORDINATES
  offsetL_M=(4.0*MAX_GLYPH_NORMALIZED_SIZE)*(255.0/254.0)*tex_o.xy - vec2(2.0*MAX_GLYPH_NORMALIZED_SIZE, 2.0*MAX_GLYPH_NORMALIZED_SIZE);
#endif

  dL_dM=p.x*X + p.y*Y - offsetL_M;
  d=(Cv.y<Cv.x)?
    max(dL, dM):
    min(dL, dM);

  return d;

#ifndef PIXEL_RELATIVE_COORDINATES
  #undef offsetL_M 
#endif

#undef X 
#undef Y 
#undef Lv 
#undef Mv 
#undef offsetL 
#undef offsetM 
#undef dL 
#undef dM 


}

mediump float
compute_coverage(void)
{
  mediump float d;

  d=compute_distance();
  
  #if defined(WRATH_DERIVATIVES_SUPPORTED)
  {
    mediump vec2 dx, dy;
    mediump float sd, a;

    dx=dFdx(GlyphCoordinate);
    dy=dFdy(GlyphCoordinate);
    sd=max(1.0, inversesqrt( (dot(dx,dx) + dot(dy,dy))/2.0 ) );
    a= 0.5 + d*sd;
    return clamp(a, 0.0, 1.0);
  }
  #else
  {
    return step(0.0, d);
  }
  #endif
  
}

mediump float 
is_covered(void)
{
  mediump float d;

  d=compute_distance();
  return step(0.0, d);
}


