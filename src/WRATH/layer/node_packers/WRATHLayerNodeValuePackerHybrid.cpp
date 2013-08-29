/*! 
 * \file WRATHLayerNodeValuePackerHybrid.cpp
 * \brief file WRATHLayerNodeValuePackerHybrid.cpp
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
#include "WRATHLayerNodeValuePackerHybrid.hpp"

namespace
{
  void
  create_active_node_value_collection(const WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &in_spec,
                                      GLenum shader_stage,
                                      WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &out_spec)
  {
    WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection::map_type::const_iterator iter;

    iter=in_spec.entries().find(shader_stage);
    if(iter!=in_spec.entries().end())
      {
        out_spec.absorb(iter->second, shader_stage);
      }
  }

    
  class LocalFunctionPacket:public WRATHLayerNodeValuePackerBase::function_packet
  {
  public:
    LocalFunctionPacket(const WRATHLayerNodeValuePackerBase::function_packet *v,
                        const WRATHLayerNodeValuePackerBase::function_packet *f):
      m_vertex(v),
      m_fragment(f)
    {}

    virtual
    SpecDataProcessedPayload::handle
    create_handle(const ActiveNodeValuesCollection &spec) const;

    virtual
    void
    append_fetch_code(WRATHGLShader::shader_source &src,
                      GLenum shader_stage,
                      const ActiveNodeValues &node_values,
                      const SpecDataProcessedPayload::handle &hnd,
                      const std::string &index_name) const;

    virtual
    void
    add_actions(const SpecDataProcessedPayload::handle& /*payload*/,
                const ProcessedActiveNodeValuesCollection&,
		WRATHShaderSpecifier::ReservedBindings& /*reserved_bindings*/,
                WRATHGLProgramOnBindActionArray& /*actions*/,
                WRATHGLProgramInitializerArray& /*initers*/) const;

    virtual
    bool
    supports_per_node_value(GLenum shader_type) const
    {
      return shader_type==GL_VERTEX_SHADER 
        or shader_type==GL_FRAGMENT_SHADER;
    }
    
  private:
    const WRATHLayerNodeValuePackerBase::function_packet *m_vertex;
    const WRATHLayerNodeValuePackerBase::function_packet *m_fragment;
    static
    LocalFunctionPacket::SpecDataProcessedPayload::handle
    create_handle_stage(GLenum shader_stage, 
                        const ActiveNodeValuesCollection &spec,
                        WRATHLayerNodeValuePackerHybridImplement::Payload::handle p,
                        const WRATHLayerNodeValuePackerBase::function_packet *src)
    {

      LocalFunctionPacket::SpecDataProcessedPayload::handle return_value;
      ActiveNodeValuesCollection vs;
      NodeDataPackParametersCollection::packing_group vg, vg_out;

      /*
        fill vs with a ActiveNodeValuesCollection that
        only has those node values of the named shader stage
       */
      create_active_node_value_collection(spec, shader_stage, vs);
      return_value=src->create_handle(vs);
      
      /*
        create individual packing_group for each stage
        we do this so that the spec passed into append_fetch_code
        is restricted to just those node values needed
        for the named stage
      */
      vg=return_value->m_packer_parameters.get_shader_packer(shader_stage);
      vg_out=p->m_packer_parameters.add_packing_group(return_value->m_packer_parameters.packer_set_parameters(vg));
      p->m_packer_parameters.set_shader_packer(shader_stage, vg_out);

      return return_value;
    }
  };

  class LocalFunctionPacketStorage:boost::noncopyable
  {
  public:
    LocalFunctionPacketStorage(void)
    {}

    ~LocalFunctionPacketStorage();

    const WRATHLayerNodeValuePackerBase::function_packet*
    fetch(const WRATHLayerNodeValuePackerBase::function_packet *v,
          const WRATHLayerNodeValuePackerBase::function_packet *f);

  private:
    typedef std::pair<const WRATHLayerNodeValuePackerBase::function_packet*,
                      const WRATHLayerNodeValuePackerBase::function_packet*> key_type;
    typedef WRATHLayerNodeValuePackerBase::function_packet *value_type;
    
    typedef std::map<key_type, value_type> map_type;

    WRATHMutex m_mutex;
    map_type m_data;
    
  };

}

///////////////////////////////////////////////
// LocalFunctionPacket methods
LocalFunctionPacket::SpecDataProcessedPayload::handle
LocalFunctionPacket::
create_handle(const ActiveNodeValuesCollection &spec) const
{
  WRATHLayerNodeValuePackerHybridImplement::Payload::handle p;
  NodeDataPackParametersCollection::packing_group vg, fg, vg_out, fg_out;

  p=WRATHNew WRATHLayerNodeValuePackerHybridImplement::Payload();


  
  p->m_vertex=create_handle_stage(GL_VERTEX_SHADER, spec, p, m_vertex);
  p->m_fragment=create_handle_stage(GL_FRAGMENT_SHADER, spec, p, m_fragment);
  

  p->m_number_slots=std::min(p->m_vertex->m_number_slots,
                             p->m_fragment->m_number_slots);

  return p;
}




