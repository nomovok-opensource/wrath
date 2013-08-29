/*! 
 * \file WRATHFormattedTextStream.hpp
 * \brief file WRATHFormattedTextStream.hpp
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




#ifndef __WRATH_FORMATTED_TEXT_DATA_HPP__
#define __WRATH_FORMATTED_TEXT_DATA_HPP__

#include "WRATHConfig.hpp"
#include <algorithm>
#include <boost/utility.hpp>
#include "WRATHTextData.hpp"
#include "WRATHTextureFont.hpp"
#include "WRATHFormatter.hpp"

/*! \addtogroup Text
 * @{
 */

/*!\class WRATHFormattedTextStream
  A WRATHFormattedTextStream represents a stream
  of formatted characters and end of line
  data of the formatting.
 */
class WRATHFormattedTextStream
{
public:

  /*!\typedef glyph_index_type
    Convenience typedef to WRATHTextureFont::glyph_index_type
   */
  typedef WRATHTextureFont::glyph_index_type glyph_index_type;

  /*!\typedef character_code_type
    Convenience typedef to WRATHTextureFont::character_code_type
   */
  typedef WRATHTextureFont::character_code_type character_code_type;

  /*!\typedef glyph_instance
    Convenience typedef to WRATHFormatter::glyph_instance
   */
  typedef WRATHFormatter::glyph_instance glyph_instance;

  /*!\enum corner_type
    Enumeration describing the corners 
    of a quad used to draw a glyph.
   */
  enum corner_type
    {
      /*!
        Bottom left corner
       */
      bottom_left_corner=0,

      /*!
        Bottom right corner
       */
      bottom_right_corner=1,

      /*!
        top right corner
       */
      top_right_corner=2,

      /*!
        top left corner
       */
      top_left_corner=3,

      /*!
        indicates not a corner
       */
      not_corner,
    };

  /*!\fn WRATHFormattedTextStream
    Default ctor, initializes the formatted text as empty.
   */
  WRATHFormattedTextStream(void);
  
  /*!\fn WRATHFormatter::pen_position_return_type set_text
    Resets the WRATHFormattedTextStream from a WRATHTextData
    and WRATHStateStream using a WRATHFormatter to dictate 
    the layout. Returns the positional data of the pen
    after formatting.
    \param fmt pointer to WRATHFormatter to dictate the layout
    \param raw_data text to layout
    \param state_stream the WRATHStateStream of the WRATHTextDataStream
                        that generated in_data.    
   */
  WRATHFormatter::pen_position_return_type 
  set_text(WRATHFormatter::handle fmt,
           const WRATHTextData &raw_data,
           const WRATHStateStream &state_stream);
  
  
  /*!\fn const std::vector<glyph_instance>& data_stream
    Returns the formatted text data, each entry having
    it's glyph index and position.
   */
  const std::vector<glyph_instance>&
  data_stream(void) const
  {
    return m_data;
  }

  /*!\fn const glyph_instance& data
    Conveniance function, equivalent to
    data_stream()[i].
    \param i index into data_stream()
   */
  const glyph_instance&
  data(int i) const
  {
    WRATHassert(i>=0);
    WRATHassert(i<static_cast<int>(m_data.size()));
    return m_data[i];
  }

  /*!\fn const std::vector<std::pair<int, WRATHFormatter::LineData> >& eols
    Returns the locations and line data
    of the lines of the formatted text,
    .first indicates when the named line
    begins, thue eols()[i].first equals
    eols()[i].second.m_range.m_begin.
  */
  const std::vector<std::pair<int, WRATHFormatter::LineData> >&
  eols(void) const
  {
    return m_eols;
  }

  /*!\fn ivec2 texture_coordinate(int, enum corner_type, enum WRATHTextureFont::texture_coordinate_size) const
    Returns the texture coordinate of the named
    corner of the named glyph of data_stream().
    \param i index into data_stream()
    \param ct which corner.
    \param L specifiy to ue minified or native
             resolution of the glyph.
   */
  ivec2
  texture_coordinate(int i, enum corner_type ct,
                     enum WRATHTextureFont::texture_coordinate_size L) const;

  /*!\fn vecN<ivec2, 2> texture_coordinate(int, enum WRATHTextureFont::texture_coordinate_size) const
    Returns the texture coordinates of
    the named glyph of data_stream(), the
    bottom left is returned as [0] and
    the top right as [1].
    \param i index into data_stream()
    \param L specifiy to ue minified or native
             resolution of the glyph.
   */
  vecN<ivec2, 2>
  texture_coordinate(int i,
                     enum WRATHTextureFont::texture_coordinate_size L) const;

  /*!\fn vec2 position(int, enum corner_type, vec2, enum WRATHTextureFont::texture_coordinate_size) const
    Returns the xy-position of the named
    corner of the named glyph of data_stream().
    \param i index into data_stream()
    \param ct which corner.
    \param scale_factor scaling factor to apply to glyph size in each dimention seperately
    \param L specifiy to ue minified or native
             resolution of the glyph.
   */
  vec2
  position(int i, enum corner_type ct, vec2 scale_factor,
           enum WRATHTextureFont::texture_coordinate_size L) const;

  /*!\fn vecN<vec2,2> position(int, vec2, enum WRATHTextureFont::texture_coordinate_size) const
    Returns the xy-positions of the corners of
    the named glyph of data_stream(), the
    bottom left is returned as [0] and
    the top right as [1].
    \param i which character of data_stream()
    \param scale_factor scaling factor to apply to glyph size in each dimention seperately
    \param L specifiy to ue minified or native
             resolution of the glyph.
   */
  vecN<vec2,2>
  position(int i, vec2 scale_factor,
           enum WRATHTextureFont::texture_coordinate_size L) const;

  /*!\fn enum WRATHFormatter::screen_orientation_type orientation
    Returns the y-orientation of the formatted
    text as specified by the formatter used
    to format this WRATHFormattedTextStream.
   */
  enum WRATHFormatter::screen_orientation_type
  orientation(void)
  {
    return m_orientation;
  }

  /*!\fn float y_factor
    Y-multiplier for computing glyph corner
    positions, is -1.0f if orientation() 
    returns WRATHFormatter::y_increases_upward
    and +1.0f otherwise.
   */
  float
  y_factor(void) const
  {
    return m_yfactor;
  }

  /*!\fn bool y_factor_positive
    Returns true if the y-factor is positive.
    see also \ref y_factor()
   */
  bool 
  y_factor_positive(void) const
  {
    return m_y_factor_positive;
  }

  /*!\fn void swap
    STL compliant swap function
    \param obj WRATHFormattedTextStream object 
               to swap contents with.
   */
  void
  swap(WRATHFormattedTextStream &obj)
  {
    std::swap(obj.m_data, m_data);
    std::swap(obj.m_orientation, m_orientation);
    std::swap(obj.m_yfactor, m_yfactor);
    std::swap(obj.m_eols, m_eols);
    std::swap(obj.m_y_factor_positive, m_y_factor_positive);
  }

private:
  std::vector<std::pair<int, WRATHFormatter::LineData> > m_eols;
  std::vector<glyph_instance> m_data;
  enum WRATHFormatter::screen_orientation_type m_orientation;
  float m_yfactor;
  bool m_y_factor_positive;
};

namespace std
{
  template <>
  inline
  void
  swap(WRATHFormattedTextStream &A, WRATHFormattedTextStream &B)
  {
    A.swap(B);
  }
}
/*! @} */

#endif
