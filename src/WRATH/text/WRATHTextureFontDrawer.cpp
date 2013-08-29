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
  class FontSizeMagic:public WRATHUniformData::uniform_setter_base
  {
  public:
    FontSizeMagic(WRATHTextureFont *font, int texture_page, const std::string &pname):
      m_name(pname),
      m_ready(false),
      m_texture_reciprocal_location(-1),
      m_font(font),
      m_texture_page(texture_page),
      m_both_there(false)
    {}

    virtual
    void
    gl_command(WRATHGLProgram *pr) 
    {
      if(!m_ready)
        {
          m_ready=true;
          WRATHGLProgram::attribute_uniform_query_result u;

          u=pr->find_uniform(m_name);
          if(u.m_info!=NULL)
            {
              m_both_there=(u.m_info->m_count>=2);
              m_texture_reciprocal_location=u.m_info->m_location;
            }

          if(m_texture_reciprocal_location==-1)
            {
              WRATHwarning("\nUnable to find texture size uniform \"" << m_name
                           << "\" in WRATHGLProgram \"" << pr->resource_name()
                           << "\"\n");
                           
            }
        }

      if(m_texture_reciprocal_location==-1)
        {
          return;
        }

      if(!m_both_there)
        {
          vec2 recip_sz(m_font->texture_size_reciprocal(m_texture_page,
                                                        WRATHTextureFont::native_value));
          
          glUniform2fv(m_texture_reciprocal_location, 1, recip_sz.c_ptr());
        }
      else
        {
          vecN<vec2, 2> recip_szs(m_font->texture_size_reciprocal(m_texture_page,
                                                                  WRATHTextureFont::native_value),
                                  m_font->texture_size_reciprocal(m_texture_page,
                                                                  WRATHTextureFont::minified_value));
          glUniform2fv(m_texture_reciprocal_location, 2, recip_szs[0].c_ptr());

        }
    }


  private:
    std::string m_name;
    bool m_ready;
    GLint m_texture_reciprocal_location;
    WRATHTextureFont *m_font;
    int m_texture_page;
    bool m_both_there;
  };

 

}


//////////////////////////////////////////
// WRATHTextureFontDrawer::per_type methods
WRATHTextureFontDrawer::per_type::
per_type(const std::string &pname):
  m_name(pname)
{
}

WRATHTextureFontDrawer::per_type::
~per_type()
{}

WRATHUniformData::uniform_setter_base::handle
WRATHTextureFontDrawer::per_type::
texture_size_uniform(WRATHTextureFont *v, int p)
{
  
  WRATHassert(v!=NULL);
  
  //build as needed:
  std::map<map_key, map_value>::iterator iter;
  WRATHUniformData::uniform_setter_base::handle pWRATHNewValue;
  
  iter=m_map.find(map_key(v,p));
  if(iter!=m_map.end())
    {
      return iter->second;
    }

  pWRATHNewValue=WRATHNew FontSizeMagic(v, p, m_name);
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

  m_passes[opaque_draw_pass]=WRATHNew per_type("reciprocal_texture_size");

  if(ptranslucent_drawer!=NULL)
    {
      m_passes[transluscent_draw_pass]=WRATHNew per_type("reciprocal_texture_size");
    }
  else
    {
      m_passes[transluscent_draw_pass]=NULL;
    }

  m_passes[pure_transluscent]=WRATHNew per_type("reciprocal_texture_size");

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



  

