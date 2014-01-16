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
#define WRATH_FONT_IMPLEMENT_SIGNED_DISTANCE


uniform mediump sampler2D wrath_AnalyticNormalTexture;
uniform mediump sampler2D wrath_AnalyticPositionTexture;



#ifdef WRATH_FONT_ANALYTIC_PIXEL_RELATIVE_COORDINATES

#ifndef WRATH_FONT_ANALYTIC_MAX_GLYPH_NORMALIZED_SIZE 
#define WRATH_FONT_ANALYTIC_MAX_GLYPH_NORMALIZED_SIZE 1.0
#endif

#endif


/*
  tex_v=texture2D(texture_unit0, tex_coord);
  tex_o=texture2D(texture_unit1, tex_coord);
  prelative_coord= relative_coord attribute
 */
mediump float
wrath_analytic_font_compute_distance(in vec2 GlyphTextureCoordinate,
                                     in vec2 GlyphCoordinate)
{
  mediump vec4 tex_v, X_Y;  
  mediump vec2 p, Cv, dL_dM;
  mediump float d;

#ifdef WRATH_FONT_ANALYTIC_PIXEL_RELATIVE_COORDINATES
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

  tex_v=texture2D(wrath_AnalyticNormalTexture, GlyphTextureCoordinate);

  #ifdef WRATH_FONT_ANALYTIC_PIXEL_RELATIVE_COORDINATES
    tex_o=texture2D(wrath_AnalyticPositionTexture, GlyphTextureCoordinate);
  #elif defined(WRATH_FONT_USE_LA_LOOKUP)
    tex_o=texture2D(wrath_AnalyticPositionTexture, GlyphTextureCoordinate).ra;
  #else
    tex_o=texture2D(wrath_AnalyticPositionTexture, GlyphTextureCoordinate).rg;
  #endif


#ifdef WRATH_FONT_ANALYTIC_PIXEL_RELATIVE_COORDINATES
  p=GlyphCoordinate - 255.0*tex_o.zw;
  p=clamp(p, 0.0, WRATH_FONT_ANALYTIC_MAX_GLYPH_NORMALIZED_SIZE);
#else
  p=GlyphCoordinate;
#endif

  X_Y=(2.0)*(255.0/254.0)*tex_v.xzyw - vec4(1.0, 1.0, 1.0, 1.0);
  
  Cv= Lv.xy*Mv.yx;

#ifdef WRATH_FONT_ANALYTIC_PIXEL_RELATIVE_COORDINATES
  offsetL_M=(4.0*WRATH_FONT_ANALYTIC_MAX_GLYPH_NORMALIZED_SIZE)*(255.0/254.0)*tex_o.xy 
    - vec2(2.0*WRATH_FONT_ANALYTIC_MAX_GLYPH_NORMALIZED_SIZE, 2.0*WRATH_FONT_ANALYTIC_MAX_GLYPH_NORMALIZED_SIZE);
#endif

  dL_dM=p.x*X + p.y*Y - offsetL_M;
  d=(Cv.y<Cv.x)?
    max(dL, dM):
    min(dL, dM);

  return d;

#ifndef WRATH_FONT_ANALYTIC_PIXEL_RELATIVE_COORDINATES
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

