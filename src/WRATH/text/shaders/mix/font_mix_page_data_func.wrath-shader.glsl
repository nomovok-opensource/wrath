/*! 
 * \file font_mix_page_data_func.wrath-shader.glsl
 * \brief file wrath_font_page_data.wrath-shader.glsl
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

float
wrath_font_page_data(in int idx)
{
  return wrath_font_page_data_original_function(idx + WRATH_MIX_FONT_PAGE_DATA_OFFSET);
}
