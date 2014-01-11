/*! 
 * \file WRATHLayerNodeValuePackerUniformArrays.cpp
 * \brief file WRATHLayerNodeValuePackerUniformArrays.cpp
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
  
  0) In GLES2 (see the appendix/back of the GLSL specification for why)
     a uniform array of floats (uniform name[N]) usually takes the same
     amount of uniform room as an array of vec4's of the same length
     (uniform vec4 name[N]) even though the latter has 4 times as many
     floats. Because of this, we will pack our float values into an
     array of vec4's. The packing is so that the values for a fixed
     node are continuous and we pad the values to that the next node
     starts at the beginning of the next vec4.

  1) For each entry in the passed ActiveNodeValuesCollection,
     we have a float (non-uniform) that fetches the 
     value from the massive uniform array. This fetching
     is done in the GLSL function WRATH_LAYER_UNIFORM_PACKER_UNIFORM_ARRAYS_fetch_values()

  2) We generate the main which is just WRATH_LAYER_UNIFORM_PACKER_UNIFORM_ARRAYS_fetch_values()
     followed by WRATH_LAYER_UNIFORM_PACKER_UNIFORM_ARRAYS_real_main()

  3) The GLSL code then #define's main as tWRATH_LAYER_UNIFORM_PACKER_UNIFORM_ARRAYS_real_main 
     so that subsequent code that has a main actually defines tWRATH_LAYER_UNIFORM_PACKER_UNIFORM_ARRAYS_real_main

  4) We need only one uniform, that array of vec4's which has it's values
     set as WRATHLayerNodeValuePackerUniformArrays::data_to_pack_to_GL()
*/


namespace
{
  static 
  unsigned int&
  size_of_vec4_array_int(void)
  {
    static unsigned int R(200);
    return R;
  }

  class UniformArrayFunction:public WRATHLayerNodeValuePackerBase::function_packet
  {
  public:
    typedef const char* c_string;

    int
    max_number_slots_allowed(int number_per_node_values) const
    {
      int modulas(number_per_node_values%4);
      int padding;

      /*
        pad number_per_node_values so it is a multiple of 4.
       */
      padding=(modulas!=0)?
        4-modulas:
        0;

      /*
        TODO: query GL how much uniform room there 
        is or have a static variable/function of 
        WRATHLayerNodeValuePackerUniformArrays
        that specifies how much room the per-item 
        uniform arrays are allowed to use.
       */
      int number_vec4s(WRATHLayerNodeValuePackerUniformArrays::size_of_vec4_array());
      int return_value;

      return_value=(4*number_vec4s)/(padding+number_per_node_values);

      return return_value;
    }

    virtual
    bool
    supports_per_node_value(GLenum shader_type) const
    {
      return shader_type==GL_VERTEX_SHADER;
    }

    virtual
    SpecDataProcessedPayload::handle
    create_handle(const ActiveNodeValuesCollection &) const
    {
      /*
        set return value's m_number_slots in
        append_uniform_fetch_code
       */
      return WRATHNew SpecDataProcessedPayload();
    }

    virtual
    void
    add_actions(const SpecDataProcessedPayload::handle& /*payload*/,
                const ProcessedActiveNodeValuesCollection&,
                WRATHShaderSpecifier::ReservedBindings& /*reserved_bindings*/,
                WRATHGLProgramOnBindActionArray& /*actions*/,
                WRATHGLProgramInitializerArray& /*initers*/) const
    {}

