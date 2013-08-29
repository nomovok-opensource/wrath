/*! 
 * \file font_common_base.frag.wrath-shader.glsl
 * \brief file font_common_base.frag.wrath-shader.glsl
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





#ifndef TEX_ATTRIBUTE_TYPE
#define TEX_ATTRIBUTE_TYPE highp
#endif

#ifndef TEX_VARYING_TYPE
#define TEX_VARYING_TYPE mediump
#endif

#ifndef TEX_RECIP_TYPE
#define TEX_RECIP_TYPE highp
#endif






/*
  This is.. insane. Mix font shaders include 2
  fragment sources: one from the native font and
  one from the minified font. Each of those
  when used to create a WRATHShaderSpecifier
  will include this file. So what we do is
  that on the 2nd time the file is included,
  we change the meaning of some of the macros!
 */
#ifndef FONT_COMMON_BASE_INCLUDED
  #define FONT_COMMON_BASE_INCLUDED

  /*
    PrimaryGlyphCoordinate is used to compute
    scaling factor within fragment shader.
   */
  #define PrimaryGlyphCoordinate relative_coord.xy

  shader_in mediump float GlyphIndex;
  shader_in TEX_VARYING_TYPE vec2 GlyphNormalizedCoordinate;


  #if defined(MIX_FONT_SHADER)
    shader_in TEX_VARYING_TYPE vec4 tex_coord;
    shader_in TEX_VARYING_TYPE vec4 relative_coord;
  #else
    shader_in TEX_VARYING_TYPE vec2 tex_coord;
    shader_in TEX_VARYING_TYPE vec2 relative_coord;
  #endif

  /*
    macros, GlyphTextureCoordinate and GlyphCoordinate
    will be redefined by mix font shaders. as they will include
    other _base.frag.wrath-shader.glsl files. However, for all but mix shaders,
    the GlyphTextureCoordinate GlyphCoordinate are taken directly
    from tex_coord and relative_coord.
  */
  #define GlyphTextureCoordinate tex_coord.xy
  #define GlyphCoordinate relative_coord.xy

#else

  #undef GlyphTextureCoordinate
  #define GlyphTextureCoordinate tex_coord.zw

  #undef GlyphCoordinate
  #define GlyphCoordinate relative_coord.zw

#endif
