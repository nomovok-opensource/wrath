/*! 
 * \file WRATHFontSupport.hpp
 * \brief file WRATHFontSupport.hpp
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





#ifndef WRATH_HEADER_FONT_SUPPORT_HPP_
#define WRATH_HEADER_FONT_SUPPORT_HPP_

#include "WRATHConfig.hpp"
#include <stdint.h>
#include <list>
#include <set>



/*! \addtogroup Text
 * @{
 */

/*!\namespace WRATHFontSupport
  Namespace to encapsulate support functions
  and classes for font handling.
 */
namespace WRATHFontSupport
{
  /*!\class glyph_index_type
    A glyph_index_type is a wrapper over 
    an integer to represent a raw
    index for a glyph of a font,
    the index represents an element
    of an array, NOT a character 
    code.
   */
  class glyph_index_type:public std::pair<uint32_t, bool>
  {
  public:
    /*!\fn glyph_index_type(uint32_t)
      Ctor. Intializes as a valid and
      the index value to the passed parameter
      \param m intial value to give the index.
     */
    explicit
    glyph_index_type(uint32_t m):
      std::pair<uint32_t, bool>(m, true)
    {}

    /*!\fn glyph_index_type(void)
      Ctor. Intializes the glyph_index_type
      as invalid.
     */
    glyph_index_type(void):
      std::pair<uint32_t, bool>(0, false)
    {}

    /*!\fn uint32_t value(void) const
      Returns the "wrapped integer", i.e the index 
      of the glyph.
     */
    uint32_t 
    value(void) const
    {
      return first;
    }

    /*!\fn void value(uint32_t)
      Sets the "wrapped integer", i.e the index 
      of the glyph and sets the glyph_index_type
      as valid.
      \param v value to set the wrapped integer value to.
     */
    void
    value(uint32_t v)
    {
      first=v;
      second=true;
    }
    
    /*!\fn bool valid
      Indicates if the glyph_index_type is
      a valid index.
     */
    bool 
    valid(void) const
    {
      return second;
    }

    /*!\fn void mark_invalid
      Marks the glyph_index_type
      as invalid.
     */
    void
    mark_invalid(void)
    {
      second=false;
    }
  };

  
  /*!\class character_code_type
    A character_code_type represents a label
    for a glyph, typically the encoding
    is ASCII "compatible", for example
    65 corresponds to 'A'.
   */
  class character_code_type
  {
  public:
    /*!\fn character_code_type
      Ctor. Intializes the character_code_type value
      to the passed parameter.
      \param m intial value to give the character_code_type.
     */
    explicit
    character_code_type(uint32_t m=0):
      m_value(m)
    {}

    /*!\var m_value
      The "wrapped integer", i.e the 
      character code of the glyph.
     */
    uint32_t m_value;

    /*!\fn operator<(character_code_type, character_code_type)
      Comparison operator, equivalent to
      comparing the m_value field.
      \param A left hand side of < operator
      \param B right hand side of < operator
     */
    friend
    bool
    operator<(character_code_type A, character_code_type B)
    {
      return A.m_value<B.m_value;
    }

  };

  
};
/*! @} */

#endif
