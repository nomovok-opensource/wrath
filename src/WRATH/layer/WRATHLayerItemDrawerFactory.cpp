/*! 
 * \file WRATHLayerItemDrawerFactory.cpp
 * \brief file WRATHLayerItemDrawerFactory.cpp
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
#include "WRATHLayerItemDrawerFactory.hpp"

namespace
{
  typedef std::map<GLenum, WRATHLayerNodeValuePackerBase::ActiveNodeValues::Filter::const_handle> local_map_type;

  /*
    returns invalid enum if there is no next stage,
    unfortantely, this is hackeyed since a GL program
    may or may not define all stages.
    For GL4.x the shader stages, in order are:
    
      vertex --> tesselation control --> tesselation evaluation --> geometry --> fragment

   */
  class PreviousShaderStage
  {
  public:

    PreviousShaderStage(const local_map_type &allowed_stages):
      m_logical_size(0)
    {
      
      add_stage(GL_VERTEX_SHADER, allowed_stages);

      #ifdef GL_TESS_CONTROL_SHADER
        {
          add_stage(GL_TESS_CONTROL_SHADER, allowed_stages);
        }
      #endif

      #ifdef GL_TESS_EVALUATION_SHADER
        {
          add_stage(GL_TESS_EVALUATION_SHADER, allowed_stages);
        }
      #endif

      #ifdef GL_GEOMETRY_SHADER
        {
          add_stage(GL_GEOMETRY_SHADER, allowed_stages);
        }
      #endif

      add_stage(GL_FRAGMENT_SHADER, allowed_stages);
    }

    void
    add_stage(GLenum v, const local_map_type &allowed_stages)
    {
      local_map_type::const_iterator iter(allowed_stages.find(v));
      if(iter!=allowed_stages.end())
        {
          m_shader_stages.push_back(v);
          if(iter->second.valid())
            {
              m_logical_size=std::max(m_logical_size, static_cast<unsigned int>(m_shader_stages.size()));
            }
        }
    }

    unsigned int m_logical_size;
    std::vector<GLenum> m_shader_stages;
  };


  
  
  class PropogateQuery:boost::noncopyable
  {
  public:
    PropogateQuery(const local_map_type &allowed_stages, const std::string &index_name):
      m_R(allowed_stages)
    {
      /*
        basic idea: 
          find all the stages that we need to worry
          about, from there get the next stage.

          We first need to find the _LAST_ shader stage
          within allowed_stages that requires a 
          node index, that is given by m_R.m_last_needed

          To get the index in/out names for each stage
          we just need to start walking at 0 and 
          proceed up to and including m_R.m_last_needed

          TODO:
            different shader stages do wierd things,
            for example geometry shader has an array since
            it gets N-outputs from vertex shader.
       */

      std::string current_index_name(index_name);
      for(unsigned int I=0; I<m_R.m_logical_size; ++I)
        {
          m_in_index_name[ m_R.m_shader_stages[I] ]=current_index_name;
         
          //last index does NOT propogate the value.
          if(I+1<m_R.m_logical_size)
            {
              current_index_name=current_index_name + "_sub";
              m_out_index_name[ m_R.m_shader_stages[I] ]=current_index_name;
            }
        }
    }
    

    PreviousShaderStage m_R;
    std::map<GLenum, std::string> m_in_index_name;
    std::map<GLenum, std::string> m_out_index_name;
  };

 
  


  /*
    TODO: have LocalFilter investigate the source code
    and then use that checking to determine if an
    ActiveNodeValue should really be absorbed.
   */
  class LocalFilter:public WRATHLayerNodeValuePackerBase::ActiveNodeValues::Filter
  {
  public:
    virtual
    bool
    absorb_active_node_value(const WRATHLayerNodeValuePackerBase::ActiveNodeValue&) const
    {
      return true;
    }
  };


  void
  note_available_shaders(const std::map<GLenum, WRATHGLShader::shader_source> &in_map,
                         const WRATHLayerNodeValuePackerBase::function_packet &node_packer_functions,
                         const WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection &spec,
                         std::ostream &inout_define_string_stream,
                         uint32_t &visited_inout_shaders_as_bits,
                         local_map_type &inout_shaders)
  {
    for(std::map<GLenum, WRATHGLShader::shader_source>::const_iterator 
          iter=in_map.begin(),
          end=in_map.end();
        iter!=end; ++iter)
      {
        uint32_t shader_bit(WRATHGLShader::gl_shader_bit(iter->first));
        

        if(0==(shader_bit&visited_inout_shaders_as_bits))
          {
            visited_inout_shaders_as_bits|=shader_bit;

            if(node_packer_functions.supports_per_node_value(iter->first) 
               and spec.entries().find(iter->first)!=spec.entries().end())
              {
                inout_shaders[iter->first]=WRATHNew LocalFilter();
                inout_define_string_stream << "\n#ifndef " 
                                           << "WRATH_"
                                           << WRATHGLShader::gl_shader_type_label(iter->first)
                                           << "_ITEM_VALUE_FETCH_OK"
                                           << "\n#define "
                                           << "WRATH_"
                                           << WRATHGLShader::gl_shader_type_label(iter->first)
                                           << "_ITEM_VALUE_FETCH_OK"
                                           << "\n#endif\n";
              
              }
            else 
              {
                inout_shaders[iter->first]=NULL;
              }
          }
      }
    
  }
}


