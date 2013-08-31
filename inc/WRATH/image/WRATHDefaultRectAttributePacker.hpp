/*! 
 * \file WRATHDefaultRectAttributePacker.hpp
 * \brief file WRATHDefaultRectAttributePacker.hpp
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




#ifndef __WRATH_DEFAULT_IMAGE_ATTRIBUTE_PACKER_HPP__
#define __WRATH_DEFAULT_IMAGE_ATTRIBUTE_PACKER_HPP__

#include "WRATHConfig.hpp"
#include "WRATHShaderSpecifier.hpp"
#include "WRATHRectAttributePacker.hpp"
#include "WRATHBaseSource.hpp"
#include "WRATHGradientSourceBase.hpp"

/*! \addtogroup Imaging
 * @{
 */


/*!\class WRATHDefaultRectAttributePacker
  A WRATHDefaultRectAttributePacker is an
  example of a WRATHRectAttributePacker.
  It supports texturing by exactly one 
  texture.
 */
class WRATHDefaultRectAttributePacker:public WRATHRectAttributePacker
{
public:

  /*!\class Rect
    Rect is the rectangle type that WRATHDefaultRectAttributePacker
    accepts.
   */
  class Rect:public WRATHReferenceCountedObjectT<Rect>
  {
  public:
    /*!\fn Rect(const vec2&, float)
      Ctor.
      \param pwh width and height 
      \param z perspective z-coordinate
     */
    explicit
    Rect(const vec2 &pwh=vec2(0.0f, 0.0f), 
         float z=-1.0f):
      m_width_height(pwh),
      m_z(z),
      m_brush_offset(0.0f, 0.0f),
      m_brush_stretch(1.0f, 1.0f)
    {}

    /*!\fn Rect(float, float, float)
      Ctor.
      \param w width 
      \param h height 
      \param z perspective z-coordinate
     */
    Rect(float w, float h, float z=-1.0f):
      m_width_height(w,h),
      m_z(z),
      m_brush_offset(0.0f, 0.0f),
      m_brush_stretch(1.0f, 1.0f)
    {}

    /*!\var m_width_height
      Dimension of rectangle
      - .x() is width
      - .y() is height
     */
    vec2 m_width_height;

    /*!\var m_z
      Z coordinate to feed to projection.
     */
    float m_z;

    /*!\var m_brush_offset
      The position fed to the brush is given
      by:\n
      \ref m_brush_offset + \ref m_brush_stretch*p\n
      where p is in item coordinates. Initial
      value is (0,0).
     */
    vec2 m_brush_offset;

    /*!\var m_brush_stretch
      The position fed to the brush is given
      by:\n
      \ref m_brush_offset + \ref m_brush_stretch*p\n
      where p is in item coordinates.Initial
      value is (1,1).
     */
    vec2 m_brush_stretch;
  };

  enum 
    {
      /*!
        location of position data,
        packed. Attribute name in GLSL 
        is "size_and_z"
        - .xy width-height of the rectangle
        - .z z-coordinate for projective drawing
      */
      size_and_z_location=0, 

      /*!
        location for brush position data, GLSL name is
        "brush"
        - .xy = position offset, see \ref Rect::m_brush_offset
        - .zw = stretch factor, see \ref Rect::m_brush_stretch
       */
      brush_position_stretch_location=1,

      /*!
        normalized coordinate indicates
        where on the quad the vertex is,
        a vec2 in GLSL.
        Attribute name in GLSL is "normalized_coordinate".
       */
      normalized_location=2,
    };

  /*!\fn WRATHDefaultRectAttributePacker* fetch 
    WRATHDefaultRectAttributePacker is stateless and
    only at most one WRATHDefaultRectAttributePacker
    needs to exist. The function fetch(), if necessary
    contructs the WRATHDefaultRectAttributePacker, and
    then returns a pointer to it.
   */
  static
  WRATHDefaultRectAttributePacker*
  fetch(void)
  {
    return WRATHAttributePacker::fetch_make<WRATHDefaultRectAttributePacker>(Factory());
  }

  /*!\fn Rect::handle rect_properties(float, float, float)
    Provided as a conveniance, equivalent to
    \code
    WRATHNew Rect(w, h, z);
    \endcode
    \param w width 
    \param h height 
    \param z perspective z-coordinate
   */
  static
  Rect::handle
  rect_properties(float w, float h, float z=-1.0f)
  {
    return WRATHNew Rect(w, h, z);
  }

  /*!\fn Rect::handle rect_properties(const vec2&, float)
    Provided as a conveniance, equivalent to
    \code
    WRATHNew Rect(pwh, z);
    \endcode
    \param pwh width and height 
    \param z perspective z-coordinate
   */
  static
  Rect::handle
  rect_properties(const vec2 &pwh=vec2(0.0f, 0.0f), float z=-1.0f)
  {
    return WRATHNew Rect(pwh, z);
  }

  virtual
  void
  attribute_key(WRATHAttributeStoreKey &attrib_key) const;


protected:

  virtual
  void
  set_attribute_data_implement(WRATHAbstractDataSink &sink,
                               int attr_location,
                               const WRATHReferenceCountedObject::handle &rect,
                               const WRATHStateBasedPackingData::handle &h) const;

  
private:

  class Factory:public WRATHAttributePacker::AttributePackerFactory
  {
  public:
    virtual
    WRATHAttributePacker*
    create(void) const 
    {
      return WRATHNew WRATHDefaultRectAttributePacker();
    }
  };

  WRATHDefaultRectAttributePacker(void);
                              
};


/*! @} */

#endif
