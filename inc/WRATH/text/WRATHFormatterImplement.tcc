// -*- C++ -*-

/*! 
 * \file WRATHFormatterImplement.tcc
 * \brief file WRATHFormatterImplement.tcc
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


#if !defined(__WRATH_FORMATTER_HPP__) || defined(__WRATH_FORMATTER_IMPLEMENT_TCC__)
#error "Direction inclusion of private header file WRATHFormatterImplement.tcc" 
#endif

#define __WRATH_FORMATTER_IMPLEMENT_TCC__

namespace WRATHFormatterImplement
{
  template<typename iterator>
  int
  simple_text_legnth(int coordinate,
                     WRATHTextureFont *fnt, 
                     iterator begin, iterator end,
                     bool kern)
  {
    int return_value(0);
    WRATHTextureFont::glyph_index_type prev;
    
    for(;begin!=end; ++begin)
      {
        WRATHTextData::character ch(*begin);
        WRATHTextureFont::glyph_index_type G;
        int from_glyph, kern_value;
        
        G=(ch.glyph_index().valid())?
          ch.glyph_index():
          fnt->glyph_index(ch.character_code());
        
        kern_value=(kern)?
          fnt->kerning_offset(prev, G)[coordinate]:
          0;
        
        
        const WRATHTextureFont::glyph_data_type &gl(fnt->glyph_data(G));
        from_glyph=gl.glyph_index().valid()?
          gl.iadvance()[coordinate]:
          0;
   
        return_value+=(kern_value+from_glyph);
                        
        prev=G;
      }

    return return_value;
  }
}

template<typename iterator> 
int
WRATHFormatter::
simple_text_width(WRATHTextureFont *fnt, 
                  iterator begin, iterator end,
                  bool kern)
{
  return WRATHFormatterImplement::simple_text_legnth(0, fnt, begin, end, kern);
}

template<typename iterator> 
int
WRATHFormatter::
simple_text_height(WRATHTextureFont *fnt, 
                   iterator begin, iterator end,
                   bool kern)
{
  return WRATHFormatterImplement::simple_text_legnth(1, fnt, begin, end, kern);
}
