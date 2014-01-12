/*! 
 * \file WRATHTextureFontDrawer.cpp
 * \brief file WRATHTextureFontDrawer.cpp
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
#include "WRATHassert.hpp" 
#include "WRATHTextureFontDrawer.hpp"

namespace
{
  class TexturePageDataUniform:public WRATHUniformData::uniform_setter_base
  {
  public:
    TexturePageDataUniform(WRATHTextureFont *font, int texture_page):
      m_ready(false),
      m_location(-1),
      m_font(font),
      m_texture_page(texture_page),
      m_size(-1)
    {}

    virtual
    void
    gl_command(WRATHGLProgram *pr) 
    {
      if(!m_ready)
        {
          m_ready=true;
          WRATHGLProgram::attribute_uniform_query_result u;

          //this must match the uniform found in the built in GLSL
          //found in font_shader_texture_page_data.wrath-shader.glsl
          u=pr->find_uniform("wrath_font_page_data_uniforms");
          if(u.m_info!=NULL)
            {
              m_size=std::min(m_font->texture_page_data_size(), u.m_info->m_count);
              m_location=u.m_info->m_location;
              m_values.resize(m_size, 0.0f);
              for(int i=0; i<m_size; ++i)
                {
                  m_values[i]=m_font->texture_page_data(m_texture_page, i);
                }
            }

          if(m_location==-1)
            {
              WRATHwarning("\nUnable to find texture page data uniform "
                           << "in WRATHGLProgram \"" << pr->resource_name()
                           << "\"\n");
                           
            }
        }

      if(m_location==-1 or m_size<=0)
        {
          return;
        }

      glUniform1fv(m_location, m_size, &m_values[0]);
    }


  private:
    bool m_ready;
    GLint m_location;
    WRATHTextureFont *m_font;
    int m_texture_page;
    int m_size;
    std::vector<float> m_values;
  };

 

}


//////////////////////////////////////////
// WRATHTextureFontDrawer::per_type methods
WRATHTextureFontDrawer::per_type::
per_type(void)
{
}

WRATHTextureFontDrawer::per_type::
~per_type()
{}

WRATHUniformData::uniform_setter_base::handle
WRATHTextureFontDrawer::per_type::
texture_page_data_uniform(WRATHTextureFont *v, int p)
{
  
  WRATHassert(v!=NULL);
  
  //build as needed
  WRATHAutoLockMutex(m_mutex);

  std::map<map_key, map_value>::iterator iter;
  WRATHUniformData::uniform_setter_base::handle pWRATHNewValue;
  
  iter=m_map.find(map_key(v,p));
  if(iter!=m_map.end())
    {
      return iter->second;
    }

  pWRATHNewValue=WRATHNew TexturePageDataUniform(v, p);
  m_map[ map_key(v,p) ]=pWRATHNewValue;

  return pWRATHNewValue;
}


///////////////////////////////////////////////
// WRATHTextureFontDrawer methods

WRATHTextureFontDrawer::
WRATHTextureFontDrawer(const WRATHTextureFontDrawer::ResourceKey &pname,
                       WRATHItemDrawer *popaque_drawer,
                       WRATHItemDrawer *ptranslucent_drawer,
                       WRATHItemDrawer *ptranslucent_drawer_standalone):
  WRATHTwoPassDrawer(pname, popaque_drawer, ptranslucent_drawer, ptranslucent_drawer_standalone)
{
  init(popaque_drawer,
       ptranslucent_drawer,
       ptranslucent_drawer_standalone);
}


WRATHTextureFontDrawer::
WRATHTextureFontDrawer(WRATHItemDrawer *popaque_drawer,
                       WRATHItemDrawer *ptranslucent_drawer,
                       WRATHItemDrawer *ptranslucent_drawer_standalone):
  WRATHTwoPassDrawer(popaque_drawer, ptranslucent_drawer, ptranslucent_drawer_standalone)
{
  init(popaque_drawer,
       ptranslucent_drawer,
       ptranslucent_drawer_standalone);
}


void
WRATHTextureFontDrawer::
init(WRATHItemDrawer *popaque_drawer,
     WRATHItemDrawer *ptranslucent_drawer,
     WRATHItemDrawer *ptranslucent_drawer_standalone)
{
  WRATHunused(popaque_drawer);
  WRATHunused(ptranslucent_drawer_standalone);

  m_passes[opaque_draw_pass]=WRATHNew per_type();

  if(ptranslucent_drawer!=NULL)
    {
      m_passes[transluscent_draw_pass]=WRATHNew per_type();
    }
  else
    {
      m_passes[transluscent_draw_pass]=NULL;
    }

  m_passes[pure_transluscent]=WRATHNew per_type();

}


WRATHTextureFontDrawer::
~WRATHTextureFontDrawer()
{
  WRATHDelete(m_passes[opaque_draw_pass]);
  WRATHDelete(m_passes[pure_transluscent]);

  if(m_passes[transluscent_draw_pass]!=NULL)
    {
      WRATHDelete(m_passes[transluscent_draw_pass]);
    }
}



  

