/*! 
 * \file font_mix_linear.vert.wrath-shader.glsl
 * \brief file font_mix_linear.vert.wrath-shader.glsl
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




/*
  I feel icky repeating the glyph_position if
  the native font already did so; sighs.
 */
shader_out vec2 wrath_mix_font_glyph_position;



void wrath_pre_compute_glyph(in vec2 glyph_position, 
                       in vec2 glyph_bottom_left,
                       in vec2 glyph_size,
                       in float custom_data[WRATH_MIX_FONT_CUSTOM_DATA_SIZE])
{

  /*
    call pre_compute on both
    native and minified
   */
  #if defined(WRATH_MIX_FONT_NATIVE_CUSTOM_SIZE)
  {
    float v[WRATH_MIX_FONT_NATIVE_CUSTOM_SIZE];
    int i;

    for(i=0; i<WRATH_MIX_FONT_NATIVE_CUSTOM_SIZE; ++i)
      {
        v[i]=custom_data[i+WRATH_MIX_FONT_NATIVE_CUSTOM_OFFSET];
      }

    wrath_native_wrath_pre_compute_glyph(glyph_position,
                                   glyph_bottom_left,
                                   glyph_size,
                                   v);
  }
  #else
  {
    wrath_native_wrath_pre_compute_glyph(glyph_position,
                                   glyph_bottom_left,
                                   glyph_size);
  }
  #endif

  #if defined(WRATH_MIX_FONT_MINIFIED_CUSTOM_SIZE)
  {
    float v[WRATH_MIX_FONT_MINIFIED_CUSTOM_SIZE];
    int i;

    for(i=0; i<WRATH_MIX_FONT_MINIFIED_CUSTOM_SIZE; ++i)
      {
        v[i]=custom_data[i+WRATH_MIX_FONT_MINIFIED_CUSTOM_OFFSET];
      }

    wrath_minified_wrath_pre_compute_glyph(glyph_position*wrath_mix_font_ratio_inverse,
                                     vec2(custom_data[0], custom_data[1]),
                                     glyph_size*wrath_mix_font_ratio_inverse,
                                     v);
  }
  #else
  {
    wrath_minified_wrath_pre_compute_glyph(glyph_position*wrath_mix_font_ratio_inverse,
                                     vec2(custom_data[0], custom_data[1]),
                                     glyph_size*wrath_mix_font_ratio_inverse);
  }
  #endif

  wrath_mix_font_glyph_position=glyph_position;
}
