/*! 
 * \file WRATHLayerItemNodeLinearGradient.hpp
 * \brief file WRATHLayerItemNodeLinearGradient.hpp
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


#ifndef WRATH_HEADER_LAYER_ITEM_LINEAR_GRADIENT_HPP_
#define WRATH_HEADER_LAYER_ITEM_LINEAR_GRADIENT_HPP_

#include "WRATHConfig.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHLinearGradientValue.hpp"
#include "WRATHLayerItemNodeFunctionPacketT.hpp"


/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerItemNodeLinearGradient
  A WRATHLayerItemNodeLinearGradient is
  a generic class that adds linear gradient
  data to a node type via inheriting from
  \ref WRATHLinearGradientValue.
  \n\n The node type T must:
  - inherit from WRATHLayerItemNodeBase
  - define the static function const node_function_packet& functions(void)
  - the return value of the virtual function node_functions() must be the same as functions()
  - must define the enumeration number_per_node_values indicating the number of per-item
    uniforms it uses
  \tparam T Node type to supplement with linear gradient positional values
 */
template<typename T>
class WRATHLayerItemNodeLinearGradient:
  public T,
  public WRATHLinearGradientValue
{
public:

  enum
    {
      /*!
        Enumeration of the number of per node values
        from the base class T
       */
      base_number_per_node_values=T::number_per_node_values,

      /*!
        Enumeration indicating number of per node values
        a WRATHLayerItemNodeLinearGradient has
       */
      number_per_node_values=T::number_per_node_values+WRATHLinearGradientValue::number_per_node_values
    };

  /*!\fn WRATHLayerItemNodeLinearGradient(const WRATHTripleBufferEnabler::handle&,
                                          const vec2&, const vec2&)
    Ctor to create a root WRATHLayerItemNodeLinearGradient.
    \param r handle to WRATHTripleBufferEnabler to coordinate triple buffering
    \param pstart initial value to assign to location of the start position
                 of the linear gradient 
    \param pend initial value to assign to location of the end position
                of the linear gradient 
   */
  explicit
  WRATHLayerItemNodeLinearGradient(const WRATHTripleBufferEnabler::handle &r,
                                   const vec2 &pstart=vec2(0.0f, 0.0f),
                                   const vec2 &pend=vec2(1.0f, 1.0f)):
    T(r),
    WRATHLinearGradientValue(pstart, pend)
  {
  }

  /*!\fn WRATHLayerItemNodeLinearGradient(S*, const vec2&, const vec2&)
    Ctor to create a child WRATHLayerItemNodeLinearGradient,
    parent takes ownership of child.
    \param pparent parent node object
    \param pstart initial value to assign to location of the start position
                 of the linear gradient 
    \param pend initial value to assign to location of the end position
                of the linear gradient 
   */
  template<typename S>
  explicit
  WRATHLayerItemNodeLinearGradient(S *pparent,
                                   const vec2 &pstart=vec2(0.0f, 0.0f),
                                   const vec2 &pend=vec2(1.0f, 1.0f)):
    T(pparent),
    WRATHLinearGradientValue(pstart, pend)
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
    static WRATHLayerItemNodeFunctionPacketT<T, WRATHLinearGradientValue> R;
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
    WRATHLinearGradientValue::extract_values_at(T::number_per_node_values, out_value);
  }

  /*!\fn void set_shader_brush
    Sets the gradient shader code (\ref WRATHShaderBrush::m_gradient_source)
    as the source to compute linear gradients
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
