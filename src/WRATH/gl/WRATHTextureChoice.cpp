/*! 
 * \file WRATHTextureChoice.cpp
 * \brief file WRATHTextureChoice.cpp
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
#include "WRATHTextureChoice.hpp"

////////////////////////////
// WRATHTextureChoice::texture_base methods
void
WRATHTextureChoice::texture_base::
unbind_texture(GLenum) 
{}

WRATHUniformData::uniform_setter_base::handle
WRATHTextureChoice::texture_base::
texture_size(const std::string&)
{
  return WRATHUniformData::uniform_setter_base::handle();
}

////////////////////////////
// WRATHTextureChoice::texture methods
void
WRATHTextureChoice::texture::
bind_texture(GLenum) 
{
  glBindTexture(binding_point(), m_texture_name);
}

/////////////////////////////////////
// WRATHTextureChoice methods

void
WRATHTextureChoice::
add_texture(GLenum tex_unit, texture_base::handle ptex)
{
  WRATHassert(tex_unit>=GL_TEXTURE0);
  WRATHassert(tex_unit<
              GL_TEXTURE0 + static_cast<GLenum>(WRATHglGet<GLint>(GL_MAX_TEXTURE_IMAGE_UNITS)));

  m_values[tex_unit]=ptex;
}

void
WRATHTextureChoice::
remove_texture(GLenum tex_unit)
{
  std::map<GLenum, texture_base::handle>::iterator iter;
  iter=m_values.find(tex_unit);
  if(iter!=m_values.end())
    {
      m_values.erase(iter);
    }
}

int
WRATHTextureChoice::
bind_textures(const const_handle &h) const
{
  if(h.valid())
    {
      std::map<GLenum, texture_base::handle>::const_iterator i1, i2, e1, e2;
      int return_value(0);

      i1=h->m_values.begin();
      i2=m_values.begin();
      e1=h->m_values.end(); 
      e2=m_values.end();
      
      /*
        Ugly comment: 

        We do NOT detect if a fixed texture goes from
        being bound to one unit to a different unit.

        A better API might be to introduce a lock()
        and unlock() methods to bind_textures()
        and to have an std::map<texture_base::handle, count>
        which is used to correctly call lock() and unlock().
       */
      while(i1!=e1 and i2!=e2)
        {
          if(i1->first==i2->first)
            {
              if(i1->second!=i2->second)
                {
                  GLenum unit(i1->first);

                  glActiveTexture(unit);
                  i1->second->unbind_texture(unit);
                  i2->second->bind_texture(unit);
                  ++return_value;
                }
              ++i1;
              ++i2;
            }
          else if(i1->first<i2->first)
            {
              for(;i1!=e1 and i1->first<i2->first; ++i1)
                {
                  GLenum unit(i1->first);
                  i1->second->unbind_texture(unit);
                } 
            }
          else if(i2->first<i1->first)
            {
              for(;i2!=e2 and i2->first<i1->first; ++i2)
                {
                  GLenum unit(i2->first);

                  glActiveTexture(unit);
                  i2->second->bind_texture(unit);
                  ++return_value;
                } 
            }
        }

      for(;i1!=e1;++i1)
        {
          GLenum unit(i1->first);

          i1->second->unbind_texture(unit);
        }

      for(;i2!=e2; ++i2)
        {
          GLenum unit(i2->first);
          
          glActiveTexture(unit);
          i2->second->bind_texture(unit);
          ++return_value;
        } 

      return return_value;
    }
  else
    {
      for(std::map<GLenum, texture_base::handle>::const_iterator 
            iter=m_values.begin(), end=m_values.end();
          iter!=end; ++iter)
        {    
          glActiveTexture(iter->first);
          iter->second->bind_texture(iter->first);
        }
      return m_values.size();
    }

}

void
WRATHTextureChoice::
unbind_textures(void) const
{

  for(std::map<GLenum, texture_base::handle>::const_iterator 
        iter=m_values.begin(), end=m_values.end();
      iter!=end; ++iter)
    {    
      iter->second->unbind_texture(iter->first);
    }
}



bool
WRATHTextureChoice::
different(const WRATHTextureChoice::const_handle &v0,
          const WRATHTextureChoice::const_handle &v1)
{
  if(v0==v1)
    {
      return false;
    }

  if(v0.valid() and v1.valid())
    {
      return v0->m_values!=v1->m_values;
    }

  return true;
}

bool
WRATHTextureChoice::
compare(const WRATHTextureChoice::const_handle &lhs,
        const WRATHTextureChoice::const_handle &rhs)
{
  if(lhs==rhs)
    {
      return false;
    }

  if(!lhs.valid())
    {
      return true;
    }

  if(!rhs.valid())
    {
      return false;
    }

  return lhs->m_values < rhs->m_values;
}
