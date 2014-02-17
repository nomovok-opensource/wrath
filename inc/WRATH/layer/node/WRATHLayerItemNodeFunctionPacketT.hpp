/*! 
 * \file WRATHLayerItemNodeFunctionPacketT.hpp
 * \brief file WRATHLayerItemNodeFunctionPacketT.hpp
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


#ifndef WRATH_HEADER_LAYER_ITEM_NODE_FUNCTION_PACKET_T_HPP_
#define WRATH_HEADER_LAYER_ITEM_NODE_FUNCTION_PACKET_T_HPP_


/*! \addtogroup Layer
 * @{
 */

/*!\class WRATHLayerItemNodeFunctionPacketT
  WRATHLayerItenNodeFunctionPacketT is a template machine
  to generate a WRATHLayerItemNodeBase::node_function_packet
  derived type as follows:\n\n

  Given a node type N, derived from WRATHLayerItemNodeBase which
  has the static method:
  \code
    const WRATHLayerItemNodeBase::node_function_packet& functions(void)
  \endcode
  and a value type S which has the static method:
  \code
   void add_per_node_values_at(int, WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection&, 
                                 const WRATHLayerNodeValuePackerBase::function_packet &)
  \endcode

  create a WRATHLayerItemNodeBase::node_function_packet which is the
  same as N::functions() except that it also calls add_per_node_values_at(),
  passing N::number_per_node_values as the start index argument.
  \tparam N node type derived from WRATHLayerItemNodeBase
  \tparam S type implementng add_per_node_values_at() to add additional per node values
 */
template<typename N, typename S>
class WRATHLayerItemNodeFunctionPacketT:public WRATHLayerItemNodeBase::node_function_packet
{
public:
  virtual
  WRATHLayerItemNodeBase*
  create_completely_clipped_node(const WRATHTripleBufferEnabler::handle &tr) const
  {
    return N::functions().create_completely_clipped_node(tr);
  }
  
  virtual
  void
  add_per_node_values(WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                        const WRATHLayerNodeValuePackerBase::function_packet &available) const
  {
    N::functions().add_per_node_values(spec, available);
    S::add_per_node_values_at(N::number_per_node_values, spec, available);
  }
  
  virtual
  void
  append_shader_source(std::map<GLenum, WRATHGLShader::shader_source> &src,
                       const WRATHLayerNodeValuePackerBase::function_packet &available) const
  {
    N::functions().append_shader_source(src, available);
  }   
  
};


/*! @} */

#endif
