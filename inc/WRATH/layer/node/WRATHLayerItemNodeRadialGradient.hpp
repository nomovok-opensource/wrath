/*! 
 * \file WRATHLayerItemNodeRadialGradient.hpp
 * \brief file WRATHLayerItemNodeRadialGradient.hpp
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


#ifndef __WRATH_LAYER_ITEM_RADIAL_GRADIENT_HPP__
#define __WRATH_LAYER_ITEM_RADIAL_GRADIENT_HPP__

#include "WRATHConfig.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHRadialGradientValue.hpp"
#include "WRATHLayerItemNodeFunctionPacketT.hpp"

/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerItemNodeRadialGradient
  A WRATHLayerItemNodeRadialGradient is
  a generic class that adds radial
  gradient data (via inheriting from
  WRATHRadialGradientValue)
  to a pre-existing node class T.

  \n\n The node type T must:
  - inherit from WRATHLayerItemNodeBase
  - define the static function const node_function_packet& functions(void)
  - the return value of the virtual function node_functions() must be the same as functions()
  - must define the enumeration number_per_node_values indicating the number of per-node
    values it uses
  \tparam T Node type to supplement with radial gradient positional values
 */
template<typename T>
class WRATHLayerItemNodeRadialGradient:
  public T,
  public WRATHRadialGradientValue
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
        a WRATHLayerItemNodeRadialGradient has
       */
      number_per_node_values=T::number_per_node_values+WRATHRadialGradientValue::number_per_node_values
    };

  /*!\fn WRATHLayerItemNodeRadialGradient(const WRATHTripleBufferEnabler::handle&,
                                          const vec2&, float, const vec2&, float)
    Ctor to create a root WRATHLayerItemNodeRadialGradient.
    \param r handle to WRATHTripleBufferEnabler to coordinate triple buffering
    \param pstart initial value to assign to location of the start position
                  of the radial gradient 
    \param pstart_radius initial value to assign to location of the start radius
                         of the radial gradient 
    \param pend initial value to assign to location of the end position
                 of the radial gradient 
    \param pend_radius initial value to assign to location of the end radius
                       of the radial gradient 
   */
  explicit
  WRATHLayerItemNodeRadialGradient(const WRATHTripleBufferEnabler::handle &r,
                                   const vec2 &pstart=vec2(0.0f, 0.0f),
                                   float pstart_radius=0.0f,
                                   const vec2 &pend=vec2(1.0f, 1.0f),
                                   float pend_radius=1.0f):
    T(r),
    WRATHRadialGradientValue(pstart, pstart_radius, pend, pend_radius)
  {
  }

  /*!\fn WRATHLayerItemNodeRadialGradient(S*, const vec2&, float, const vec2&, float)
    Ctor to create a child WRATHLayerItemNodeRadialGradient,
    parent takes ownership of child.
    \param pparent parent node object
    \param pstart initial value to assign to location of the start position
                  of the radial gradient 
    \param pstart_radius initial value to assign to location of the start radius
                         of the radial gradient 
    \param pend initial value to assign to location of the end position
                 of the radial gradient 
    \param pend_radius initial value to assign to location of the end radius
                       of the radial gradient 
   */
  template<typename S>
  explicit
  WRATHLayerItemNodeRadialGradient(S *pparent,
                                   const vec2 &pstart=vec2(0.0f, 0.0f),
                                   float pstart_radius=0.0f,
                                   const vec2 &pend=vec2(1.0f, 1.0f),
                                   float pend_radius=1.0f):
    T(pparent),
    WRATHRadialGradientValue(pstart, pstart_radius, pend, pend_radius)
  {
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
    static WRATHLayerItemNodeFunctionPacketT<T, WRATHRadialGradientValue> R;
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
    WRATHRadialGradientValue::extract_values_at(T::number_per_node_values, out_value);
  }

  /*!\fn void set_shader_brush
    Sets the gradient shader code (\ref WRATHShaderBrush::m_gradient_source)
    as the source to compute radial gradients
    \param brush WRATHShaderBrush to set
   */ 
  static
  void
  set_shader_brush(WRATHShaderBrush &brush)
  {
    T::set_shader_brush(brush);
    brush.m_gradient_source=gradient_source();
  }

  /*!\fn void set_from_brush
    Sets the node value that stores the
    y-texture coordinate of \ref WRATHGradient
    from the gradient of a \ref WRATHBrush
    (i.e. \ref WRATHBrush::m_gradient)
    \param brush \ref WRATHBrush from which to set y-texture coordinate
   */
  virtual
  void
  set_from_brush(const WRATHBrush &brush)
  {
    T::set_from_brush(brush);
    y_texture_coordinate(brush.m_gradient);
  }
};



/*! @} */



#endif