void
LocalFunctionPacket::
append_fetch_code(WRATHGLShader::shader_source &src,
                  GLenum shader_stage,
                  const ActiveNodeValues &node_values,
                  const SpecDataProcessedPayload::handle &payload,
                  const std::string &index_name) const
{
  WRATHLayerNodeValuePackerHybridImplement::Payload::handle h;

  WRATHassert(payload.dynamic_cast_handle<WRATHLayerNodeValuePackerHybridImplement::Payload>().valid());
  h=payload.static_cast_handle<WRATHLayerNodeValuePackerHybridImplement::Payload>();

  switch(shader_stage)
    {
    default:
      WRATHassert(false);
      return;

    case GL_VERTEX_SHADER:
      m_vertex->append_fetch_code(src, shader_stage, node_values, h->m_vertex, index_name);
      break;

    case GL_FRAGMENT_SHADER:
      m_fragment->append_fetch_code(src, shader_stage, node_values, h->m_fragment, index_name);
      break;
    }

  h->m_number_slots=std::min(h->m_vertex->m_number_slots,
                             h->m_fragment->m_number_slots);  
}


void
LocalFunctionPacket::
add_actions(const SpecDataProcessedPayload::handle& payload,
            const ProcessedActiveNodeValuesCollection &spec,
            WRATHShaderSpecifier::ReservedBindings& reserved_bindings,
            WRATHGLProgramOnBindActionArray& actions,
            WRATHGLProgramInitializerArray& initers) const
{
  WRATHLayerNodeValuePackerHybridImplement::Payload::handle h;

  WRATHassert(payload.dynamic_cast_handle<WRATHLayerNodeValuePackerHybridImplement::Payload>().valid());
  h=payload.static_cast_handle<WRATHLayerNodeValuePackerHybridImplement::Payload>();

  /*
    generate m_vertex_spec and m_fragment_spec spec,
    we want to grab only those entries for each
    shader stage.
   */
  std::map<GLenum, ActiveNodeValues::Filter::const_handle> vs, fs;
  vs[GL_VERTEX_SHADER]=fs[GL_FRAGMENT_SHADER]=WRATHNew ActiveNodeValues::Filter();

  h->m_vertex_spec.set(h->m_vertex->m_packer_parameters,
                       spec.original_data(),
                       vs);


  h->m_fragment_spec.set(h->m_fragment->m_packer_parameters,
                         spec.original_data(),
                         fs);

    

  m_vertex->add_actions(h->m_vertex, h->m_vertex_spec, 
                        reserved_bindings, actions, initers);

  m_fragment->add_actions(h->m_fragment, h->m_fragment_spec,
                          reserved_bindings, actions, initers);

  h->m_number_slots=std::min(h->m_vertex->m_number_slots,
                             h->m_fragment->m_number_slots);  
}

////////////////////////////////////////////////////
// LocalFunctionPacketStorage methods
LocalFunctionPacketStorage::
~LocalFunctionPacketStorage()
{
  for(map_type::const_iterator 
        iter=m_data.begin(), 
        end=m_data.end();
      iter!=end; ++iter)
    {
      WRATHDelete(iter->second);
    }
}

const WRATHLayerNodeValuePackerBase::function_packet*
LocalFunctionPacketStorage::
fetch(const WRATHLayerNodeValuePackerBase::function_packet *v,
      const WRATHLayerNodeValuePackerBase::function_packet *f)
{
  WRATHAutoLockMutex(m_mutex);

  key_type K(v, f);
  map_type::iterator iter;

  iter=m_data.find(K);
  if(iter==m_data.end())
    {
      WRATHLayerNodeValuePackerBase::function_packet *R;

      R=WRATHNew LocalFunctionPacket(K.first, K.second);
      m_data[K]=R;

      return R;
    }
  else
    {
      return iter->second;
    }
}



//////////////////////////////////////////////////
// WRATHLayerNodeValuePackerHybridImplement methods
const WRATHLayerNodeValuePackerBase::function_packet*
WRATHLayerNodeValuePackerHybridImplement::
fetch_function_packet(const WRATHLayerNodeValuePackerBase::function_packet *VertexPacker,
                      const WRATHLayerNodeValuePackerBase::function_packet *FragmentPacker)
{
  WRATHStaticInit();
  static LocalFunctionPacketStorage R;

  WRATHassert(VertexPacker->supports_per_node_value(GL_VERTEX_SHADER));
  WRATHassert(FragmentPacker->supports_per_node_value(GL_FRAGMENT_SHADER));

  return R.fetch(VertexPacker, FragmentPacker);
}
