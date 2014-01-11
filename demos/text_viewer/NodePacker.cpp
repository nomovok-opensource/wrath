/*! 
 * \file NodePacker.cpp
 * \brief file NodePacker.cpp
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


#include "NodePacker.hpp"

namespace
{
  class TheFunctions:public WRATHLayerNodeValuePackerBase::function_packet
  {
  public:
    virtual
    SpecDataProcessedPayload::handle
    create_handle(const ActiveNodeValuesCollection &spec) const
    {
      SpecDataProcessedPayload::handle R;

      R=NodePackerBase::functions().create_handle(spec);
      R->m_number_slots=std::min(NodePacker::max_node_count(), R->m_number_slots);

      return R;
    }


    virtual
    bool
    supports_per_node_value(GLenum shader_type) const
    {
      return NodePackerBase::functions().supports_per_node_value(shader_type);
    }

    virtual
    void
    append_fetch_code(WRATHGLShader::shader_source &src,
                      GLenum shader_stage,
                      const ActiveNodeValues &node_values,
                      const SpecDataProcessedPayload::handle &hnd,
                      const std::string &index_name) const
    {
      NodePackerBase::functions().append_fetch_code(src, shader_stage, node_values, hnd, index_name);
      hnd->m_number_slots=std::min(NodePacker::max_node_count(), hnd->m_number_slots);
    }

    virtual
    void
    add_actions(const SpecDataProcessedPayload::handle& payload,
                const ProcessedActiveNodeValuesCollection &spec,
                WRATHShaderSpecifier::ReservedBindings& reserved_bindings,
                WRATHGLProgramOnBindActionArray& actions,
                WRATHGLProgramInitializerArray& initers) const
    {
      NodePackerBase::functions().add_actions(payload, spec, reserved_bindings, actions, initers);
      payload->m_number_slots=std::min(NodePacker::max_node_count(), payload->m_number_slots);
    }
  };
}

int&
NodePacker::
max_node_count(void)
{
  static int V(100);
  return V;
}

const WRATHLayerNodeValuePackerBase::function_packet&
NodePacker::
functions(void)
{
  static TheFunctions R;
  

  return R;
}
