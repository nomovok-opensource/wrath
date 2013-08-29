/*! 
 * \file FileType.hpp
 * \brief file FileType.hpp
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


#ifndef __FILE_TYPES_HPP__
#define __FILE_TYPES_HPP__

#include "WRATHConfig.hpp"

namespace FileType
{
  enum file_fetch_type
    {
      load_interpreted,
      load_utf8,
      load_utf16,
      load_raw,
      load_raw_utf8,
      load_raw_utf16,
      load_image,
      load_font,
      load_font_subrange,
      load_directory
    };
}

#endif
