/*! 
 * \file WRATHTextDataStreamManipulator.hpp
 * \brief file WRATHTextDataStreamManipulator.hpp
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





#ifndef WRATH_HEADER_TEXT_DATA_STREAM_MANIPULATOR_HPP_
#define WRATH_HEADER_TEXT_DATA_STREAM_MANIPULATOR_HPP_


#include "WRATHConfig.hpp"
#include "WRATHTextData.hpp"
#include "WRATHTextureFont.hpp"
#include "WRATHStateStreamManipulators.hpp"

/*! \addtogroup Text
 * @{
 */

namespace WRATHText
{

  using namespace WRATHStateStreamManipulators;

  /*!\typedef color_type
    typedef to vecN<GLubyte,4> for specifying font colors
   */
  typedef vecN<GLubyte,4> color_type; 
    
  /*!\enum letter_spacing_e
    Enumeration to specify letter spacing when
    text layout occurs.
   */
  enum letter_spacing_e
    {
      /*!
        For this letter spacing mode, the 
        space between letters is increased
        by an absolute value whose units
        are pixels
       */
      letter_spacing_absolute,

      /*!
        For this spacing mode, the space
        after a letter is increased by
        a value whose units are the size
        of the glyph. For for horizontal 
        oriented text, i.e. WRATHFormatter::horizontal_orientation
        the units are then in width's of 
        the letter and for vertically oriented text,
        i.e. WRATHFormatter::vertical_orientation
        the units are then in height's of 
        the letter
       */
      letter_spacing_relative
    };

  /*!\enum capitalization_e
    Enumeration to describe capitalization 
    stype when text layout occurs.
   */
  enum capitalization_e
    {
      /*!
        Do not change the capitalization
        of letters in the stream
       */
      capitalization_as_in_stream,

      /*!
        All letters lower case
       */
      capitalization_all_lower_case,

      /*!
        All letters upper case
       */
      capitalization_all_upper_case,
      
      /*!
        First letter of each word is 
        made upper case, other letters 
        are made lower case.
       */
      capitalization_title_case
    };

  
  /*!\class WRATHText::localization 
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
     - WRATHTextDataStream objects initialized with default std::locale
     - localization dictates conversion to upper and/or lower case
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(localization, std::locale) 

  /*!\class WRATHText::capitalization
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
     - WRATHTextDataStream objects initialized with default \ref capitalization_as_in_stream
     - controls how/if capitalization is acted on the stream
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(capitalization, enum capitalization_e)

  /*!\class WRATHText::letter_spacing
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
     - WRATHTextDataStream objects initialized with value 0.0f
     - see \ref letter_spacing_type and \ref letter_spacing_e
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(letter_spacing, float)
  
  /*!\class WRATHText::letter_spacing_type
    See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
    letter_spacing_type. Determines the units for the
    letter spacing value specified in letter_spacing.
     - WRATHTextDataStream objects initialized with value letter_spacing_absolute
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(letter_spacing_type, letter_spacing_e)

  /*!\class WRATHText::z_position
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
     - WRATHTextDataStream objects initialized with value -1.0f
     - is a geometry position for when text is transformed
       with a 3D-perspective transformation
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(z_position, float)

  /*!\class WRATHText::kerning
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
     - WRATHTextDataStream objects initialized with value true
     - enable/disable kerning between letters
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(kerning, bool)

  /*!\class WRATHText::word_spacing
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
     - WRATHTextDataStream objects initialized with value 0.0
     - space between words in increased by the value word_spacing,
       units are unscaled pixels.
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(word_spacing, float)

  /*!\class WRATHText::scale
     - WRATHTextDataStream objects initialized with value 1.0
     - scaling factor applied to pixel_size
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(scale, float)
  
  /*!\class WRATHText::pixel_size 
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
     - stream initialized with value 32.0
     - specifies pixel size. Actual font display size
       is the product of the pixel size and the scale
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(pixel_size, float)

  /*!\class WRATHText::horizontal_stretching
     - WRATHTextDataStream objects initialized with value 1.0
     - stretches letter horizontally as indicated by value,
       i.e. 1.0 = no strectching, 0.5=character width halved, etc.
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(horizontal_stretching, float)
  
  /*!\class WRATHText::vertical_stretching
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
     - WRATHTextDataStream objects initialized with value 1.0
     - stretches letter vertically as indicated by value,
       i.e. 1.0 = no strectching, 0.5=character height halved, etc.
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(vertical_stretching, float)

  /*!\class WRATHText::font
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
     - WRATHTextDataStream objects initialized the current return value of WRATHFontFetch::fetch_default_font().
     - font applied to proceeding characters
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(font, WRATHTextureFont*)

  /*!\class WRATHText::baseline_shift_y
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
     - WRATHTextDataStream objects initialized with value 0.0
     - how much to shift the base line in y-direction, for subscripts and super-scripts    
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(baseline_shift_y, float)

  /*!\class WRATHText::baseline_shift_x
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
     - WRATHTextDataStream objects initialized with value 0.0
     - how much to shift the base line in x-direction, for subscripts and super-scripts    
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(baseline_shift_x, float)

  /*!\class WRATHText::color_bottom_left
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
    - WRATHTextDataStream objects initialized with value color_type(255, 255, 255, 255) [opaque white]
    - color of corner of each glyph, color interpolated linearly across glyphs
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(color_bottom_left, color_type)

  /*!\class WRATHText::color_bottom_right
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
    - WRATHTextDataStream objects initialized with value color_type(255, 255, 255, 255) [opaque white]
    - color of corner of each glyph, color interpolated linearly across glyphs
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(color_bottom_right, color_type)

  /*!\class WRATHText::color_top_left
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
    - WRATHTextDataStream objects initialized with value color_type(255, 255, 255, 255) [opaque white]
    - color of corner of each glyph, color interpolated linearly across glyphs
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(color_top_left, color_type)

  /*!\class WRATHText::color_top_right
     See \ref WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY
    - WRATHTextDataStream objects initialized with value color_type(255, 255, 255, 255) [opaque white]
    - color of corner of each glyph, color interpolated linearly across glyphs
   */
  WRATH_STATE_STREAM_DECLARE_IMPLEMENT_PROPERTY(color_top_right, color_type)

