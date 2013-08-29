/*! 
 * \file augmented_node.hpp
 * \brief file augmented_node.hpp
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


#include "WRATHConfig.hpp"
#include "WRATHLayerItemWidgets.hpp"
#include "WRATHLayerItemNodeBase.hpp"

/*
  we will define our custom node class a template type.
 */

template<typename BaseNode>
class RingNode:public BaseNode
{
public:
  
  enum
    {
      //has two more per-node value than BaseNode
      number_per_node_values=BaseNode::number_per_node_values + 2
    };
  
  //note that we allow the parent of a RingNode
  //to be another type, The reason being that 
  //the type BaseNode type might itself be
  //another augmented node type, so we let template recursion
  //into the eventual base node class specify what is
  //an allowable parent class
  template<typename S>
  explicit
  RingNode(S *p):
    BaseNode(p),
    m_inner_radius(0.0f),
    m_outer_radius(10.0f),
    m_velocity(0.0f, 0.0f)
  {}
  
  RingNode(const WRATHTripleBufferEnabler::handle &tr):
    BaseNode(tr),
    m_inner_radius(0.0f),
    m_outer_radius(10.0f),
    m_velocity(0.0f, 0.0f)
  {}
  
  static
  const WRATHLayerItemNodeBase::node_function_packet&
  functions(void);
  
  const WRATHLayerItemNodeBase::node_function_packet&
  node_functions(void) const
  {
    return functions();
  }
  
  void
  extract_values(reorder_c_array<float> out_values)
  {
    //call the BaseNode's extract_values() 
    BaseNode::extract_values(out_values.sub_array(0, BaseNode::number_per_node_values));
    
    //place our value after BaseNode values,
    //adjust them so that both radii are positive
    //and the larger is the outer radii.
    float r1, r2;

    r1=std::abs(m_inner_radius);
    r2=std::abs(m_outer_radius);

    out_values[BaseNode::number_per_node_values + 0] = std::min(r1, r2);
    out_values[BaseNode::number_per_node_values + 1] = std::max(r1, r2);
  }

  //the actual radii values, we make them public.
  float m_inner_radius, m_outer_radius;  

  //a value not sent to GL
  vec2 m_velocity;
};

/*
  RingNodeFunctionPacket defines the WRATHLayerItemNodeBase::node_function_packet
  returned by RingNode<BaseNode>::functions(), it too need to be a template
  class from the base node type.
 */
template<typename BaseNode>
class RingNodeFunctionPacket:public WRATHLayerItemNodeBase::node_function_packet
{
public:
  
  virtual
  WRATHLayerItemNodeBase*
  create_completely_clipped_node(const WRATHTripleBufferEnabler::handle &r) const
  {
     //we will let the BaseNode class make the non-visible node.
     return BaseNode::functions().create_completely_clipped_node(r);
  }

  virtual
  void
  add_per_node_values(WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                      const WRATHLayerNodeValuePackerBase::function_packet &fp) const
  {
    //add the per_node_values from the BaseNode type
    BaseNode::functions().add_per_node_values(spec, fp);

    //now add our values, make them available to the vertex shader
    spec.add_source(BaseNode::number_per_node_values + 0, "inner_radius", GL_VERTEX_SHADER);
    spec.add_source(BaseNode::number_per_node_values + 1, "outer_radius", GL_VERTEX_SHADER);
  }

  virtual
  void
  append_shader_source(std::map<GLenum, WRATHGLShader::shader_source> &src,
                       const WRATHLayerNodeValuePackerBase::function_packet &fpt) const
  {
     //our custom example does not add any additional shader code, but
     //we do need to let BaseNode add it's code
     BaseNode::functions().append_shader_source(src, fpt);
  }
};


template<typename BaseNode>
const WRATHLayerItemNodeBase::node_function_packet&
RingNode<BaseNode>::
functions(void)
{
  static RingNodeFunctionPacket<BaseNode> R;
  return R;
}
