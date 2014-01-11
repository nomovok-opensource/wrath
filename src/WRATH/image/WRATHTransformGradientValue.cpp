/*! 
 * \file WRATHTransformGradientValue.cpp
 * \brief file WRATHTransformGradientValue.cpp
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
#include "WRATHTransformGradientValue.hpp"

void
WRATHTransformGradientValue::
add_per_node_values_at(int start,
                         WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                         const WRATHLayerNodeValuePackerBase::function_packet &fpt,
                         uint32_t add_to_shader_stage_flags)
{ 
  if((add_to_shader_stage_flags&add_to_fragment_shader)!=0 
     and fpt.supports_per_node_value(GL_FRAGMENT_SHADER))
    {
      GLenum shader_stage=GL_FRAGMENT_SHADER;
      spec
        .add_source(start+0, "WRATH_TRANSFORM_GRADIENT_tx", shader_stage)
        .add_source(start+1, "WRATH_TRANSFORM_GRADIENT_ty", shader_stage)
        .add_source(start+2, "WRATH_TRANSFORM_GRADIENT_rx", shader_stage)
        .add_source(start+3, "WRATH_TRANSFORM_GRADIENT_ry", shader_stage);
    }

  if((add_to_shader_stage_flags&add_to_fragment_shader)!=0 
     and fpt.supports_per_node_value(GL_VERTEX_SHADER))
    {
      GLenum shader_stage=GL_VERTEX_SHADER;
      spec
        .add_source(start+0, "WRATH_TRANSFORM_GRADIENT_tx", shader_stage)
        .add_source(start+1, "WRATH_TRANSFORM_GRADIENT_ty", shader_stage)
        .add_source(start+2, "WRATH_TRANSFORM_GRADIENT_rx", shader_stage)
        .add_source(start+3, "WRATH_TRANSFORM_GRADIENT_ry", shader_stage);
    }
}
