/*! 
 * \file WRATHUniformData.cpp
 * \brief file WRATHUniformData.cpp
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
#include "WRATHUniformData.hpp"

/////////////////////////////
// WRATHUniformData::uniform_by_name_base methods
WRATHUniformData::uniform_by_name_base::
uniform_by_name_base(const std::string &uniform_name):
  m_pr(NULL),
  m_uniform_name(uniform_name),
  m_location(-1)
{}

void
WRATHUniformData::uniform_by_name_base::
gl_command(WRATHGLProgram *pr) 
{
  if(m_pr!=pr)
    {
      std::map<WRATHGLProgram*, GLint>::iterator iter;

      m_pr=pr;
      iter=m_location_map.find(m_pr);
      if(iter!=m_location_map.end())
        {
          m_location=iter->second;
        }
      else if(m_pr!=NULL)
        {
          if(m_pr->link_success())
            {
              m_location=glGetUniformLocation(pr->name(), m_uniform_name.c_str());
            }
          else
            {
              m_location=-1;
            }
            

          m_location_map[m_pr]=m_location;

          if(m_location==-1)
            {
              WRATHwarning("Unable to find uniform \""
                           << m_uniform_name
                           << " in program "
                           << m_pr->resource_name());
            }
        }
    }

  if(m_pr!=NULL and m_location!=-1)
    {
      set_uniform_value(m_location);
    }
}


////////////////////////////////////
// WRATHUniformData methods
void
WRATHUniformData::
add_uniform(const uniform_setter_base::handle &p)
{
  WRATHassert(p.valid());
  m_uniforms.insert(p);    
}


enum return_code
WRATHUniformData::
remove_uniform(const uniform_setter_base::handle &h)
{
  std::set<uniform_setter_base::handle>::iterator iter;

  iter=m_uniforms.find(h);
  if(iter!=m_uniforms.end())
    {
      m_uniforms.erase(iter);
      return routine_success;
    }
  return routine_fail;
}


void
WRATHUniformData::
execute_gl_commands(WRATHGLProgram *pr) const
{
   for(std::set<uniform_setter_base::handle>::const_iterator 
        iter=m_uniforms.begin(), end=m_uniforms.end();
      iter!=end; ++iter)
     {
       (*iter)->gl_command(pr);
     }
}


bool
WRATHUniformData::
different(const WRATHUniformData::const_handle &v0,
          const WRATHUniformData::const_handle &v1)
{
  if(v0==v1)
    {
      return false;
    }

  if(v0.valid() and v1.valid())
    {
      return v0->m_uniforms!=v1->m_uniforms;
    }

  return true;
}

bool
WRATHUniformData::
compare(const WRATHUniformData::const_handle &lhs,
        const WRATHUniformData::const_handle &rhs)
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

  return lhs->m_uniforms < rhs->m_uniforms;
}
