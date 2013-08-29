/*! 
 * \file WRATHWidgetEnums.hpp
 * \brief file WRATHWidgetEnums.hpp
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


#ifndef __WRATH_WIDGET_ENUMS_HPP__
#define __WRATH_WIDGET_ENUMS_HPP__

#include "WRATHConfig.hpp"
#include "WRATHDrawType.hpp"


/*! \addtogroup Widgets
 * @{
 */

/*!\namespace WRATHWidgetEnums
  Namespace to hold a variety of enumerations 
  for the WRATHWidget framework
 */
namespace WRATHWidgetEnums
{
  /*!\enum widget_clipping_t
    Enumeration that specifies if a widget's clipping
    relies on quad-quad clipping or has generic 
    clipping. Quad-Quad clipping uses processing
    in the vertex shader to clip a quad to another
    quad where the quads are parallel. The latter uses
    "real" clipping via clip-planes and can operate
    on arbitary primitives. Unextended GLES2 does 
    NOT explose user defined clip-planes and as such
    to do that requires executing discard on the fragment
    shader.
   */
  enum widget_clipping_t
    {
      /*!
        Clipping is handled via vertex shader,
        the primitives must be essentially
        quads where the attribute data has
        a sufficient global picture of the quad
        so that the vertex shader can clip
        the quad
       */
      widget_quad_clipping,

      /*!
        Clipping is unrestricted, i.e. clipping
        is implemented via hardware user
        define clipping planes or via discard.
       */
      widget_generic_clipping,
    };
  
  /*!\enum node_type_bits
    Enumeration bit values indicating
    with what data tp augment a basic
    node type.
   */
  enum node_type_bits
    {
      /*!
        Bit up indicates with linear
        gradient positional data added
        Cannot have this bit and
        \ref radial_gradient up at
        the same time
       */
      linear_gradient=1,

      /*!
        Bit up indicates with radial
        gradient positional data added
        Cannot have this bit and
        \ref linear_gradient up at
        the same time
       */
      radial_gradient=2,

      /*!
        Bit up indiates gradient with
        window position so that gradient
        is repeated according to the window.
        Requires that exactly one of 
        linear_gradient_node or
        radial_gradient_node is up
      */
      gradient_repeat=4,

      /*!
        Indicates linear repeat gradient, i.e.
        just \ref linear_gradient| \ref gradient_repeat
       */
      linear_repeat_gradient=linear_gradient|gradient_repeat,

      /*!
        Indicates radial repeat gradient, i.e.
        just \ref radial_gradient| \ref gradient_repeat
       */
      radial_repeat_gradient=radial_gradient|gradient_repeat,
      
        
      /*!
        indicates a with color type added
       */
      color=8,

      /*!
        indicates with texture coordinate added
        so that repeat mode is dynamic,
        i.e. changeable duing the lifetime
        of the item
      */
      image=16,
    };

  /*!\class node_type
    A particularly ugly template class so that one can use 
    bitwise ors from \ref node_type_bits to create node types,
    for example
    \code
    node_type<T, N, image|gradient_repeat|linear_gradient>::type
    \endcode
    defines a node type N with the additions of having
    a texture coordinate, linear gradient which is repeated.

    \tparam T needs to define the type T<node, I>::type
            where I is where exactly _one_ bit of node_type_bits
            is up so that T<node,I>::type is the node type node with
            the attached data. 
    \tparam N base node type
    \tparam up_bits bits defined \ref node_type_bits specifying the data to add
    \tparam test_bit leave this as default, unless you want an unpleasant surprise
   */
  template<typename N, 
           template<typename Node, uint32_t bit> class T, 
           uint32_t up_bits, uint32_t test_bit=image>
  class node_type
  {
  public:
    /*!\typedef base_node_type
      Local typedef for the node type
      with no additional information attached.
     */
    typedef typename node_type<N, T, up_bits, (test_bit>>1) >::base_node_type base_node_type;

    /*!\typedef base_type
      The immediate base class to \ref type; view
      the class definition as a template implementation
      detail.
     */
    typedef typename node_type<N, T, up_bits, (test_bit>>1) >::type base_type; 

    /*!\typedef type
      Node type augmented by the bitfield template parameter up_bits
     */
    typedef typename T<base_type, up_bits&test_bit>::type type; 
  };

  /// @cond
  template<typename N, 
           template<typename Node, uint32_t bit> class T, 
           uint32_t up_bits>
  class node_type<N, T, up_bits, 0>
  {
  public:
    typedef N base_node_type;
    typedef N base_type;
    typedef N type;
  };
  /// @endcond

  /*!\enum canvas_clip_t
    Enumeration specifying how a drawn item clips
    a canvas. A canvas has _2_ clipping regions
    associated to it: a clip inside region I
    and a clip outside region O. Items places within
    the canvas must be within the region I and 
    outside the region O.
   */
  enum canvas_clip_t
    {
      /*!
        Item adds to clipping inside region, I.
       */
      clip_inside=WRATHDrawType::clip_inside_draw,

      /*!
        Item adds to clipping outside region, O.
       */
      clip_outside=WRATHDrawType::clip_outside_draw,
    };

  /*!\fn enum WRATHDrawType::draw_type_t convert_type(enum canvas_clip_t t) 
    Conveniance function to convert a \ref canvas_clip_t
    to a WRATHDrawType::draw_type_t
    \param t value from which to convert
   */
  inline
  enum WRATHDrawType::draw_type_t
  convert_type(enum canvas_clip_t t)
  {
    WRATHassert(t==clip_inside or t==clip_outside);
    return static_cast<enum WRATHDrawType::draw_type_t>(t);
  }
  
}

/*! @} */

#endif
