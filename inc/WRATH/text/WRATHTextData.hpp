/*! 
 * \file WRATHTextData.hpp
 * \brief file WRATHTextData.hpp
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


/*! \addtogroup Text
 * @{
 */


#ifndef WRATH_HEADER_TEXT_DATA_HPP_
#define WRATH_HEADER_TEXT_DATA_HPP_

#include "WRATHConfig.hpp"
#include "WRATHassert.hpp" 
#include <vector>
#include <boost/utility.hpp>
#include "vectorGL.hpp"
#include "WRATHTextureFont.hpp"

/*!\class WRATHTextData
  A WRATHTextData represents a sequence
  of characters (including control characters).
  Internally, a WRATHTextData is just an array
  uint32_t's. If the leading bit is up,
  it indicates that the lower 31 bits are
  a glyph index, if the leading bit is down,
  then it indicates that the lower 31 bits
  is a character code.
 */
class WRATHTextData:boost::noncopyable
{
public:    
  /*!\class character
    Essentially a wrapper to a 32-bit
    integer. Encapsulates if the value
    refers to a raw glyph index or a 
    character code.
   */
  class character
  {
  public:

    /*!\fn character(void)
      Default ctor, initializes to represent
      the character of character code 0.
     */
    character(void):
      m_value(0)
    {}
    
    /*!\fn character(WRATHTextureFont::character_code_type)
      Ctor to initialize to refer to the character
      of a character code.
      \param pc character code
     */
    character(WRATHTextureFont::character_code_type pc):
      m_value(pc.m_value&(~bit31))
    {}
    
    /*!\fn character(WRATHTextureFont::glyph_index_type)
      Ctor to initialize to refer to a glyph index.
      \param pc glyph index 
     */
    character(WRATHTextureFont::glyph_index_type pc):
      m_value(pc.value()|bit31)
    {}
    
    /*!\fn character(uint32_t)
      Ctor to initialize to refer to the character
      of a character code.
      \param pc character code
     */
    character(uint32_t pc):
      m_value(pc&(~bit31))
    {}
    
    /*!\fn character(int32_t)
      Ctor to initialize to refer to the character
      of a character code.
      \param pc character code
     */    
    character(int32_t pc):
      m_value(static_cast<uint32_t>(pc)&(~bit31))
    {}
    
    /*!\fn character(uint8_t)
      Ctor to initialize to refer to the character
      of a character code.
      \param pc character code
     */    
    character(uint8_t pc):
      m_value(pc&(~bit31))
    {}
    
    /*!\fn character(int8_t)
      Ctor to initialize to refer to the character
      of a character code.
      \param pc character code
     */    
    character(int8_t pc):
      m_value(static_cast<uint32_t>(pc)&(~bit31))
    {}
    
    /*!\fn character(uint16_t)
      Ctor to initialize to refer to the character
      of a character code.
      \param pc character code
     */    
    character(uint16_t pc):
      m_value(pc&(~bit31))
    {}
    
    /*!\fn character(int16_t)
      Ctor to initialize to refer to the character
      of a character code.
      \param pc character code
     */    
    character(int16_t pc):
      m_value(static_cast<uint32_t>(pc)&(~bit31))
    {}

    /*!\fn WRATHTextureFont::glyph_index_type glyph_index
      If this character refers to a glyph index,
      returns that glyph index, otherwise returns an
      invalid glyph_index (i.e. glyph_index().valid() is false).
     */
    WRATHTextureFont::glyph_index_type
    glyph_index(void) const
    {
      return m_value&bit31?
        WRATHTextureFont::glyph_index_type(m_value&~bit31):
        WRATHTextureFont::glyph_index_type();
    }

    /*!\fn WRATHTextureFont::character_code_type character_code
      If this character refers to a character code,
      returns that character code, otherwise returns the
      character code 0.
     */
    WRATHTextureFont::character_code_type
    character_code(void) const
    {
      return m_value&bit31?
        WRATHTextureFont::character_code_type(0):
        WRATHTextureFont::character_code_type(m_value);
    }

    /*!\fn bool is_glyph_index
      Returns true if and only if this refers
      to a glyph index.
     */
    bool
    is_glyph_index(void) const
    {
      return (m_value&bit31)!=0;
    }

    /*!\fn bool operator<(character) const
      Comparison operator for sorting.
      \param rhs value to which to compare
     */
    bool
    operator<(character rhs) const
    {
      return m_value<rhs.m_value;
    }

    /*!\fn bool bool operator==(character) const
      Comparison operator for equality.
      \param rhs value to which to compare
     */
    bool
    operator==(character rhs) const
    {
      return m_value==rhs.m_value;
    }

  private:
    uint32_t m_value;
    static const uint32_t bit31=1<<31;
  };

  /*!\fn WRATHTextData
    Ctor, initializes the WRATHTextData as empty.
  */
  WRATHTextData(void)
  {}
  
  /*!\fn void clear
    Clears this WRATHTextData.
   */
  void
  clear(void)
  {
    m_data.clear();
  }
  
  /*!\fn const std::vector<character>& character_data(void) const
    Returns the data of this WRATHTextData.
   */
  const std::vector<character>&
  character_data(void) const
  {
    return m_data;
  }

  /*!\fn character character_data(unsigned int) const
    Conveniance function, equivalent
    to character_data()[i].
    \param i which chararater of the string to get.
   */
  character
  character_data(unsigned int i) const
  {
    WRATHassert(i<m_data.size());
    return m_data[i];
  }
  
  /*!\fn void push_back
    Explicit adding of a character 
    element.
    \param C character to add 
   */
  void
  push_back(character C)
  {
    m_data.push_back(C);
  }

  /*!\fn void append
    Appends a sequence of character codes
    to this WRATHTextData.
    \tparam iterator iterator type to \ref character type
    \param begin iterator to 1st character code to add
    \param end iterator to one past the last character code to add
   */
  template<typename iterator>
  void
  append(iterator begin, iterator end)
  {
    m_data.reserve( m_data.size() + std::distance(begin,end));
    for(;begin!=end; ++begin)
      {
        push_back(*begin);
      }
  }
 
  
private:
  std::vector<character> m_data;
};
/*! @} */




#endif
