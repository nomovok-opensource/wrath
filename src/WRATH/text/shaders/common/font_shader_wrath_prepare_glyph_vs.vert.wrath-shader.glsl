/*! 
 * \file font_shader_wrath_prepare_glyph_vs.vert.wrath-shader.glsl
 * \brief file font_shader_wrath_prepare_glyph_vs.vert.wrath-shader.glsl
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



void
wrath_font_prepare_glyph_vs(
#ifdef WRATH_TEXTURE_FONT_LINEAR
                            in vec2 glyph_position,
#endif
                            in vec2 glyph_bottom_left,
                            in vec2 glyph_size)
{
  #if defined(WRATH_FONT_CUSTOM_DATA)
  {
    wrath_font_custom_data_t custom_values_str;
  
    wrath_font_shader_custom_data_func(custom_values_str);
    pre_compute_glyph(
                      #ifdef WRATH_TEXTURE_FONT_LINEAR
                      glyph_position,
                      #endif
                      glyph_bottom_left,
                      glyph_size,
                      custom_values_str.values);
  }  
  #else
  {
    pre_compute_glyph(
                      #ifdef WRATH_TEXTURE_FONT_LINEAR
                      glyph_position,
                      #endif
                      glyph_bottom_left,
                      glyph_size);
  }
  #endif
}
