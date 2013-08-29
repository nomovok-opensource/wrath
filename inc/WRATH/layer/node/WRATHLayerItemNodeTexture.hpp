/*! 
 * \file WRATHLayerItemNodeTexture.hpp
 * \brief file WRATHLayerItemNodeTexture.hpp
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


#ifndef __WRATH_LAYER_ITEM_TEXTURE_HPP__
#define __WRATH_LAYER_ITEM_TEXTURE_HPP__

#include "WRATHConfig.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHTextureCoordinate.hpp"
#include "WRATHTextureCoordinateDynamic.hpp"
#include "WRATHLayerItemNodeFunctionPacketT.hpp"


/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerItemNodeTextureT
  A WRATHLayerItemNodeTexture is
  a generic class that adds texture
  coordinate data
  \tparam T node type
  \tparam texture_type type that hold texture coordinate information,
                       for now must be derived from WRATHTextureCoordinate
  \tparam default_boundary_crop_x default value to use for on_image_data_boundary_crop_x
                                  in methods sub_image() and full_image()
  \tparam default_boundary_crop_y default value to use for on_image_data_boundary_crop_y
                                  in methods sub_image() and full_image()

  \n\n The node type T must:
  - inherit from WRATHLayerItemNodeBase
  - define the static function const node_function_packet& functions(void)
  - the return value of the virtual function node_functions() must be the same as functions()
  - must define the enumeration number_per_node_values indicating the number of per-item
    uniforms it uses
 */
template<typename T, class texture_type, 
         bool default_boundary_crop_x,
         bool default_boundary_crop_y>
class WRATHLayerItemNodeTextureT:
  public T,
  public texture_type
{
public:

  enum
    {
      /*!
        Enumeration of the number of per-node values
        from the base class T
       */
      base_number_per_node_values=T::number_per_node_values,

      /*!
        Enumeration indicating number of per-node values
        a WRATHLayerItemNodeTexture has
       */
      number_per_node_values=T::number_per_node_values+texture_type::number_per_node_values
    };

  /*!\fn WRATHLayerItemNodeTextureT(const WRATHTripleBufferEnabler::handle&)
    Ctor to create a root WRATHLayerItemNodeTextureT.
    \param r handle to WRATHTripleBufferEnabler to coordinate triple buffering
   */
  explicit
  WRATHLayerItemNodeTextureT(const WRATHTripleBufferEnabler::handle &r):
    T(r)
  {}

  /*!\fn WRATHLayerItemNodeTextureT(S*)
    Ctor to create a child WRATHLayerItemNodeTextureT,
    parent takes ownership of child.
    \param pparent parent node object
   */
  template<typename S>
  explicit
  WRATHLayerItemNodeTextureT(S *pparent):
    T(pparent),
    m_image(NULL)
  {}

  /*!\fn void sub_image
    Set the node to use the sub-portion of the portion of the WRATHImage to which it is linked
    \param xy (left,bottom) in pixels relative to WRATHImage
    \param wh (width,height) in pixels relative to WRATHImage
    \param on_image_data_boundary_crop_x for each vertical side of the image (left and right side)
                                         if the WRATHImage does not have a pixel boundary data
                                         (see \ref WRATHImage::boundary_size()), crop the 
                                         image by 1 pixel. This is only needed for when the
                                         repeat mode in the x-direction is \ref WRATHTextureCoordinate::repeat
    \param on_image_data_boundary_crop_y for each horizontal side of the image (top and bottom side)
                                         if the WRATHImage does not have a pixel boundary data
                                         (see \ref WRATHImage::boundary_size()), crop the 
                                         image by 1 pixel. This is only needed for when the
                                         repeat mode in the y-direction is \ref WRATHTextureCoordinate::repeat
   */
  void
  sub_image(const ivec2 &xy, const ivec2 &wh, 
            bool on_image_data_boundary_crop_x=default_boundary_crop_x,
            bool on_image_data_boundary_crop_y=default_boundary_crop_y)
  {
    texture_type::set(m_image, xy, wh, 
                      on_image_data_boundary_crop_x,
                      on_image_data_boundary_crop_y);
  }

  /*!\fn void full_image
    Set the node to use the entire WRATHImage to which it is linked
    \param on_image_data_boundary_crop_x for each vertical side of the image (left and right side)
                                         if the WRATHImage does not have a pixel boundary data
                                         (see \ref WRATHImage::boundary_size()), crop the 
                                         image by 1 pixel. This is only needed for when the
                                         repeat mode in the x-direction is \ref WRATHTextureCoordinate::repeat
    \param on_image_data_boundary_crop_y for each horizontal side of the image (top and bottom side)
                                         if the WRATHImage does not have a pixel boundary data
                                         (see \ref WRATHImage::boundary_size()), crop the 
                                         image by 1 pixel. This is only needed for when the
                                         repeat mode in the y-direction is \ref WRATHTextureCoordinate::repeat
   */
  void
  full_image(bool on_image_data_boundary_crop_x=default_boundary_crop_x,
             bool on_image_data_boundary_crop_y=default_boundary_crop_y)
  {
    texture_type::set(m_image, 
                      on_image_data_boundary_crop_x,
                      on_image_data_boundary_crop_y);
  }

  /*!\fn const WRATHLayerItemNodeBase::node_function_packet& node_functions()
    Implements WRATHLayerItemNodeBase::node_functions() 
   */
  virtual
  const WRATHLayerItemNodeBase::node_function_packet&
  node_functions(void) const
  {
    return functions();
  }

  /*!\fn const WRATHLayerItemNodeBase::node_function_packet& functions()
    Returns same value as node_functions()
   */
  static
  const WRATHLayerItemNodeBase::node_function_packet&
  functions(void)
  {
    WRATHStaticInit();
    static WRATHLayerItemNodeFunctionPacketT<T, texture_type> R;
    return R;
  }

  /*!\fn void extract_values(reorder_c_array<float>)
    Implements WRATHLayerItemNodeBase::extract_values(reorder_c_array<float>)
    \param out_value location to which to write per-node values
   */
  virtual
  void
  extract_values(reorder_c_array<float> out_value)
  {
    T::extract_values(out_value.sub_array(0, T::number_per_node_values));
    texture_type::extract_values_at(T::number_per_node_values, out_value);
  }
  
  /*!\fn void set_shader_brush
    Sets the texture coordinate shader code 
    (\ref WRATHShaderBrush::m_texture_coordinate_source)
    as the source to compute texture coordinates
    \param brush WRATHShaderBrush to set
   */ 
  static
  void
  set_shader_brush(WRATHShaderBrush &brush)
  {
    T::set_shader_brush(brush);
    brush.m_texture_coordinate_source=texture_type::source();
  }

  /*!\fn void set_from_brush
    Sets the node value that stores the
    various texture coordinate values. If the
    value of \ref WRATHBrush::m_image is different
    than before, then also calls full_image() to
    use the entire image
    \param brush \ref WRATHBrush from which to use \ref WRATHBrush::m_image
   */
  virtual
  void
  set_from_brush(const WRATHBrush &brush)
  {
    T::set_from_brush(brush);
    if(m_image!=brush.m_image)
      {
        m_image=brush.m_image;
        full_image();
      }
  }

private:
  WRATHImage *m_image;
};


