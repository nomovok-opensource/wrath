/*! 
 * \file WRATHQTFontSupport.hpp
 * \brief file WRATHQTFontSupport.hpp
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




#ifndef __WRATH_QT_FONT_SUPPORT_HPP__
#define __WRATH_QT_FONT_SUPPORT_HPP__

#include "WRATHConfig.hpp"
#include <QFont>
#include "WRATHFreeTypeSupport.hpp"
#include "WRATHTextureFont.hpp"
#include "WRATHFontSupport.hpp"
#include "WRATHFontFetch.hpp"


/*! \addtogroup Qt
 * @{
 */

namespace WRATHQT
{
  /*!\fn void generate_font_properties(const QFont&, WRATHFontFetch::FontProperties &)
    Set a WRATHFontFetch::FontProperties to specify (or atleast attempt to specify)
    the same font that a QFont uses.
    \param in_fnt QFont from which to generate the additional selection criteria
    \param spec object to which to write font selection requirement
   */
  void
  generate_font_properties(const QFont &in_fnt, 
			   WRATHFontFetch::FontProperties &spec);

  
  /*!\fn WRATHFontFetch::FontProperties generate_font_properties(const QFont&)
    Return a WRATHFontFetch::FontProperties
    whose requirements derive from a QFont.
    \param in_fnt QFont from which to generate the font selection criteria
   */
  inline
  WRATHFontFetch::FontProperties
  generate_font_properties(const QFont &in_fnt)
  {
    WRATHFontFetch::FontProperties R;
    
    generate_font_properties(in_fnt, R);
    return R;
  }

  /*!\fn WRATHTextureFont* fetch_font(const QFont&, int)
    Create/Fetch a WRATHTextureFont object using
    a QFont to specify the font source.
    Equivalent to
    \code
    WRATHFontFetch::fetch_font(pixel_height, generate_font_properties(fnt), type_tag<T>());
    \endcode
    \tparam T WRATHTexureFont derived type, the return font
              will be this type
    \param fnt source QFont 
    \param pixel_height pixel height to use for creation of font.
   */
  template<typename T>
  WRATHTextureFont*
  fetch_font(const QFont &fnt, int pixel_height)
  {
    WRATHFontFetch::FontProperties k;
    WRATHTextureFont *p;
    generate_font_properties(fnt, k);
    p=WRATHFontFetch::fetch_font(pixel_height, k, type_tag<T>());
    return p;
  }

  
}

/*! @} */

#endif