    void
    append_fetch_code(WRATHGLShader::shader_source &src,
                      GLenum shader_stage,
                      const ActiveNodeValues &node_values,
                      const SpecDataProcessedPayload::handle &hnd,
                      const std::string &index_name) const
    {
      WRATHunused(shader_stage);
      WRATHassert(shader_stage==GL_VERTEX_SHADER);

      std::ostringstream ostr;
      int padded_size(node_values.number_active());
      int modulas(padded_size%4);

      hnd->m_number_slots=max_number_slots_allowed(node_values.number_active());

      if(modulas!=0)
        {
          padded_size+= (4-modulas);
        }

      ostr << "\n\n#define fetch_node_value(X) X\n";
      
      for(ActiveNodeValues::map_type::const_iterator iter=node_values.entries().begin(),
            end=node_values.entries().end(); iter!=end; ++iter)
        {
          ostr << "\nfloat " << iter->second.label() << ";\n";
          /*
            TODO: handle aliases with #defines.
          */
          
        }

      /*
        TODO: should we specify a precision??
       */

      ostr << "\n\nuniform vec4 WRATH_LAYER_UNIFORM_PACKER_UNIFORM_ARRAYS"
           << "[" << hnd->m_number_slots*(padded_size/4) << "];\n\n"
           << "void pre_fetch_node_values(void)"
           << "\n{"
           << "\n\tint node_start_index;"
           << "\n\tnode_start_index=int("
           << index_name
           << ")*"
           << padded_size/4 << ";";
      
      for(ActiveNodeValues::map_type::const_iterator iter=node_values.entries().begin(),
            end=node_values.entries().end(); iter!=end; ++iter)
        {
          int component(iter->second.m_offset%4);
          const char component_name[]=
            {
              'x',
              'y',
              'z',
              'w'
            };
          
          ostr << "\n\t" << iter->second.label()
               << "=WRATH_LAYER_UNIFORM_PACKER_UNIFORM_ARRAYS[node_start_index+"
               << iter->second.m_offset/4 << "]." 
               << component_name[component] << ";";
        }
      ostr << "\n}\n\n";
      src.add_source(ostr.str(), WRATHGLShader::from_string);

    }
    
  };


  class local_uniform_type:public WRATHUniformData::uniform_by_name_base
  {
  public:
    local_uniform_type(WRATHLayerNodeValuePackerBase::DataToGL owner):
      WRATHUniformData::uniform_by_name_base("WRATH_LAYER_UNIFORM_PACKER_UNIFORM_ARRAYS"),
      m_active(true),
      m_owner(owner),
      m_not_first_time_called(0)
    {}

    void
    deactiveate(void)
    {
      m_active=false;
    }

    virtual
    void
    set_uniform_value(GLint location)
    {
      if(m_active)
        {
          vecN<const_c_array<float>, 2> datum(m_owner.data_to_pack_to_GL(),
                                              m_owner.data_to_pack_to_GL_restrict());
          const_c_array<vec4> casted_datum(datum[m_not_first_time_called].reinterpret_pointer<vec4>());

          WRATHglUniform(location, casted_datum);
          m_not_first_time_called=1;
        }
    }

  private:
    bool m_active;
    WRATHLayerNodeValuePackerBase::DataToGL m_owner;
    int m_not_first_time_called;
  };

}


///////////////////////////////////////////////
// WRATHLayerNodeValuePackerUniformArrays methods
WRATHLayerNodeValuePackerUniformArrays::
WRATHLayerNodeValuePackerUniformArrays(WRATHLayerBase *layer,
                                       const SpecDataProcessedPayload::const_handle &payload,
                                       const ProcessedActiveNodeValuesCollection &spec):
  WRATHLayerNodeValuePackerBase(layer, 
                                payload, spec)

{
  /*
    for now, just the vertex shader dude.
   */
  m_uniform=WRATHNew local_uniform_type(data_to_gl(GL_VERTEX_SHADER)); 
}

WRATHLayerNodeValuePackerUniformArrays::
~WRATHLayerNodeValuePackerUniformArrays()
{}

void
WRATHLayerNodeValuePackerUniformArrays::
phase_render_deletion(void)
{
  WRATHassert(m_uniform.dynamic_cast_handle<local_uniform_type>().valid());
  m_uniform.static_cast_handle<local_uniform_type>()->deactiveate();
  m_uniform=NULL;

  WRATHLayerNodeValuePackerBase::phase_render_deletion();
}
  
void
WRATHLayerNodeValuePackerUniformArrays::
append_state(WRATHSubItemDrawState &skey)
{
  skey.add_uniform(m_uniform);
}

const WRATHLayerNodeValuePackerBase::function_packet&
WRATHLayerNodeValuePackerUniformArrays::
functions(void)
{
  WRATHStaticInit();
  static UniformArrayFunction return_value;
  return return_value;
}

unsigned int
WRATHLayerNodeValuePackerUniformArrays::
size_of_vec4_array(void)
{
  return size_of_vec4_array_int();
}

void
WRATHLayerNodeValuePackerUniformArrays::
size_of_vec4_array(unsigned int v)
{
  size_of_vec4_array_int()=v;
}