  /*!\class stream_defaults
    Manipulator that when placed in a stream resets
    those stream values that have a default value
    to their default value.
   */
  class stream_defaults {};

  /*!\namespace WRATHText::effective_scale
    The scaling factor to apply to a glyph is:\n\n

    scale * pixel_size / font->pixel_size()\n\n

    which is a headache to track since it is 3 different
    state streams. The namespace effective_scale
    provides the functions
    - sub_range
    - update_value_from_change
    - init_stream_iterator

    and the type
    - stream_iterator (to walk the values)

    to allow one to get the effective scale factors.
   */
  namespace effective_scale
  {
    /*!\class stream_iterator
      iterator type interface to "get" the effective
      scaling factor within a state stream
     */
    class stream_iterator
    {
    public:
      /*!\fn stream_iterator()
        Default ctor.
       */
      stream_iterator(void);

      /*!\fn float sub_range(int)
        Analogue to \ref WRATHStateStreamManipulators::generic_state::sub_range()
        for effective scale value, i.e. increment this \ref stream_iterator
        until start_index is reached
        \param start_index location to which to increment 
       */
      float
      sub_range(int start_index);

      /*!\fn bool update_value_from_change(int, float&)
        Analogue to \ref WRATHStateStreamManipulators::generic_state::update_value_from_change()
        for effective scale value
        \param current_index new index value since \ref init_stream_iterator or
                             \ref update_value_from_change(int) or update_value_from_change(int, float&)
                             has been called
        \param out_value location to write new effice scale value if the value has changed
       */
      bool
      update_value_from_change(int current_index, float &out_value);

      /*!\fn bool update_value_from_change(int)
        Analogue to \ref WRATHStateStreamManipulators::generic_state::update_value_from_change()
        for effective scale value; overload version that does not return
        via parameter refernce the new effective scale value.
        \param current_index new index value since \ref init_stream_iterator or
                             \ref update_value_from_change(int) or update_value_from_change(int, float&)
                             has been called
       */
      bool
      update_value_from_change(int current_index);

