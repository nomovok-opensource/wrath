/*! 
 * \file WRATHLayerItemNodeRepeatGradient.hpp
 * \brief file WRATHLayerItemNodeRepeatGradient.hpp
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


#ifndef __WRATH_LAYER_ITEM_NODE_REPEAT_GRADIENT_HPP__
#define __WRATH_LAYER_ITEM_NODE_REPEAT_GRADIENT_HPP__

#include "WRATHConfig.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHRepeatGradientValue.hpp"
#include "WRATHLayerItemNodeFunctionPacketT.hpp"


/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerItemNodeRepeatGradient
  A WRATHLayerItemNodeRepeatGradient is
  a generic class that adds a repeat to
  a gradient class via inheriting
  from \ref WRATHRepeatGradientValue.
  \n\n The node type T must:
  - inherit from WRATHLayerItemNodeBase
  - define the static function const node_function_packet& functions(void)
  - the return value of the virtual function node_functions() must be the same as functions()
  - must define the enumeration number_per_node_values indicating the number of per-item
    uniforms it uses  
 */
template<typename T>
class WRATHLayerItemNodeRepeatGradient:
  public T,
  public WRATHRepeatGradientValue
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
        a WRATHLayerItemNodeRepeatGradient has
       */
      number_per_node_values=T::number_per_node_values+WRATHRepeatGradientValue::number_per_node_values
    };

  /*!\fn WRATHLayerItemNodeRepeatGradient(const WRATHTripleBufferEnabler::handle&,
                                                 const vec2&, const vec2&)
    Ctor to create a root WRATHLayerItemNodeRepeatGradient.
    \param r handle to WRATHTripleBufferEnabler to coordinate triple buffering
    \param pstart_window specifies the start of the repeat window
    \param pend_window specifies the end of the repeat window
   */
  explicit
  WRATHLayerItemNodeRepeatGradient(const WRATHTripleBufferEnabler::handle &r,
                                    const vec2 &pstart_window=vec2(0.0f, 0.0f),
                                    const vec2 &pend_window=vec2(0.0f, 0.0f)):
    T(r),
    WRATHRepeatGradientValue(pstart_window, pend_window)
  {
  }

  /*!\fn WRATHLayerItemNodeRepeatGradient(S*, const vec2&, const vec2&)
    Ctor to create a child WRATHLayerItemNodeRepeatGradient,
    parent takes ownership of child.
    \param pparent parent node object
    \param pstart_window specifies the start of the repeat window
    \param pend_window specifies the end of the repeat window
   */
  template<typename S>
  explicit
  WRATHLayerItemNodeRepeatGradient(S *pparent,
                                   const vec2 &pstart_window=vec2(0.0f, 0.0f),
                                   const vec2 &pend_window=vec2(0.0f, 0.0f)):
    T(pparent),
    WRATHRepeatGradientValue(pstart_window, pend_window)
  {
  }

  /*!\fn void set_shader_brush
    Sets the gradient shader code (\ref WRATHShaderBrush::m_gradient_source)
    as the source to compute repeat gradient.
    \param brush WRATHShaderBrush to set
   */ 
  static
  void
  set_shader_brush(WRATHShaderBrush &brush)
  {
    T::set_shader_brush(brush);
    brush.m_gradient_source=WRATHRepeatGradientValue::gradient_source(brush.m_gradient_source);
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
    static WRATHLayerItemNodeFunctionPacketT<T, WRATHRepeatGradientValue> R;
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
    WRATHRepeatGradientValue::extract_values_at(T::number_per_node_values, out_value);
  }

};

/*! @} */

#endif
