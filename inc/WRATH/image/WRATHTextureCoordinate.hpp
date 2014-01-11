/*! 
 * \file WRATHTextureCoordinate.hpp
 * \brief file WRATHTextureCoordinate.hpp
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


#ifndef __WRATH_TEXTURE_COORDINATE_NORMALIZED_HPP__
#define __WRATH_TEXTURE_COORDINATE_NORMALIZED_HPP__


#include "WRATHConfig.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHImage.hpp"
#include "WRATHTextureCoordinateSourceBase.hpp"


/*! \addtogroup Imaging
 * @{
 */

/*!\class WRATHTextureCoordinate
  A WRATHTextureCoordinate holds 
  the necessary value to transform a normalized
  coordinate of a sub-rectangle of a texture
  into a normalized coordinate of the texture.
  The main use case is for getting the color
  values from a WRATHImage. Gives the following
  per-node values to be extract from the
  shader code with fetch_node_value(X):
  - WRATH_TEXTURE_subrect_x left side of rect in normalized texture coordinates
  - WRATH_TEXTURE_subrect_y bottom side of rect in normalized texture coordinates
  - WRATH_TEXTURE_subrect_w the width of rect in normalized texture coordinates
  - WRATH_TEXTURE_subrect_h the height of rect in normalized texture coordinates

  Of critial importance: the values stored are normalized texture
  coordinates *NOT* pixel coordinates.
 */
class WRATHTextureCoordinate
{
public:
  
  enum
    {
      /*!
        Enumeration specifyig how many
        per node values needed to store
        the packed data of a 
        WRATHTextureCoordinate object
       */
      number_per_node_values=4
    };

  /*!\enum repeat_mode_type  
    Enumeration for image_source()
    to specify the repeat mode to
    use for the sub-rectangle
   */
  enum repeat_mode_type
    {
      /*!
        Assume the input normalized coordinates
        given are always within the range [0,1]
       */
      simple,

      /*!
        Do not assume the input normalized coordinates
        given are always within the range [0,1] and
        clamp the value
       */
      clamp,

      /*!
        Do not assume the input normalized coordinates
        given are always within the range [0,1] and
        use the fractional part of the value
       */
      repeat,

      /*!
        Do not assume the input normalized coordinates
        given are always within the range [0,1] and
        perform mirror repeat
       */
      mirror_repeat,

      /*!
        conveniance enumeration stating
        number of repeat mode types
       */
      number_modes
    };

  /*!\fn WRATHTextureCoordinate(void)
    Ctor. Initializes the object to use the entire texture
   */
  WRATHTextureCoordinate(void):
    m_minx_miny(0.0f, 0.0f),
    m_wh(1.0f, 1.0f)
  {}

  virtual
  ~WRATHTextureCoordinate()
  {}

  /*!\fn void set(const vec2&, const vec2&)
    Set the WRATHTextureCoordinate to 
    use a specific portion of a texture
    \param pminx_miny (left,bottom) in normalized texture coordinates
    \param pwh (width,height) in normalized texture coordinates
   */
  void
  set(const vec2 &pminx_miny, const vec2 &pwh)
  {
    m_minx_miny=pminx_miny;
    m_wh=pwh;
  }

  /*!\fn void set(const WRATHImage*,
                  const ivec2&, const ivec2&,
                  bool, bool)
    Set the WRATHTextureCoordinate to use
    the sub-portion of the portion of a texture occupied by a WRATHImage.
    \param image \ref WRATHImage 
    \param pminx_miny (left,bottom) in pixels relative to WRATHImage
    \param pwh (width,height) in pixels relative to WRATHImage
    \param on_image_data_boundary_crop_x for each vertical side of the image (left and right side)
                                         if the WRATHImage does not have a pixel boundary data
                                         (see \ref WRATHImage::boundary_size()), crop the 
                                         image by 1 pixel. This is only needed for when the
                                         repeat mode in the x-direction is \ref repeat
    \param on_image_data_boundary_crop_y for each horizontal side of the image (top and bottom side)
                                         if the WRATHImage does not have a pixel boundary data
                                         (see \ref WRATHImage::boundary_size()), crop the 
                                         image by 1 pixel. This is only needed for when the
                                         repeat mode in the y-direction is \ref repeat
   */
  void
  set(const WRATHImage *image, 
      const ivec2 &pminx_miny, const ivec2 &pwh, 
      bool on_image_data_boundary_crop_x,
      bool on_image_data_boundary_crop_y);

