/*! 
 * \file WRATHQTFontSupport.cpp
 * \brief file WRATHQTFontSupport.cpp
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


#include "WRATHConfig.hpp"
#include <QFontInfo>
#include <QFont>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <typeinfo>
#include "WRATHQTFontSupport.hpp"
#include "WRATHFontFetch.hpp"






void
WRATHQT::
generate_font_properties(const QFont &fnt, WRATHFontFetch::FontProperties &chooser)
{
  QFontInfo fnt_info(fnt);
  QByteArray pfamily;
  
  pfamily=fnt_info.family().toUtf8();
  if(!pfamily.isEmpty())
    {
      /*
        extract the foundry name as well.
        Qt's family name string is of the form:
        "Family [Foundry]" the foundary is optional..
      */
      std::string as_string(pfamily.begin(), pfamily.end());
      std::string::iterator iter_open, iter_close;

      iter_open=std::find(as_string.begin(), as_string.end(), '[');
      iter_close=std::find(iter_open, as_string.end(), ']');

      if(iter_open!=as_string.end())
        {
          chooser.family_name(std::string(as_string.begin(), iter_open));

          ++iter_open;
          chooser.foundry_name(std::string(iter_open, iter_close));
        }
      else
        {
          chooser.family_name(as_string);
        }
    }

  chooser
    .bold(fnt_info.bold())
    .italic(fnt_info.italic());

}










