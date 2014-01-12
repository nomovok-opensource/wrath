/*! 
 * \file font_shader_texture_page_data.wrath-shader.glsl
 * \brief file font_shader_texture_page_data.wrath-shader.glsl
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

#ifdef WRATH_FONT_TEXTURE_PAGE_DATA_EMPTY
highp float
wrath_font_page_data(in int idx)
{
  return 0.0;
}

#else

/*
  This variable name must match with the uniform fetch name used
  in WRATHTextureFontDrawer
 */
uniform highp float wrath_font_page_data_uniforms[WRATH_FONT_TEXTURE_PAGE_DATA_SIZE];

highp float
wrath_font_page_data(in int idx)
{
  return wrath_font_page_data_uniforms[idx];
}

#endif