/*
  Sighs:
  - typedef does not work with templates
  - Doing type<T, X, Y>::Type is ugly
 */

/*!\class WRATHLayerItemNodeTexture
  Node class that uses fixed repeat modes
  for texture coordinate generation.
  Provided as a conveniance, equivalent to
  WRATHLayerItemNodeTextureT<T, WRATHTextureCoordinateT<X, Y>, X==WRATHTextureCoordinate::repeat, Y==WRATHTextureCoordinate::repeat>

  \tparam T node type
  \tparam X repeat mode in x-direction
  \tparam Y repeat mode in y-direction
 */
template<typename T, 
         enum WRATHTextureCoordinate::repeat_mode_type X=WRATHTextureCoordinate::repeat,
         enum WRATHTextureCoordinate::repeat_mode_type Y=WRATHTextureCoordinate::repeat>
class WRATHLayerItemNodeTexture:
  public WRATHLayerItemNodeTextureT<T, 
                                    WRATHTextureCoordinateT<X, Y>,
                                    X==WRATHTextureCoordinate::repeat,
                                    Y==WRATHTextureCoordinate::repeat>
{
public:
  /*!\typedef base_class
    Conveniance typedef
   */
  typedef WRATHLayerItemNodeTextureT<T, 
                                     WRATHTextureCoordinateT<X, Y>,
                                     X==WRATHTextureCoordinate::repeat,
                                     Y==WRATHTextureCoordinate::repeat> base_class;
  
  /*!\fn WRATHLayerItemNodeTexture(const WRATHTripleBufferEnabler::handle&)
    Ctor to create a root WRATHLayerItemNodeTexture.
    \param r handle to WRATHTripleBufferEnabler to coordinate triple buffering
   */
  explicit
  WRATHLayerItemNodeTexture(const WRATHTripleBufferEnabler::handle &r):
    base_class(r)
  {
  }

  /*!\fn WRATHLayerItemNodeTexture(S*)
    Ctor to create a child WRATHLayerItemNodeTexture,
    parent takes ownership of child.
    \param pparent parent node object
   */
  template<typename S>
  explicit
  WRATHLayerItemNodeTexture(S *pparent):
    base_class(pparent)
  {
  }
};

/*!\class WRATHLayerItemNodeTextureDynamic
  Node class that uses repeat mode
  that can change dynamically.
  Provided as a conveniance, equivalent to
  WRATHLayerItemNodeTextureT<T, WRATHTextureCoordinateDynamic>
  \tparam T node type
 */
template<typename T>
class WRATHLayerItemNodeTextureDynamic:
  public WRATHLayerItemNodeTextureT<T, WRATHTextureCoordinateDynamic, true, true>
{
public:
  /*!\typedef base_class
    Conveniance typedef
   */
  typedef WRATHLayerItemNodeTextureT<T, WRATHTextureCoordinateDynamic, true, true> base_class;

  /*!\fn WRATHLayerItemNodeTextureDynamic(const WRATHTripleBufferEnabler::handle&)
    Ctor to create a root WRATHLayerItemNodeTextureDynamic.
    \param r handle to WRATHTripleBufferEnabler to coordinate triple buffering
   */
  explicit
  WRATHLayerItemNodeTextureDynamic(const WRATHTripleBufferEnabler::handle &r):
    base_class(r)
  {
  }

  /*!\fn WRATHLayerItemNodeTextureDynamic(S*)
    Ctor to create a child WRATHLayerItemNodeTextureDynamic,
    parent takes ownership of child.
    \param pparent parent node object
   */
  template<typename S>
  explicit
  WRATHLayerItemNodeTextureDynamic(S *pparent):
    base_class(pparent)
  {
  }  
};


/*! @} */




#endif