  /*!\fn void set(const WRATHImage *, bool, bool)
    Set the WRATHTextureCoordinate to use
    the the portion of a texture occupied by a WRATHImage.
    \param image \ref WRATHImage from which to extract texture-coordinates
    \param on_image_data_boundary_crop_x for each vertical side of the image (left and right side)
                                         if the WRATHImage does not have a pixel boundary data
                                         (see \ref WRATHImage::boundary_size()), crop the 
                                         image by 1 pixel. This is only needed for when the
                                         repeat mode in the x-direction is \ref repeat
    \param on_image_data_boundary_crop_y for each horizontal side of the image (top and bottom side)
                                         if the WRATHImage does not have a pixel boundary data
                                         (see \ref WRATHImage::boundary_size()), crop the 
                                         image by 1 pixel. This is only needed for when the
                                         repeat mode in the y-direction is \ref repeat
   */
  void
  set(const WRATHImage *image, 
      bool on_image_data_boundary_crop_x,
      bool on_image_data_boundary_crop_y);

  /*!\fn void extract_values_at
    Extracts the values of this WRATHTextureCoordinate
    object into a reorder_c_array<> starting at a given position
    suitable for the shader returned by gradient_source().
    \param start_index index value as passed to add_per_node_values_at()
                             to specify the per node values
    \param out_value target to which to pack the data, ala WRATHLayerItemNode::extract_values()
   */
  virtual
  void
  extract_values_at(int start_index, reorder_c_array<float> out_value);

  /*!\fn void add_per_node_values_at
    Adds the per-item values that a WRATHTextureCoordinate
    object requires for shader returned by gradient_source().
    The method places the per-node value locations at a given
    starting index.
    \param start_index starting index to specify the node value locations
    \param spec object to which to specify the node values to add, see
                WRATHLayerItemNode::node_function_packet::add_per_node_values()
    \param available specifies what stages per-node value fetching is available
   */
  static
  void
  add_per_node_values_at(int start_index,
                           WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                           const WRATHLayerNodeValuePackerBase::function_packet &available);

  /*!\fn const WRATHTextureCoordinateSourceBase* source(enum repeat_mode_type, enum repeat_mode_type)
    Return a WRATHTextureCoordinateSourceBase for the specified 
    repeat modes.
    \param repeat_mode_x specify the repeat mode in the x direction
    \param repeat_mode_y specify the repeat mode in the y direction
   */
  static
  const WRATHTextureCoordinateSourceBase*
  source(enum repeat_mode_type repeat_mode_x, 
         enum repeat_mode_type repeat_mode_y);

  /*!\fn const WRATHTextureCoordinateSourceBase* source(enum repeat_mode_type)
    Return a WRATHTextureCoordinateSourceBase where the x and y 
    repeat modes are the same. Equivalent to
    \code
    source(repeat_mode_xy, repeat_mode_xy)
    \endcode
    \param repeat_mode_xy specify the repeat mode for both the x and y directions
   */
  static
  const WRATHTextureCoordinateSourceBase*
  source(enum repeat_mode_type repeat_mode_xy)
  {
    return source(repeat_mode_xy, repeat_mode_xy);
  }

private:
  vec2 m_minx_miny, m_wh;
};


/*!\class WRATHTextureCoordinateT
  The class WRATHTextureCoordinateT is a conveniance
  class that provides a source() method taking no
  arguments; the argument value is the template
  parameter value. Think of the class as a way
  to strongly bind the repeat mode to a type
  \tparam X repeat mode in X-direction
  \tparam Y repeat mode in Y-direction
 */
template<enum WRATHTextureCoordinate::repeat_mode_type X,
         enum WRATHTextureCoordinate::repeat_mode_type Y>
class WRATHTextureCoordinateT:
  public WRATHTextureCoordinate
{
public:
  /*!\fn const WRATHTextureCoordinateSourceBase* source
    Returns the WRATHTextureCoordinateSourceBase for
    the repeat modes determined by the template arguments.
    Equivalent to
    \code
    WRATHTextureCoordinate::source(X, Y)
    \endcode
   */
  static
  const WRATHTextureCoordinateSourceBase*
  source(void)
  {
    return WRATHTextureCoordinate::source(X, Y);
  }
};

/*! @} */

#endif
