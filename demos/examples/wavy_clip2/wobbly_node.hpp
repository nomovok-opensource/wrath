/*! 
 * \file wobbly_node.hpp
 * \brief file wobbly_node.hpp
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
class WobblyNode:public BaseNode
{
public:
  
  enum
    {
      //has 3 more per-node value than BaseNode
      number_per_node_values=BaseNode::number_per_node_values + 3
    };
  
  //note that we allow the parent of a WobblyNode
  //to be another type, The reason being that 
  //the type BaseNode type might itself be
  //another augmented node type, so we let template recursion
  //into the eventual base node class specify what is
  //an allowable parent class
  template<typename S>
  explicit
  WobblyNode(S *p):
    BaseNode(p),
    m_wobble_freq(1.0f),
    m_wobble_magnitude(1.0f),
    m_wobble_phase(0.0f)
  {}
  
  WobblyNode(const WRATHTripleBufferEnabler::handle &tr):
    BaseNode(tr),
    m_wobble_freq(1.0f),
    m_wobble_magnitude(1.0f),
    m_wobble_phase(0.0f)
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
    //note that we place a different value to send to GL,
    //this is because the node stores a frequency but we
    //want a coeff.
    out_values[BaseNode::number_per_node_values + 0] = 2.0f*M_PI * m_wobble_freq;
    out_values[BaseNode::number_per_node_values + 1] = m_wobble_magnitude;
    out_values[BaseNode::number_per_node_values + 2] = m_wobble_phase;
  }

  //the actual radii values, we make them public.
  float m_wobble_freq, m_wobble_magnitude, m_wobble_phase;
};

/*
  RingNodeFunctionPacket defines the WRATHLayerItemNodeBase::node_function_packet
  returned by RingNode<BaseNode>::functions(), it too need to be a template
  class from the base node type.
 */
template<typename BaseNode>
class WobblyNodeFunctionPacket:public WRATHLayerItemNodeBase::node_function_packet
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

    //now add our values, since they are used in the fragment shader
    //add them their if possible, otherwise add them to the vertex shader
    if(fp.supports_per_node_value(GL_FRAGMENT_SHADER))
      {
        spec.add_source(BaseNode::number_per_node_values + 0, "wobbly_angular_speed", GL_FRAGMENT_SHADER);
        spec.add_source(BaseNode::number_per_node_values + 1, "wobbly_magnitude", GL_FRAGMENT_SHADER);
        spec.add_source(BaseNode::number_per_node_values + 2, "wobbly_phase", GL_FRAGMENT_SHADER);
      }
    else
      {
        spec.add_source(BaseNode::number_per_node_values + 0, "wobbly_angular_speed", GL_VERTEX_SHADER);
        spec.add_source(BaseNode::number_per_node_values + 1, "wobbly_magnitude", GL_VERTEX_SHADER);
        spec.add_source(BaseNode::number_per_node_values + 2, "wobbly_phase", GL_VERTEX_SHADER);
      }
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
WobblyNode<BaseNode>::
functions(void)
{
  static WobblyNodeFunctionPacket<BaseNode> R;
  return R;
}
