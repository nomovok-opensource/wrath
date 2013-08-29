/*! 
 * \file WRATHLayerItemNodeColorValue.cpp
 * \brief file WRATHLayerItemNodeColorValue.cpp
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



#include "WRATHLayerItemNodeColorValue.hpp"

namespace 
{
  void
  generate_source(WRATHGLShader::shader_source &obj,
                  const std::string &prec)
  {
    std::ostringstream ostr;

    ostr << "\n\n" << prec << " vec4\n"
         << "const_color_value(void)"
         << "\n{\n" 
         << "\n\treturn vec4(fetch_node_value(WRATH_LAYER_ITEM_NODE_CONST_COLOR_RED),"
         << "\n\t                               WRATH_LAYER_ITEM_NODE_CONST_COLOR_GREEN,"
         << "\n\t                               WRATH_LAYER_ITEM_NODE_CONST_COLOR_BLUE,"
         << "\n\t                               WRATH_LAYER_ITEM_NODE_CONST_COLOR_ALPHA);"
         << "\n}\n";
         
    obj.add_source(ostr.str(), WRATHGLShader::from_string);
  }

  class ColorNodeSource:public WRATHColorValueSource
  {
  public:
    ColorNodeSource(void)
    {
      generate_source(m_shader_source[default_precision], prec_string(default_precision));
      generate_source(m_shader_source[mediump_precision], prec_string(mediump_precision));
      generate_source(m_shader_source[highp_precision], prec_string(highp_precision));
    }

    virtual
    uint32_t
    shader_useablity_flags(void) const
    {
      /*
        unextended GLES2 does not allow variable
        uniform indexing in fragment shader

        TODO: the uniform packer type can decide
        if a per-item uniform is fetchable from
        the vertex or fragment shader, but the
        node type does not know about it...
       */
      return vertex_shader_fetchable;
    }

    virtual
    const WRATHGLShader::shader_source& 
    shader_code(enum precision_t prec) const
    {
      return m_shader_source[prec];
    }

  private:
    vecN<WRATHGLShader::shader_source,3> m_shader_source;
  };
}



const WRATHColorValueSource*
WRATHLayerItemNodeColorValueImplement::
color_source(void)
{
  static ColorNodeSource R;
  return &R;
}

void
WRATHLayerItemNodeColorValueImplement::
add_per_node_values_implement(int start,
                                WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                                const WRATHLayerNodeValuePackerBase::function_packet &)
{
  /*
    TODO: 
    Specify how each of the colors is needed: in the vertex shader
    or fragment shader based upon available.supports_per_item_uniform()
  */
  spec
    .add_source(start+0, "WRATH_LAYER_ITEM_NODE_CONST_COLOR_RED", GL_VERTEX_SHADER)
    .add_source(start+1, "WRATH_LAYER_ITEM_NODE_CONST_COLOR_GREEN", GL_VERTEX_SHADER)
    .add_source(start+2, "WRATH_LAYER_ITEM_NODE_CONST_COLOR_BLUE", GL_VERTEX_SHADER)
    .add_source(start+3, "WRATH_LAYER_ITEM_NODE_CONST_COLOR_ALPHA", GL_VERTEX_SHADER);
}