WRATHMultiGLProgram*
WRATHLayerItemDrawerFactoryCommon::
generate_multi_glsl_program(const WRATHShaderSpecifier *shader_specifier,
                            const WRATHAttributePacker *attribute_packer,
                            enum clipping_implementation_type tp,
                            const WRATHLayerItemNodeBase::node_function_packet &node_functions,
                            const WRATHLayerNodeValuePackerBase::function_packet &node_packer_functions,
                            WRATHLayerNodeValuePackerBase::ProcessedActiveNodeValuesCollection &out_spec,
                            WRATHLayerNodeValuePackerBase::SpecDataProcessedPayload::handle &payload)
{
  std::map<GLenum, WRATHGLShader::shader_source> shader_srcs;
  WRATHGLShader::shader_source &vs(shader_srcs[GL_VERTEX_SHADER]);
  WRATHGLShader::shader_source &fs(shader_srcs[GL_FRAGMENT_SHADER]);
  WRATHGLProgramOnBindActionArray on_bind_actions;
  WRATHGLPreLinkActionArray attribute_bindings;
  std::string index_name("transf_index");
  local_map_type visited_stages_of_node_values;
  WRATHLayerNodeValuePackerBase::ActiveNodeValuesCollection in_spec;
  std::string item_fetch_supports_string;

  switch(tp)
    {
    default:
    case quad_clipping:
      break;

    case clip_vertex_clipping:
      vs.add_macro("WRATH_CLIP_VIA_CLIP_VERTEX");
      fs.add_macro("WRATH_CLIP_VIA_CLIP_VERTEX");
      /*
        TODO: create functor object to add
        to WRATHGLSLProgram to set clipping
        planes and to enable the first 4 clipping 
        planes.
       */
      break;

    case clip_distance_clipping:
      /*
        TODO: add functor object to add
        to WRATHGLSLProgram to enable the 
        first 4 clipping planes.
       */
      vs.add_macro("WRATH_CLIP_VIA_CLIP_DISTANCE");
      fs.add_macro("WRATH_CLIP_VIA_CLIP_DISTANCE");
      break;

    case clip_discard_clipping:
      vs.add_macro("WRATH_CLIP_VIA_DISCARD");
      fs.add_macro("WRATH_CLIP_VIA_DISCARD");
      break;
    }

  /*
    add attribute bindings of packer:
   */
  attribute_packer->bind_attributes(attribute_bindings);

  /*
    add attribute binding of our
    implicit attribute:
   */
  attribute_bindings
    .add_binding(index_name, attribute_packer->number_attributes());
    
  
  
  node_functions.add_per_node_values(in_spec, node_packer_functions);

  {
    uint32_t visited_stages_of_node_values_as_bits(0);
    std::ostringstream item_fetch_supports;

    note_available_shaders(shader_specifier->all_pre_shader_sources(),
                           node_packer_functions, in_spec, 
                           item_fetch_supports,
                           visited_stages_of_node_values_as_bits,
                           visited_stages_of_node_values);

    note_available_shaders(shader_specifier->all_shader_sources(),
                           node_packer_functions, in_spec,
                           item_fetch_supports,
                           visited_stages_of_node_values_as_bits,
                           visited_stages_of_node_values);

    item_fetch_supports_string=item_fetch_supports.str();
  }

  for(std::map<GLenum, WRATHLayerNodeValuePackerBase::ActiveNodeValues::Filter::const_handle>::iterator 
        iter=visited_stages_of_node_values.begin(),
        end=visited_stages_of_node_values.end(); 
      iter!=end; ++iter)
    {
      shader_srcs[iter->first].add_source(item_fetch_supports_string, WRATHGLShader::from_string);
    }

  for(std::map<GLenum, WRATHGLShader::shader_source>::const_iterator
        iter=shader_specifier->all_pre_shader_sources().begin(),
        end=shader_specifier->all_pre_shader_sources().end();
      iter!=end; ++iter)
    {
      shader_srcs[iter->first].absorb(iter->second);
    }

  payload=node_packer_functions.create_handle(in_spec);

  /*
    TODO:
      walk the code from shader_specifier->all_pre_shader_sources()
      and shader_specifier->all_shader_sources() checking what
      per-node values are really needed/used. Those that are not
      should then be ignored.
   */
  out_spec.set(payload->m_packer_parameters, 
	       in_spec, 
	       visited_stages_of_node_values);
               
  
  
  PropogateQuery propogate_query(visited_stages_of_node_values, index_name);
  
  for(local_map_type::iterator 
        siter=visited_stages_of_node_values.begin(),
        send=visited_stages_of_node_values.end(); 
      siter!=send; ++siter)
    {
      GLenum shader_stage(siter->first);
      std::ostringstream fake_main_addition;
      std::string in_index_name, out_index_name;

      /*
        defines the GLSL function pre_fetch_node_values(), this is done
        when iter->second is valid, indicating that we need to
        fetch node values.
       */
      in_index_name=propogate_query.m_in_index_name[shader_stage];
      out_index_name=propogate_query.m_out_index_name[shader_stage];

      if(siter->second.valid())
        {
          std::ostringstream declare_index;

          WRATHassert(!in_index_name.empty());
          declare_index << "\nshader_in mediump float " << in_index_name << ";\n";

          shader_srcs[shader_stage]
            .add_source(declare_index.str(), WRATHGLShader::from_string);

          WRATHassert(out_spec.shader_entries().find(shader_stage)!=out_spec.shader_entries().end());
          int idx(out_spec.shader_entries().find(shader_stage)->second);
          const WRATHLayerNodeValuePackerBase::ActiveNodeValues &v(out_spec.active_node_values(idx));


          node_packer_functions.append_fetch_code(shader_srcs[shader_stage], 
                                                  shader_stage, v, 
                                                  payload, in_index_name);
        }


      
      
      /*
        is it wise to declare the index as mediump? that essentially
        limits the number of nodes per call to 1024, which is a
        pretty damn big number of nodes per call anyways.
       */
      if(!out_index_name.empty())
        {
          fake_main_addition << "shader_out mediump float " << out_index_name << ";\n";
        }

      fake_main_addition << "\nvoid shader_main(void);"
                         << "\nvoid main(void)"
                         << "\n{";

      
      //only make the GLSL call to pre_fetch_node_values() if
      //there are node values to fetch
      if(siter->second.valid())
        {
          fake_main_addition << "\n\tpre_fetch_node_values();";
        }
          
      //forward the index to the next stage
      if(!out_index_name.empty())
        {
          fake_main_addition << "\n\t" << out_index_name << "=" << in_index_name << ";";
        }
          
      fake_main_addition << "\n\tshader_main();"
                         << "\n}";

      shader_srcs[shader_stage]
        .add_source(fake_main_addition.str(), WRATHGLShader::from_string);
        
    }



  node_functions.append_shader_source(shader_srcs, node_packer_functions);
  
  for(std::map<GLenum, WRATHGLShader::shader_source>::const_iterator
        iter=shader_specifier->all_shader_sources().begin(),
        end=shader_specifier->all_shader_sources().end();
      iter!=end; ++iter)
    {
      shader_srcs[iter->first].absorb(iter->second);
    }

     
  on_bind_actions
    .absorb(shader_specifier->bind_actions());

  WRATHGLProgramInitializerArray initers;

  initers.absorb(shader_specifier->initializers());

  /*
    add the initers and on bind actions _AFTER_ the shader code
   */
  WRATHShaderSpecifier::ReservedBindings bindings(shader_specifier->bindings());
  node_packer_functions.add_actions(payload, out_spec,
                                    bindings, 
                                    on_bind_actions, initers);


  WRATHMultiGLProgram *pr;

  std::ostringstream program_name;

  program_name << "[ ShaderSpecifier=\"" << shader_specifier->resource_name()
               << "\", node_packer=\""
               << typeid(node_packer_functions).name()
               << "\", node_functions=\"" << typeid(node_functions).name()
               << "\" ]";

  pr=WRATHNew WRATHMultiGLProgram(program_name.str(),
                                  shader_srcs,
                                  attribute_bindings,
                                  initers,
                                  on_bind_actions);

  return pr;

}
