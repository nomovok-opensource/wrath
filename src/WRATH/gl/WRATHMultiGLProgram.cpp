/*! 
 * \file WRATHMultiGLProgram.cpp
 * \brief file WRATHMultiGLProgram.cpp
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
#include "WRATHMultiGLProgram.hpp"
#include "WRATHStaticInit.hpp"

namespace
{
  class SelectorHoard
  {
  public:
    typedef std::map<std::string, std::string> macro_values;

    SelectorHoard(void)
    {
      unsigned int v;

      /*
        insert 0 as the first entry into m_map
       */
      v=insert_entry(macro_values());
      WRATHassert(v==0);
      WRATHunused(v);
    }

    bool
    valid_ID(unsigned int V)
    {
      WRATHAutoLockMutex(m_mutex);
      return V<m_macro_reference.size();
    }

    unsigned int
    fetch(const macro_values &k)
    {
      if(k.empty())
        {
          return 0;
        }
      
      WRATHAutoLockMutex(m_mutex);
      map_type::const_iterator iter;
      iter=m_map.find(k);
      if(iter!=m_map.end())
        {
          return iter->second;
        }
      else
        {
          return insert_entry(k);
        }
    }
    
    const macro_values&
    macros(unsigned int V)
    {
      WRATHassert(m_map.find(*m_macro_reference[V])!=m_map.end());
      WRATHassert(m_map[ *m_macro_reference[V] ]==V);
      return *m_macro_reference[V];
    }

  private:
    typedef std::map<macro_values, unsigned int> map_type;

    unsigned int
    insert_entry(const macro_values &key)
    {
      std::pair<map_type::iterator, bool> R;
      unsigned int value(m_macro_reference.size());

      WRATHassert(m_macro_reference.size()==m_map.size());

      R=m_map.insert( map_type::value_type(key, value));
      WRATHassert(R.second==true);
      m_macro_reference.push_back(&R.first->first);

      return value;
    }

    WRATHMutex m_mutex;
    map_type m_map;
    std::vector<const macro_values*> m_macro_reference;
  };

  SelectorHoard&
  selector_hoard(void)
  {
    WRATHStaticInit();
    static SelectorHoard R;
    return R;
  }
}


//////////////////////////////////////
// WRATHMultiGLProgram::Selector methods
WRATHMultiGLProgram::Selector::
Selector(const std::map<std::string, std::string> &macros):
  m_ID(selector_hoard().fetch(macros))
{}

WRATHMultiGLProgram::Selector::
Selector(const macro_collection &macros):
  m_ID(selector_hoard().fetch(macros.m_macros))
{}

const std::map<std::string, std::string>&
WRATHMultiGLProgram::Selector::
macro_list(void) const
{
  return selector_hoard().macros(m_ID);
}

//////////////////////////////////////
// WRATHMultiGLProgram methods
WRATH_RESOURCE_MANAGER_IMPLEMENT(WRATHMultiGLProgram, std::string)

WRATHMultiGLProgram::
~WRATHMultiGLProgram()
{
  for(std::vector<per_program>::iterator iter=m_programs.begin(),
        end=m_programs.end(); iter!=end; ++iter)
    {
      if(iter->first!=NULL)
        {
          iter->second.disconnect();
        }
    }
  m_dtor_signal();
  resource_manager().remove_resource(this);
}

void
WRATHMultiGLProgram::
register_resource(const std::string &pname)
{
  m_resource_name=pname;
  resource_manager().add_resource(pname, this);
}

void
WRATHMultiGLProgram::
on_program_delete(unsigned int ID) const
{
  WRATHAutoLockMutex(m_mutex);
  WRATHassert(m_programs[ID].first!=NULL);
  m_programs[ID].second.disconnect();
  m_programs[ID].first=NULL;
}


WRATHGLProgram*
WRATHMultiGLProgram::
fetch_program(Selector selector) const
{
  unsigned int id(selector.m_ID);
  
  WRATHassert(selector_hoard().valid_ID(id));

  /*
    Do we really need a lock, this function is only
    called by WRATHRawDrawData from the rendering thread,
    thus we really do not need to worry about this, unless 
    - a user fetches by hand to verify a shader
    - there are multiple rendering threads
   */

  WRATHAutoLockMutex(m_mutex);
  if(id<m_programs.size() and m_programs[id].first!=NULL)
    {
      return m_programs[id].first;
    }

  if(id>=m_programs.size())
    {
      m_programs.resize(id+1, 
                        per_program(NULL, boost::signals2::connection()) );
    }

  WRATHGLProgram *pr;
  std::map<GLenum, WRATHGLShader::shader_source> src(m_shader_source_code);
  const std::map<std::string, std::string> &macros(selector.macro_list());

  for(std::map<GLenum, WRATHGLShader::shader_source>::iterator 
        iter=src.begin(),
        end=src.end(); 
      iter!=end; ++iter)
    {
      for(std::map<std::string, std::string>::const_reverse_iterator 
            macro_iter=macros.rbegin(),
            macro_end_iter=macros.rend();
          macro_iter!=macro_end_iter; ++macro_iter)
        {
          iter->second.add_macro(macro_iter->first, macro_iter->second, WRATHGLShader::push_front); 
        }
    }

  std::ostringstream program_name;
  program_name << resource_name() << "[Selector=" << id << "]"; 
  pr=WRATHNew WRATHGLProgram(program_name.str(), src, m_actions, m_initers, m_bind_actions);

  m_programs[id].first=pr;
  m_programs[id].second=pr->connect_dtor(boost::bind(&WRATHMultiGLProgram::on_program_delete, this, id));
  return pr;          
}
