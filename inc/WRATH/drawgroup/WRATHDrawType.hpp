/*! 
 * \file WRATHDrawType.hpp
 * \brief file WRATHDrawType.hpp
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




#ifndef WRATH_HEADER_DRAW_TYPE_HPP_
#define WRATH_HEADER_DRAW_TYPE_HPP_


/*! \addtogroup Group
 * @{
 */

#include "WRATHConfig.hpp"
#include <limits>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

/*!\class WRATHDrawType
  A WRATHDrawType is meta-data 
  within a \ref WRATHItemDrawState, its value
  has no effect on the draw_element() of
  a \ref WRATHRawDrawDataElement but is used
  by a WRATHCanvas derived object to place
  it within a particular \ref WRATHRawDrawData
  object. Typically, it's main
  use is to distinguish between opaque
  and transparent items as such items
  should be drawn in seperate pass phases,
  i.e. elements of different values of
  \ref WRATHDrawType are in different 
  \ref WRATHRawDrawData objects.
 */
class WRATHDrawType
{
public:

  /*!\enum draw_type_t
    Enumeration specifying the nature
    of a draw call.
   */
  enum draw_type_t
    {
      /*!
        Item to be drawn is used to
        draw a region for which contents
        of a WRATHCanvas are to be clipped
        to the inside of the region (i.e.
        where the items of a WRATHCanvas 
        are drawn).
       */
      clip_inside_draw,

      /*!
        Item to be drawn is used to
        draw a region for which contents
        of a WRATHCanvas are to be clipped
        to the outside of the region (i.e.
        where the items of a WRATHCanvas
        are NOT drawn).        
       */
      clip_outside_draw,

      /*!
        Item is to be drawn to color
        as opaque. Understood as
        depth test on, depth wrties on
        and blending off.
       */
      opaque_draw,

      /*!
        Item is to be drawn to color
        as transparent. Understood as
        depth test on, depth wrties off
        and blending on. Note that one
        will need to set the blending
        function state for items, i.e by adding
        a \ref WRATHGLStateChange::blend_state
        to the state vector.
      */
      transparent_draw,

      /*!
        An invalid enumeration value, used
        to indicate number of enumeration types
        to make template coding easier.
       */
      number_draw_types,
    };


  /*!\var m_type
    Specifies the nature of the draw:
    transparent, opaque, etc.
   */
  enum draw_type_t m_type;

  /*!\var m_value
    Draw order of the pass
   */
  int m_value; 
  
  /*!\fn WRATHDrawType
    Ctor setting m_value.
    \param v value to which to set \ref m_value.
    \param ptype value to which to set \ref m_type
   */
  explicit
  WRATHDrawType(int v=0, enum draw_type_t ptype=opaque_draw):
    m_type(ptype),
    m_value(v)
  {}

  /*!\fn bool operator<(const WRATHDrawType &rhs) const
    Comparison operator.
   */
  bool
  operator<(const WRATHDrawType &rhs) const
  {
    return (m_type<rhs.m_type) or (m_type==rhs.m_type and m_value<rhs.m_value);
  }

  /*!\fn bool operator==(const WRATHDrawType &rhs) const
    Comparison operator.
   */
  bool
  operator==(const WRATHDrawType &rhs) const
  {
    return m_type==rhs.m_type
      and m_value==rhs.m_value;
  }

  /*!\fn bool operator!=(const WRATHDrawType &rhs) const 
    Comparison operator.
   */
  bool
  operator!=(const WRATHDrawType &rhs) const
  {
    return m_type!=rhs.m_type
      or m_value!=rhs.m_value;
  }

  /*!\fn WRATHDrawType opaque_pass
    Convenience function, equivalent
    to 
    \code
    WRATHDrawType(sub_pass, opaque_draw);
    \endcode
    \param sub_pass value to which to assign \ref m_value
   */
  static
  WRATHDrawType
  opaque_pass(int sub_pass=0)
  {
    return WRATHDrawType(sub_pass, opaque_draw);
  }

  /*!\fn WRATHDrawType transparent_pass
    Convenience function, equivalent
    to 
    \code
    WRATHDrawType(sub_pass, transparent_draw);
    \endcode
    \param sub_pass value to which to assign \ref m_value
   */
  static
  WRATHDrawType
  transparent_pass(int sub_pass=0)
  {
    return WRATHDrawType(sub_pass, transparent_draw);
  }
};

/*! @} */



#endif