      /*!\fn float init_stream_iterator(const WRATHStateStream&, int) 
        Analogue to \ref WRATHStateStreamManipulators::generic_state::init_stream_iterator()
        for effective scale value
        \param state_stream WRATHStateStream to query
        \param start_index location to state the iterator
       */
      float
      init_stream_iterator(const WRATHStateStream &state_stream, int start_index);

      /*!\fn float pixel_size
        Returns the actual value for the pixel size,
        see \ref WRATHText::pixel_size .
       */
      float
      pixel_size(void) const
      {
        return m_pixel_size;
      }

      /*!\fn scale
        Returns the actual value for the scale,
        see \ref WRATHText::scale
       */
      float
      scale(void) const
      {
        return m_scale;
      }

      /*!\fn font
        Returns the actual value for the font,
        see \ref WRATHText::font
       */ 
      WRATHTextureFont*
      font(void) const
      {
        return m_font;
      }

    private:
      
      float
      compute_effective_scale(void) const;

      WRATHTextureFont *m_font;
      float m_pixel_size;
      float m_scale;

      font::stream_iterator m_font_stream;
      pixel_size::stream_iterator m_pixel_size_stream;
      scale::stream_iterator m_scale_stream;
    };

    /*!\fn bool update_value_from_change(int, float&, stream_iterator&)
      Provided as a conveninace, equivalent to
      \code
      R.update_value_from_change(current_index, out_value, R)
      \endcode
      \param current_index echoed to stream_iterator::update_value_from_change(int, float&)
      \param out_value echoed to stream_iterator::update_value_from_change(int, float&)
      \param R stream_iterator object calling stream_iterator::update_value_from_change(int,float&)
     */
    inline
    bool
    update_value_from_change(int current_index, float &out_value, stream_iterator &R)
    {
      return R.update_value_from_change(current_index, out_value);
    }

    /*!\fn bool update_value_from_change(int, stream_iterator&)
      Provided as a conveninace, equivalent to
      \code
      R.update_value_from_change(current_index, R)
      \endcode
      \param current_index echoed to stream_iterator::update_value_from_change(int)
      \param R stream_iterator object calling stream_iterator::update_value_from_change(int)
     */
    inline
    bool
    update_value_from_change(int current_index, stream_iterator &R)
    {
      return R.update_value_from_change(current_index);
    }

    /*!\fn float init_stream_iterator(const WRATHStateStream&, int, stream_iterator&)
      Provided as a conveninace, equivalent to
      \code
      R.init_stream_iterator(state_stream, start_index);
      \endcode
      \param state_stream echoed to \ref stream_iterator::init_stream_iterator(const WRATHStateStream&, int)
      \param start_index echoed to \ref stream_iterator::init_stream_iterator(const WRATHStateStream&, int)
      \param R stream_iterator object calling \ref stream_iterator::init_stream_iterator(const WRATHStateStream&, int)
     */
    inline
    float
    init_stream_iterator(const WRATHStateStream &state_stream,
                         int start_index, 
                         stream_iterator &R)
    {
      return R.init_stream_iterator(state_stream, start_index);
    }

    /*!\fn float sub_range(int, stream_iterator&)
      Provided as a conveninace, equivalent to
      \code
      R.sub_range(start_index);
      \endcode
      \param start_index echoed to \ref stream_iterator::sub_range(int)
      \param R stream_iterator object calling \ref stream_iterator::sub_range(int)
     */
    inline
    float
    sub_range(int start_index, stream_iterator &R)
    {
      return R.sub_range(start_index);
    }

  }

  /*!\enum color_corner_bits
    Enumeration bit fields to specify to what corners
    a color change is to apply.
   */
  enum color_corner_bits
    {
      /*!
        bit to indicate to affect
        bottom_left corner
       */
      bottom_left_corner_bit=1,

      /*!
        bit to indicate to affect
        bottom_right corner
       */
      bottom_right_corner_bit=2,

      /*!
        bit to indicate to affect
        top_left corner
       */
      top_left_corner_bit=4,

      /*!
        bit to indicate to affect
        top_right corner
       */
      top_right_corner_bit=8,

      /*!
        conveniance, bits for both top corners up
       */
      top_corner_bits=top_right_corner_bit|top_left_corner_bit,

