// -*- C++ -*-

/*! 
 * \file WRATHLayerItemNodeColorValueImplement.tcc
 * \brief file WRATHLayerItemNodeColorValueImplement.tcc
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


#if !defined(__WRATH_LAYER_ITEM_CONST_COLOR_HPP__) || defined(__WRATH_LAYER_ITEM_CONST_COLOR_IMPLEMENT_TCC__)
#error "Direction inclusion of private header file WRATHLayerItemNodeColorValueImplement.tcc" 
#endif

#define __WRATH_LAYER_ITEM_CONST_COLOR_IMPLEMENT_TCC__

#include "WRATHStaticInit.hpp"

namespace WRATHLayerItemNodeColorValueImplement
{
  void
  add_per_node_values_implement(int start,
                                  WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                                  const WRATHLayerNodeValuePackerBase::function_packet &available);


  template<typename T>
  class function_packet:public WRATHLayerItemNodeBase::node_function_packet
  {
  public:
    virtual
    WRATHLayerItemNodeBase*
    create_completely_clipped_node(const WRATHTripleBufferEnabler::handle &tr) const
    {
      return T::functions().create_completely_clipped_node(tr);
    }

    virtual
    void
    add_per_node_values(WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                          const WRATHLayerNodeValuePackerBase::function_packet &available) const
    {
      T::functions().add_per_node_values(spec, available);
      add_per_node_values_implement(T::number_per_node_values, spec, available);
    }

    virtual
    void
    append_shader_source(std::map<GLenum, WRATHGLShader::shader_source> &src,
                         const WRATHLayerNodeValuePackerBase::function_packet &available) const
    {
      T::functions().append_shader_source(src, available);
    }
  };

  const WRATHColorValueSource*
  color_source(void);

}

template<typename T>
const WRATHLayerItemNodeBase::node_function_packet&
WRATHLayerItemNodeColorValue<T>::
functions(void) 
{
  WRATHStaticInit();
  static WRATHLayerItemNodeColorValueImplement::function_packet<T> return_value;

  return return_value;
}

template<typename T>
void
WRATHLayerItemNodeColorValue<T>::
extract_values(reorder_c_array<float> out_value)
{
  T::extract_values(out_value.sub_array(0, T::number_per_node_values));
  out_value[T::number_per_node_values+0]=m_color.x();
  out_value[T::number_per_node_values+1]=m_color.y();
  out_value[T::number_per_node_values+2]=m_color.z();
  out_value[T::number_per_node_values+3]=m_color.w();
}

template<typename T>
const WRATHColorValueSource*
WRATHLayerItemNodeColorValue<T>::
color_source(void)
{
  return WRATHLayerItemNodeColorValueImplement::color_source();
}
