/*! 
 * \file WRATHLayerItemNodeRotateTranslate.cpp
 * \brief file WRATHLayerItemNodeRotateTranslate.cpp
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
#include "WRATHLayerItemNodeRotateTranslate.hpp"
#include "WRATHLayer.hpp"
#include "WRATHStaticInit.hpp"
#include "WRATHUtil.hpp"

namespace
{
  class Transformer:public WRATHLayerIntermediateTransformation
  {
  public:

    Transformer(const WRATHLayerItemNodeRotateTranslateValues *n,
                const WRATHTripleBufferEnabler::handle &tr):
      m_node(n),
      m_tr(tr)
    {
      m_sig=m_tr->connect(WRATHTripleBufferEnabler::on_complete_simulation_frame,
                          WRATHTripleBufferEnabler::pre_update_no_lock,
                          boost::bind(&Transformer::on_complete_simulation_frame, this));
    }

    ~Transformer()
    {
      m_sig.disconnect();
    }

    void
    unhook(void)
    {
      m_sig.disconnect();
      m_node=NULL;
    }

    virtual
    void
    modify_matrix(float4x4& in_out_matrix)
    {
      const WRATH2DRigidTransformation &value(m_values[m_tr->present_ID()]);
      /*
        insert the transformation 
        between the parent and the layer,
        that is why we multiply by on the left.
      */
      in_out_matrix=value.matrix4()*in_out_matrix;
    }

  private:

    void
    on_complete_simulation_frame(void)
    {
      if(m_node!=NULL)
        {
          m_values[m_tr->current_simulation_ID()]=m_node->m_transformation;
        }
      else
        {
          m_values[m_tr->current_simulation_ID()]=WRATH2DRigidTransformation();
        }
    }

    const WRATHLayerItemNodeRotateTranslateValues *m_node;
    vecN<WRATH2DRigidTransformation, 3> m_values;
    WRATHTripleBufferEnabler::handle m_tr;
    WRATHTripleBufferEnabler::connect_t m_sig;
  };

  class NodeRotateTranslateFunctions:public WRATHLayerItemNodeBase::node_function_packet
  {
  public:
    virtual
    WRATHLayerItemNodeBase*
    create_completely_clipped_node(const WRATHTripleBufferEnabler::handle &r) const
    {
      WRATHLayerItemNodeRotateTranslate *return_value;

      return_value=WRATHNew WRATHLayerItemNodeRotateTranslate(r);
      return_value->visible(false);
  
      return return_value;
    }

    virtual
    void
    add_per_node_values(WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                          const WRATHLayerNodeValuePackerBase::function_packet &) const
    {
      spec
        .add_source(0, "WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_RX", GL_VERTEX_SHADER)
        .add_source(1, "WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_RY", GL_VERTEX_SHADER)
        .add_source(2, "WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_TX", GL_VERTEX_SHADER)
        .add_source(3, "WRATH_LAYER_ROTATE_TRANSLATE_TRANSFORMATION_TY", GL_VERTEX_SHADER)
        .add_source(4, "WRATH_LAYER_ROTATE_TRANSLATE_Z", GL_VERTEX_SHADER);
    }

    virtual
    void
    append_shader_source(std::map<GLenum, WRATHGLShader::shader_source> &src,
                         const WRATHLayerNodeValuePackerBase::function_packet &) const
    {
      src[GL_VERTEX_SHADER].add_source("transformation_layer_rotate_translate.vert.wrath-shader.glsl", 
                                       WRATHGLShader::from_resource);

      src[GL_FRAGMENT_SHADER].add_source("transformation_layer_rotate_translate.frag.wrath-shader.glsl", 
                                         WRATHGLShader::from_resource);
    }
  };

  

}




//////////////////////////////////////////////
// WRATHLayerItemNodeRotateTranslateValues methods
void
WRATHLayerItemNodeRotateTranslateValues::
compose(const WRATHLayerItemNodeRotateTranslateValues &parent_value,
        const WRATHLayerItemNodeRotateTranslateValues &local_value)
{
  m_visible=parent_value.m_visible and local_value.m_visible;
  m_transformation=parent_value.m_transformation*local_value.m_transformation;
}

const WRATHLayerItemNodeBase::node_function_packet&
WRATHLayerItemNodeRotateTranslateValues::
functions(void) 
{
  WRATHStaticInit();
  static NodeRotateTranslateFunctions return_value;
  return return_value;
}

void
WRATHLayerItemNodeRotateTranslateValues::
extract_values(reorder_c_array<float> out_values, float z_order)
{
  vec4 as_vec4(m_transformation.value_as_vec4());

  out_values[0]=as_vec4.x();
  out_values[1]=as_vec4.y();
  out_values[2]=as_vec4.z();
  out_values[3]=as_vec4.w();
  
  out_values[4]=m_visible?
    z_order:
    -100.0f;
}

WRATHLayerIntermediateTransformation::handle
WRATHLayerItemNodeRotateTranslateValues::
create_pre_transformer(const WRATHTripleBufferEnabler::handle &tr)
{
  return WRATHNew Transformer(this, tr);
}

void
WRATHLayerItemNodeRotateTranslateValues::
unhook(const WRATHLayerIntermediateTransformation::handle &h)
{
  if(h.valid())
    {
      WRATHassert(dynamic_cast<Transformer*>(h.raw_pointer())!=NULL);
      static_cast<Transformer*>(h.raw_pointer())->unhook();
    }
}