      /*!
        conveniance, bits for both bottom corners up
       */
      bottom_corner_bits=bottom_left_corner_bit|bottom_right_corner_bit,

      /*!
        conveniance, bits for both left corners up
       */
      left_corner_bits=top_left_corner_bit|bottom_left_corner_bit,

      /*!
        conveniance, bits for both right corners up
       */
      right_corner_bits=top_right_corner_bit|bottom_right_corner_bit,

      /*!
        conveniance, bits for all corners up
       */
      all_corner_bits=bottom_corner_bits|top_corner_bits
    };

  
  /*!\class set_colors_type
    Manipulator" to set color for multiple corners.
   */
  class set_colors_type
  {
  public:
    /*!\fn set_colors_type
      Ctor.
      \param C value to which to initialize \ref m_value
      \param bits value to which to initialize \ref m_bits
     */
    set_colors_type(color_type C,
                    uint32_t bits):
      m_value(C),
      m_bits(bits)
    {}

    /*!\var m_value
      color value to apply
     */
    color_type m_value;

    /*!\var m_bits
      bit field indicating to which corners
      to apply the color, see \ref color_corner_bits
     */
    uint32_t m_bits;
  };

  /*!\class push_colors_type
    Manipulator" to push color for multiple corners.
   */
  class push_colors_type
  {
  public:
    /*!\fn push_colors_type
      Ctor.
      \param C value to which to initialize \ref m_value
      \param bits value to which to initialize \ref m_bits
     */
    push_colors_type(color_type C,
                     uint32_t bits):
      m_value(C),
      m_bits(bits)
    {}

    /*!\var m_value
      color value to apply
     */
    color_type m_value;

    /*!\var m_bits
      bit field indicating to which corners
      to apply the color, see \ref color_corner_bits
     */
    uint32_t m_bits;
  };

  /*!\class pop_colors_type
    Manipulator" to pop color for multiple corners.
   */
  class pop_colors_type
  {
  public:
    /*!\fn pop_colors_type
      Ctor.
      \param bits value to which to initialize \ref m_bits
     */
    explicit 
    pop_colors_type(uint32_t bits):
      m_bits(bits)
    {}

    /*!\var m_bits
      bit field indicating to which corners
      to apply the color pop, see \ref color_corner_bits
     */
    uint32_t m_bits;
  };

  /*!\class get_color_type
    Manipulator to get the color
    of a specific corner.
   */
  class get_color_type
  {
  public:
    /*!\fn get_color_type
      Ctor.
      \param ptarget value to which to initialize \ref m_target
      \param bit value to which to initialize \ref m_bit
     */
    explicit
    get_color_type(color_type &ptarget, uint32_t bit):
      m_target(ptarget),
      m_bit(bit)
    {}

    /*!\var m_target
      location to which to write query result
     */
    color_type &m_target;

    /*!\var m_bit
      bit indicating which corner to query.
      Exactly one bit from \ref color_corner_bits
      should be up.
     */
    uint32_t m_bit;
  };

  /*!\fn set_colors_type set_color(color_type, uint32_t)
    "Manipulator" to set the color.
    \param C color value to apply
    \param corner_bits bit field indicating to which corners
                       to apply the color, see \ref color_corner_bits
   */
  inline
  set_colors_type
  set_color(color_type C, uint32_t corner_bits=all_corner_bits)
  {
    return set_colors_type(C, corner_bits);
  }

  /*!\fn set_colors_type set_color(const vec4&, uint32_t)
    "Manipulator" to set the color.
    \param C normalized color vaue
    \param corner_bits bit field indicating to which corners
                       to apply the color, see \ref color_corner_bits
   */
  inline
  set_colors_type
  set_color(const vec4 &C, uint32_t corner_bits=all_corner_bits)
  {
    color_type uC;

    uC.x()=static_cast<GLubyte>(255.0f*std::max(0.0f, std::min(1.0f, C.x())));
    uC.y()=static_cast<GLubyte>(255.0f*std::max(0.0f, std::min(1.0f, C.y())));
    uC.z()=static_cast<GLubyte>(255.0f*std::max(0.0f, std::min(1.0f, C.z())));
    uC.w()=static_cast<GLubyte>(255.0f*std::max(0.0f, std::min(1.0f, C.w())));

    return set_color(uC, corner_bits);
  }
  
