/*! 
 * \file WRATHLayerNodeValuePackerTextureFixed.cpp
 * \brief file WRATHLayerNodeValuePackerTextureFixed.cpp
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
#include "WRATHLayerNodeValuePackerUniformArrays.hpp"
#include "WRATHStaticInit.hpp"

/*
  Implementation overview:
  
   1) Each texel of the texture is one per node value
   2) Thus we do not need to deal with texel packing, etc.
*/


namespace
{
  const char *texture_name="WRATH_LAYER_UNIFORM_PACKER_TEXTURE_FIXED_sampler";


  class LocalFunctionPacket:public WRATHLayerNodeValuePackerBase::function_packet
  {
  public:
    typedef const char* c_string;

  
    virtual
    bool
    supports_per_node_value(GLenum /*shader_type*/) const
    {
      return true;
    }

    virtual
    SpecDataProcessedPayload::handle
    create_handle(const ActiveNodeValuesCollection &) const
    {
      return WRATHNew SpecDataProcessedPayload();
    }

    virtual
    void
    add_actions(const SpecDataProcessedPayload::handle& /*payload*/,
                const ProcessedActiveNodeValuesCollection&,
		WRATHShaderSpecifier::ReservedBindings& /*reserved_bindings*/,
                WRATHGLProgramOnBindActionArray& /*actions*/,
                WRATHGLProgramInitializerArray& /*initers*/) const;
    

    void
    append_fetch_code(WRATHGLShader::shader_source &src,
                      GLenum shader_stage,
                      const ActiveNodeValues &node_values,
                      const SpecDataProcessedPayload::handle &hnd,
                      const std::string &index_name) const;

  };


  class TextureForNode:public WRATHTextureChoice::texture_base
  {
  public:
    TextureForNodeBase(WRATHLayerNodeValuePackerBase::DataToGL src,
		       const Payload::const_handle &hnd):
      m_rgba8_data(hnd->m_texture_width*256, 
		   vecN<uint8_t, 4>(0, 0, 0, 0)),
      m_texture_unit(hnd->m_texture_unit),
      m_texture_name(0),
      m_texture_width(hnd->m_texture_width),
      m_active(true),
      m_source(src)
    {}

    void
    deactiveate(void)
    {
      m_active=false;
    }

    void
    bind_texture(GLenum ptexture_unit);

    GLenum
    texture_unit(void) const
    {
      return m_texture_unit;
    }

  private:
    void
    convert_from_fp32(const_c_array<float> input, int number_nodes);

    std::vector<vecN<uint8_t, 4> > m_rgba8_data;
    GLenum m_texture_unit;
    GLuint m_texture_name;
    int m_texture_width;
    bool m_active;
    WRATHLayerNodeValuePackerBase::DataToGL m_source;

  };

}

////////////////////////////////////////////////
// TextureForNode methods
void
TextureForNode::
bind_texture(GLenum ptexture_unit)
{
  WRATHassert(ptexture_unit==m_texture_unit);
  WRATHunused(ptexture_unit);

  if(m_rgba8_data.empty() or !m_active)
    {
      return;
    }

  /*
    convert from FP32 to our wierd 8+8.8+8 format.
   */
  if(m_texture_name==0)
    {
      glGenTextures(1, &m_texture_name);
      glBindTexture(1, m_texture_name);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);   

      glTexImage2D(GL_TEXTURE_2D,
		   0,
		   GL_RGBA,
		   m_texture_width(), 256, 0,
		   GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      
    }
  else
    {
      glBindTexture(GL_TEXTURE_2D, m_texture_name);
    }

  /*
    convert from fp32 to 8+8.8+8:
   */
  const_c_array<float> float_data(m_source.data_to_pack_to_GL_restrict());
  int number_nodes(m_source.number_slots_to_pack_to_GL());
  convert_from_fp32(float_data, number_nodes);

  /*
    now upload it to GL
   */
  glTexSubImage2D(GL_TEXTURE_2D,
		  0,
		  0, 0,
		  m_texture_width, number_nodes,
		  GL_RGBA
		  GL_UNSIGNED_BYTES,
		  &m_rgba8_data[0]);
}


///////////////////////////////////////////////
// WRATHLayerNodeValuePackerTextureFixed methods
WRATHLayerNodeValuePackerTextureFixed::
WRATHLayerNodeValuePackerTextureFixed(WRATHLayerBase *layer,
				      const SpecDataProcessedPayload::const_handle &payload,
				      const ProcessedActiveNodeValuesCollection &spec):
  WRATHLayerNodeValuePackerBase(layer, payload, spec)
{
  Payload::const_handle payload;

  WRATHassert(h.dynamic_cast_handle<Payload>().valid());
  payload=h.static_cast_handle<Payload>();

  if(payload->m_number_per_node_values > 0)
    {
      m_texture=WRATHNew TextureForNode(datum, payload);
    }
}

WRATHLayerNodeValuePackerTextureFixed::
~WRATHLayerNodeValuePackerTextureFixed()
{}

void
WRATHLayerNodeValuePackerTextureFixed::
phase_render_deletion(void)
{
  WRATHassert(m_texture.dynamic_cast_handle<>().valid());
  m_texture.static_cast_handle<>()->deactiveate();
  m_texture=NULL;

  WRATHLayerNodeValuePackerBase::phase_render_deletion();
}
  
void
WRATHLayerNodeValuePackerTextureFixed::
append_uniforms(WRATHSubItemDrawState &skey)
{
  if(m_texture.valid())
    {
      TextureForNodeBase::handle H;
      
      WRATHassert(m_texture.static_cast_handle<TextureForNode>().valid());
      H=m_texture.static_cast_handle<TextureForNode>();
      
      skey.add_texture(H->texture_unit(), H);
    }
}

const WRATHLayerNodeValuePackerBase::function_packet&
WRATHLayerNodeValuePackerTextureFixed::
functions(void)
{
  WRATHStaticInit();
  static NodePackerTextureFunctions return_value;
  return return_value;
}


