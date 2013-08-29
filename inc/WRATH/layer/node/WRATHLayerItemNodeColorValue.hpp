/*! 
 * \file WRATHLayerItemNodeColorValue.hpp
 * \brief file WRATHLayerItemNodeColorValue.hpp
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


#ifndef __WRATH_LAYER_ITEM_CONST_COLOR_HPP__
#define __WRATH_LAYER_ITEM_CONST_COLOR_HPP__

#include "WRATHConfig.hpp"
#include "WRATHLayerItemNodeBase.hpp"
#include "WRATHBrush.hpp"

/*! \addtogroup Layer
 * @{
 */


/*!\class WRATHLayerItemNodeColorValue
  A WRATHLayerItemNodeColorValue is a generic template
  class that adds a per-item uniform of color
  to a node type T. The node type T must:
  - inherit from WRATHLayerItemNodeBase
  - define the static function const node_function_packet& functions(void)
  - the return value of the virtual function node_functions() must be the same as functions()
  - must define the enumeration number_per_node_values indicating the number of per-item
    uniforms it uses
  \tparam T Node type to supplement with const color values
 */
template<typename T>
class WRATHLayerItemNodeColorValue:public T
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
        a WRATHLayerItemNodeColorValue has
       */
      number_per_node_values=T::number_per_node_values+4
    };

  /*!\typedef color_type
    Use WRATHGradient::color to specify a color.
   */
  typedef WRATHGradient::color color_type;

  /*!\fn WRATHLayerItemNodeColorValue(const WRATHTripleBufferEnabler::handle&,
                                      const color_type)
    Ctor to create a root WRATHLayerItemNodeColorValue.
    \param r handle to WRATHTripleBufferEnabler to coordinate triple buffering
    \param pcolor initial value to assign for the color
   */
  WRATHLayerItemNodeColorValue(const WRATHTripleBufferEnabler::handle &r,
                               const color_type pcolor=vec4(1.0f, 1.0f, 1.0f, 1.0f)):
    T(r)
  {
    color(pcolor);
  }

  /*!\fn WRATHLayerItemNodeColorValue(S*, const color_type)
    Ctor to create a child WRATHLayerItemNodeColorValue,
    parent takes ownership of child.
    \param pparent parent node object
    \param pcolor initial value to assign for the color
   */
  template<typename S>
  explicit
  WRATHLayerItemNodeColorValue(S *pparent,
                               const color_type pcolor=color_type(1.0f, 1.0f, 1.0f, 1.0f)):
    T(pparent)
  {
    color(pcolor);
  }

  /*!\fn const vec4& color(void) const
    Returns the const color
   */
  const vec4&
  color(void) const
  {
    return m_color;
  }

  /*!\fn void color(const color_type&)
    Sets the const color
    \param p value to which to set the color
   */
  void
  color(const color_type &p)
  {
    m_color=p.m_value;
  }
    
  /*!\fn const WRATHColorValueSource* color_source
    Returns the WRATHColorValueSource object
    to use with the color whose data is 
    determined by a WRATHLayerItemNodeColorValue.
   */
  static
  const WRATHColorValueSource*
  color_source(void);

  /*!\fn void set_shader_brush
    Sets \ref WRATHShaderBrush::m_color_value_source
    to \ref color_source().
    \param brush WRATHShaderBrush to which to set value
   */
  static
  void
  set_shader_brush(WRATHShaderBrush &brush)
  {
    T::set_shader_brush(brush);
    brush.m_color_value_source=color_source();
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
  functions(void);

  /*!\fn void extract_values(reorder_c_array<float>)
    Implements WRATHLayerItemNodeBase::extract_values(reorder_c_array<float>)
    \param out_value location to which to write per-node values
   */
  virtual
  void
  extract_values(reorder_c_array<float> out_value);

private:
  vec4 m_color;
};

/*! @} */


#include "WRATHLayerItemNodeColorValueImplement.tcc"


#endif