  /*!\fn set_colors_type set_color(GLubyte, GLubyte, GLubyte, GLubyte, uint32_t)
    "Manipulator" to set the color.
    \param r red value
    \param g green value
    \param b blue value
    \param a alpha value
    \param corner_bits bit field indicating to which corners
                       to apply the color, see \ref color_corner_bits
   */
  inline
  set_colors_type
  set_color(GLubyte r, GLubyte g, GLubyte b, GLubyte a=0xFF, 
            uint32_t corner_bits=all_corner_bits)
  {
    return set_color(color_type(r,g,b,a), corner_bits);
  }

  /*!\fn push_colors_type push_color(color_type, uint32_t)
    "Manipulator" to push the color.
    \param C color value to apply
    \param corner_bits bit field indicating to which corners
                       to apply the color, see \ref color_corner_bits
   */
  inline
  push_colors_type
  push_color(color_type C, uint32_t corner_bits=all_corner_bits)
  {
    return push_colors_type(C, corner_bits);
  }
  
  /*!\fn push_colors_type push_color(GLubyte, GLubyte, GLubyte, GLubyte, uint32_t)
    "Manipulator" to push the color.
    \param r red value
    \param g green value
    \param b blue value
    \param a alpha value
    \param corner_bits bit field indicating to which corners
                       to apply the color, see \ref color_corner_bits
   */
  inline
  push_colors_type
  push_color(GLubyte r, GLubyte g, GLubyte b, GLubyte a=0xff, 
             uint32_t corner_bits=all_corner_bits)
  {
    return push_color(color_type(r,g,b,a), corner_bits);
  }
  
  /*!\fn get_color_type get_color(color_type&, int)
    "Manipulator" to get the color.
    \param ptarget location to place value
    \param bit bit indicating which corner to query.
                   Exactly one bit from \ref color_corner_bits
                   should be up.
   */
  inline
  get_color_type
  get_color(color_type &ptarget, int bit=bottom_left_corner_bit)
  {
    return get_color_type(ptarget, bit);
  }

  /*!\fn pop_colors_type pop_color(uint32_t)
    "Manipulator" to pop the color.
    \param corner_bits bit field indicating to which corners
                       to pop the color, see \ref color_corner_bits
   */
  inline
  pop_colors_type
  pop_color(uint32_t corner_bits=all_corner_bits)
  {
    return pop_colors_type(corner_bits);
  }


  

};

                
/*!\fn WRATHStateStream& operator<<(WRATHStateStream &, const WRATHText::set_colors_type&);
  Function returning a manipulator to set the color of letters
  with a \ref WRATHText::set_colors_type
  \param stream WRATHStateStream to affect
  \param sc color value setter
 */ 
WRATHStateStream&
operator<<(WRATHStateStream &stream, const WRATHText::set_colors_type &sc);

/*!\fn WRATHStateStream& operator<<(WRATHStateStream &, const WRATHText::push_colors_type&);
  Function returning a manipulator to push the color of letters
  with a \ref WRATHText::set_colors_type
  \param stream WRATHStateStream to affect
  \param sc color value pusher
 */ 
WRATHStateStream&
operator<<(WRATHStateStream &stream, const WRATHText::push_colors_type &sc);

/*!\fn WRATHStateStream& operator<<(WRATHStateStream &, const WRATHText::pop_colors_type&);
  Function returning a manipulator to pop the color of letters
  with a \ref WRATHText::pop_colors_type
  \param stream WRATHStateStream to affect
  \param sc color value popper
 */ 
WRATHStateStream&
operator<<(WRATHStateStream &stream, const WRATHText::pop_colors_type &sc);

/*!\fn WRATHStateStream& operator<<(WRATHStateStream &, const WRATHText::get_color_type&);
  Function returning a manipulator to get the color of letters
  with a \ref WRATHText::get_color_type
  \param stream WRATHStateStream to affect
  \param sc color value getter
 */ 
WRATHStateStream&
operator<<(WRATHStateStream &stream, const WRATHText::get_color_type &sc);




/*! @} */

#endif
