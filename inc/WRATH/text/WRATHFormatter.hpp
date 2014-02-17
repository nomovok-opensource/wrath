/*! 
 * \file WRATHFormatter.hpp
 * \brief file WRATHFormatter.hpp
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




#ifndef WRATH_HEADER_FORMATTER_HPP_
#define WRATH_HEADER_FORMATTER_HPP_

#include "WRATHConfig.hpp"
#include <algorithm>
#include <boost/utility.hpp>
#include "WRATHReferenceCountedObject.hpp"
#include "WRATHTextData.hpp"
#include "WRATHTextureFont.hpp"
#include "WRATHStateStream.hpp"



/*! \addtogroup Text
 * @{
 */

/*!\class WRATHFormatter
  A WRATHFormatter is an interface on formatting blocks
  of text. A WRATHFormatter defines only an interface,
  derived classes need to implement it.
 */
class WRATHFormatter:
  public WRATHReferenceCountedObjectT<WRATHFormatter>
{
public:
  /*!\typedef glyph_index_type  
    Convenience typedef to WRATHTextureFont::glyph_index_type 
   */
  typedef WRATHTextureFont::glyph_index_type glyph_index_type;

  /*!\typedef character_code_type
    Conveniance typedef to WRATHTextureFont::character_code_type
   */
  typedef WRATHTextureFont::character_code_type character_code_type;

  /*!\enum screen_orientation_type
    Enumeration type to specify the y-orientation
    of the text renderer, this comes into play
    for different projection matrices.
   */
  enum screen_orientation_type
    {
      /*!
        y-coordinates increase as one moves
        vertically down the screen, i.e.
        y=0 is the "top of the screen"
       */
      y_increases_downward, 

      /*!
        y-coordinates increase as one moves
        vertically up the screen, i.e.
        y=0 is the "bottom of the screen"
       */
      y_increases_upward,
    };

  /*!\enum alignment_type
    Enumeration for alignment, used to 
    specify both vertical and horizontal 
    alignment. Enumeration is provided
    as a set of common enumeration values
    and types that WRATHFormatter deverived
    classes may use.
   */
  enum alignment_type
    {
      /*!
        Alignment is so for left to right text
        this means align left, for right to
        left text (such as Hebrew) align to the
        right.
       */
      align_text_begin,

      /*!
        Alignment is so for left to right text
        this means align right, for right to
        left text (such as Hebrew) align to the
        left.
       */
      align_text_end,

      /*!
        Align text centered on the line.
       */
      align_center,
    };


  /*!\enum pen_advance_type
    Enumeration to describe "pen" advance
    of text layout, i.e. left to right,
    right to left, etc. Enumeration is provided
    as a set of common enumeration values
    and types that WRATHFormatter deverived
    classes may use.
   */
  enum pen_advance_type
    {
      /*!
        Increase coordinate, use in
        for example in the horizontal
        pen advancement for left
        to right text and also used
        in the vertical pen advancement
        for text read top to bottom.
       */
      increase_coordinate=0,

      /*!
        Decrease coordinate, use in
        for example in the horizontal
        pen advancement for right
        to left text and also used
        in the vertical pen advancement
        for text read bottom to top.
       */
      decrease_coordinate=1
    };

  /*!\enum text_orientation_type
    Specifies the orientation of the text, 
    i.e is the text written horizontally or
    vertically. Enumeration is provided
    as a set of common enumeration values
    and types that WRATHFormatter deverived
    classes may use.
   */
  enum text_orientation_type
    {
      /*!
        Text pen advances horizontally
        on successive character and
        vertically on new lines. 
       */
      horizontal_orientation=0,

      /*!
        Text pen advances vertically
        on successive character and
        horizontally on new lines. 
       */
      vertical_orientation=1
    };


  /*!\class pen_position_return_type
    A pen_position_return_type gives
    pen position data to allow for
    successive texts of different
    formatting to proceeed each other.
   */
  class pen_position_return_type
    {
    public:
      /*!\var m_exact_pen_position
        The "exact" pen position,
        i.e. the coordinates of the last
        character, all white spaces 
        (i.e. \\n, \\t, etc) are viewed as 
        characters. 
       */
      vec2 m_exact_pen_position;

      /*!\var m_descend_start_pen_position
        In the coordinate of the advance
        character coordinate, returns the
        "start" position for a new line
        and in the coordinate of the
        advance line character returns
        the location of the maximum descend
        of the last line. If one wishes to 
        have text proceed on the next line
        with a different format, request this
        pen positition and prepend an EOL
        to the next text chunk.
       */
      vec2 m_descend_start_pen_position;
    };

  /*!\class glyph_instance
    A glyph_instance holds position,
    and a pointer to a glyph data of
    what character to draw.
   */
  class glyph_instance
  {
  public:
    glyph_instance(void):
      m_glyph(NULL),
      m_position(0,0)
    {}

    /*!\var m_glyph
      Pointer to character data of glyph,
      the value may be NULL. If it is NULL,
      then the source of the \ref glyph_instance 
      was a non-printing formatting character,
      such as \\n, there are kept so that
      the changees in the state stream
      of a \ref WRATHTextDataStream stay in 
      "sync" with the formatted character stream.
     */
    const WRATHTextureFont::glyph_data_type *m_glyph;

    /*!\var m_position
      Position of the character without taking into
      account WRATHTextureFont::glyph_data_type::origin(), 
      of \ref m_glyph i.e. for example
      for horizontally advancing text, .y()
      holds the position of the base line and
      .x() holds the "pen position" of where the
      glyph is drawn, i.e. the left side of the
      glyph.
     */
    vec2 m_position;
  };

  /*!\class LineData
    A LineData lists the information
    of a line of text, such as it's
    range and location.
   */
  class LineData
  {
  public:
    /*!\fn LineData
      Default ctor, initializes \ref m_range
      by arguments, both \ref m_max_ascend
      and \ref m_max_descend as 0.0 and 
      all other properties are UNITIALIZED.
      \param b value to whcih to initialize m_range.m_begin
      \param e value to whcih to initialize m_range.m_end
     */
    explicit
    LineData(int b=-1, int e=-1):
      m_range(b,e),
      m_max_ascend(0.0f),
      m_max_descend(0.0f)
    {}

    /*!\var m_range   
      The range of characaters of the 
      line.
     */
    range_type<int> m_range;

    /*!\var m_pen_position_start
      The pen position of the start of the 
      line. For horizontally oriented
      text, .x() gives the pen position
      where the line starts and .y()
      gives the location of the base
      line. For vertically oriented
      text, .y() gives the pen position
      where the line starts and .x()
      gives the location of the base
      line.  
     */
    vec2 m_pen_position_start;

    /*!\var m_pen_position_end    
      The pen position just before the
      end of the line. For horizontally 
      oriented text, .x() gives the pen 
      position where the line ends, i.e.
      after the last character, and .y()
      gives the location of the base
      line. For vertically oriented
      text, .y() gives the pen position
      where the line ends and .x()
      gives the location of the base
      line.  
     */
    vec2 m_pen_position_end;

    /*!\var m_max_ascend    
      The maximum ascend of the line.
     */
    float m_max_ascend;

    /*!\var m_max_descend    
      The maximum descend of the line.
     */
    float m_max_descend;
  };

  virtual
  ~WRATHFormatter()
  {}

 
  /*!\fn enum screen_orientation_type screen_orientation
    To be implemented by a derived class to
    report the screen orientation used
    to layout text.
   */
  virtual
  enum screen_orientation_type
  screen_orientation(void)=0;

  /*!\fn pen_position_return_type format_text
    To be implemented by a derived class to
    perform the actual formatting. A derived 
    class _MUST_ make sure that the output and 
    input stay in sync.  This is accomplished 
    by for those array elements of raw_data 
    that do not correspond to a glpyh to place
    a NULL glyph at the location for the out_data. 
    A derived class _should_ observe the change 
    font stream  (see \ref WRATHText::font) and the 
    change scale stream (see \ref WRATHText::scale).
    
    \param raw_data text to be formatted
    \param state_stream state change stream packet
                        that the formatter operated
                        on.                      
    \param out_data location to place formatted text.
    \param out_eols locations within stream of EOL's.
                    including those EOL's added by 
                    formatting, .first of each element
                    is to have the same value as
                    .second.m_range.m_begin
  */
  virtual
  pen_position_return_type
  format_text(const WRATHTextData &raw_data,
              const WRATHStateStream &state_stream,
              std::vector<glyph_instance> &out_data,
              std::vector< std::pair<int, LineData> > &out_eols)=0;

 
  
  /*!\fn int simple_text_width(WRATHTextureFont*, 
                               iterator, iterator,
                               bool)
    Walks an STL range of text computing
    the width of placing the characters one
    after the other. Returns an integer in
    value of 64'ths of a pixel.
    \param fnt WRATHTextureFont with which to compute the width
    \param begin iterator to first character of text
    \param end iterator to one past last character of text
    \param kern if true observe kerning
    \tparam iterator iterator type for which operator* returns a type
                     from which \ref glyph_instance is
                     constructable (in order to name a character code
                     from which to fetch a glyph).
   */
  template<typename iterator> 
  static
  int
  simple_text_width(WRATHTextureFont *fnt, 
                    iterator begin, iterator end,
                    bool kern=true);
  
  /*!\fn int simple_text_height(WRATHTextureFont*, 
                               iterator, iterator,
                               bool)
    Walks an STL range of text computing
    the height of placing the characters one
    after the other vertically.  Returns an integer in
    value of 64'ths of a pixel.
    \param fnt WRATHTextureFont with which to compute the width
    \param begin iterator to first character of text
    \param end iterator to one past last character of text
    \param kern if true observe kerning
    \tparam iterator iterator type for which operator* returns a type
                     from which \ref glyph_instance is
                     constructable (in order to name a character code
                     from which to fetch a glyph).
   */
  template<typename iterator> 
  static
  int
  simple_text_height(WRATHTextureFont *fnt, 
                     iterator begin, iterator end,
                     bool kern=true);

    
  /*!\fn int simple_text_width(WRATHTextureFont*, 
                              iterator, iterator,
                              enum pen_advance_type,
                              bool)
    Walks an STL range of text computing
    the width of placing the characters one
    after the other.  Returns an integer in
    value of 64'ths of a pixel.
    \param fnt WRATHTextureFont with which to compute the width
    \param begin iterator to first character of text
    \param end iterator to one past last character of text
    \param kern if true observe kerning
    \param tp dictates if rendering text right to left or left to right
    \tparam iterator iterator type for whic operator* returns a type
                     from which \ref glyph_instance is
                     constructable (in order to name a character code
                     from which to fetch a glyph).
   */
  template<typename iterator> 
  static
  int
  simple_text_width(WRATHTextureFont *fnt, 
                    iterator begin, iterator end,
                    enum pen_advance_type tp,
                    bool kern=true)
  {
    return (tp==increase_coordinate)?
      simple_text_width(fnt, begin, end, kern):
      simple_text_width(fnt, 
                        std::reverse_iterator<iterator>(end),
                        std::reverse_iterator<iterator>(begin),
                        kern);
  }
  
};

/*!\fn std::ostream& operator<<(std::ostream&, const WRATHFormatter::LineData&)
  Overloaded operator<< to print the values of a WRATHFormatter::LineData
  \param ostr std::ostream stream to which to print
  \param eol value to print
*/
inline
std::ostream&
operator<<(std::ostream &ostr, const WRATHFormatter::LineData &eol)
{
  ostr << "Line{ [" << eol.m_range.m_begin
       << ", " << eol.m_range.m_end << "): "
       << eol.m_pen_position_start << ", "
       << eol.m_pen_position_end;
  return ostr;
}
/*! @} */

#include "WRATHFormatterImplement.tcc"



#endif
