/*! 
 * \file WRATHShaderSpecifier.cpp
 * \brief file WRATHShaderSpecifier.cpp
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
#include "WRATHShaderSpecifier.hpp"

namespace
{
  typedef const char *cstring;  
}


////////////////////////////////////
// WRATHShaderSpecifier::key_type method
bool
WRATHShaderSpecifier::key_type::
operator<(const key_type &rhs) const
{
  if(m_sub_drawer_id!=rhs.m_sub_drawer_id)
    {
      return m_sub_drawer_id<rhs.m_sub_drawer_id;
    }

  if(m_item_group_drawer_type!=rhs.m_item_group_drawer_type)
    {
      return m_item_group_drawer_type.before(rhs.m_item_group_drawer_type);
    }


  return m_attribute_names<rhs.m_attribute_names;
}

//////////////////////////////////////
//WRATHShaderSpecifier::multi_pass_key_type method
bool
WRATHShaderSpecifier::multi_pass_key_type::
operator<(const multi_pass_key_type &rhs) const
{
  if(m_has_transparent_pass!=rhs.m_has_transparent_pass)
    {
      return m_has_transparent_pass<rhs.m_has_transparent_pass;
    }

  if(m_multi_draw_type!=rhs.m_multi_draw_type)
    {
      return m_multi_draw_type.before(rhs.m_multi_draw_type);
    }

  return m_key<rhs.m_key;
    
}

////////////////////////////////////
// WRATHShaderSpecifier::ReservedBindings methods
WRATHShaderSpecifier::ReservedBindings&
WRATHShaderSpecifier::ReservedBindings::
absorb(const ReservedBindings &obj)
{
  std::copy(obj.m_texture_binding_points.begin(),
            obj.m_texture_binding_points.end(),
            std::inserter(m_texture_binding_points, m_texture_binding_points.begin()) );
  std::copy(obj.m_buffer_binding_points.begin(),
            obj.m_buffer_binding_points.end(),
            std::inserter(m_buffer_binding_points, m_buffer_binding_points.begin()) );

  return *this;
}

//////////////////////////////////////////
// WRATHShaderSpecifier methods
WRATH_RESOURCE_MANAGER_IMPLEMENT(WRATHShaderSpecifier, 
                                 WRATHShaderSpecifier::ResourceKey);

WRATHShaderSpecifier::
WRATHShaderSpecifier(const ResourceKey &pname,
                     const WRATHGLShader::shader_source &vs,
                     const WRATHGLShader::shader_source &fs,
                     const Initializer &initers,
                     const WRATHGLProgramOnBindActionArray &on_bind_actions):
  m_resource_name(pname),
  m_remove_from_manager(true),
  m_initializers(initers.m_initializers),
  m_bind_actions(on_bind_actions),
  m_bindings(initers.m_bindings),
  m_modifiable(true),
  m_translucent_threshold(0.9f),
  m_sub_shader_specifiers(NULL, NULL, NULL),
  m_master(this)
{
  append_vertex_shader_source()=vs;
  append_fragment_shader_source()=fs;
  resource_manager().add_resource(m_resource_name, this);
}

WRATHShaderSpecifier::
WRATHShaderSpecifier(const WRATHGLShader::shader_source &vs,
                     const WRATHGLShader::shader_source &fs,
                     const Initializer &initers,
                     const WRATHGLProgramOnBindActionArray &on_bind_actions):
  m_resource_name(),
  m_remove_from_manager(false),
  m_initializers(initers.m_initializers),
  m_bind_actions(on_bind_actions),
  m_bindings(initers.m_bindings),
  m_modifiable(true),
  m_translucent_threshold(0.9f),
  m_sub_shader_specifiers(NULL, NULL, NULL),
  m_master(this)
{
  append_vertex_shader_source()=vs;
  append_fragment_shader_source()=fs;
}



WRATHShaderSpecifier::
WRATHShaderSpecifier(const std::string &macro,
                     const WRATHShaderSpecifier *parent):
  m_resource_name(parent->m_resource_name + "-sub_shader: " + macro),
  m_remove_from_manager(false),
  m_shader_source_code(parent->m_shader_source_code),
  m_pre_shader_source_code(parent->m_pre_shader_source_code),
  m_initializers(parent->m_initializers),
  m_bind_actions(parent->m_bind_actions),
  m_bindings(parent->m_bindings),
  m_modifiable(false),
  m_translucent_threshold(parent->m_translucent_threshold),
  m_sub_shader_specifiers(NULL, NULL, NULL),
  m_master(parent)
{
  
  for(std::map<GLenum, WRATHGLShader::shader_source>::iterator
        iter=m_shader_source_code.begin(), end=m_shader_source_code.end();
      iter!=end; ++iter)
    {
      iter->second.add_macro(macro, "", WRATHGLShader::push_front);
      iter->second.add_macro("WRATH_TRANSLUCENT_THRESHOLD", 
                             parent->m_translucent_threshold, 
                             WRATHGLShader::push_front);
    }
}


WRATHShaderSpecifier::
~WRATHShaderSpecifier()
{
  if(m_remove_from_manager)
    {
      resource_manager().remove_resource(this);
    }

  if(m_sub_shader_specifiers[0]!=NULL)
    {
      for(int i=0; i<3; ++i)
        {
          WRATHDelete(m_sub_shader_specifiers[i]);
        }
    }

  for(item_drawer_map::iterator iter=m_drawers.begin(),
        end=m_drawers.end(); iter!=end; ++iter)
    {
      iter->second.second.disconnect();
    }

  for(two_pass_drawer_map::iterator iter=m_two_pass_drawers.begin(),
        end=m_two_pass_drawers.end(); iter!=end; ++iter)
    {
      iter->second.second.disconnect();
    }
  
}


const WRATHShaderSpecifier&
WRATHShaderSpecifier::
fetch_sub_shader(enum WRATHTwoPassDrawer::drawing_pass_type tp) const
{
  m_master->ready_sub_shaders();
  return *m_master->m_sub_shader_specifiers[tp];
}



void
WRATHShaderSpecifier::
add_shader_source_code(const WRATHBaseSource *src,
                       enum WRATHBaseSource::precision_t prec,
                       const std::string &suffix)
{
  src->add_shader_source_code(append_all_shader_sources(),
                              prec, suffix);
                              
}


WRATHItemDrawer*
WRATHShaderSpecifier::
fetch_drawer(const WRATHItemDrawerFactory &factory,
             const WRATHAttributePacker *attribute_packer,
             int sub_drawer_id) const
{
  WRATHAutoLockMutex(m_mutex);

  item_drawer_map::iterator iter;
  key_type K(typeid(factory),
             attribute_packer,
             sub_drawer_id);
  
  m_modifiable=false;

  iter=m_drawers.find(K);
  if(iter!=m_drawers.end())
    {
      return iter->second.first;
    }
  else
    {  
      std::pair<item_drawer_map::iterator, bool> B;
      
      B=m_drawers.insert(item_drawer_map::value_type(K, per_item_drawer()));
      WRATHassert(B.second);

      per_item_drawer &I(B.first->second);
      I.first=factory.generate_drawer(this, attribute_packer, sub_drawer_id);
      I.second=I.first->connect_dtor(boost::bind(&WRATHShaderSpecifier::on_item_draw_dtor, 
                                                 this, B.first) );

      return I.first;
    }
}

void
WRATHShaderSpecifier::
on_item_draw_dtor(item_drawer_map::iterator iter) const
{
  WRATHAutoLockMutex(m_mutex);
  m_drawers.erase(iter);
}

void
WRATHShaderSpecifier::
on_two_pass_draw_dtor(two_pass_drawer_map::iterator iter) const
{
  WRATHAutoLockMutex(m_mutex);
  m_two_pass_drawers.erase(iter);
}

void
WRATHShaderSpecifier::
ready_sub_shaders(void) const
{
  WRATHassert(m_master==this);
  WRATHAutoLockMutex(m_mutex);

  const cstring draw_pass_labels[]=
    {
      "WRATH_IS_OPAQUE_PASS", //for opaque_draw_pass
      "WRATH_IS_TRANSLUCENT_PASS", //for transluscent_draw_pass
      "WRATH_IS_PURE_TRANSLUCENT_PASS", //for pure_transluscent
    };

  m_modifiable=false;
  if(m_sub_shader_specifiers[0]==NULL)
    {
      WRATHassert(m_sub_shader_specifiers[1]==NULL);
      WRATHassert(m_sub_shader_specifiers[2]==NULL);

      for(int i=0; i<3; ++i)
        {
          m_sub_shader_specifiers[i]=WRATHNew WRATHShaderSpecifier(draw_pass_labels[i], this);
        }
    }
}


  
