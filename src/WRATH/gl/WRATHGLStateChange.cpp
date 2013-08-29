/*! 
 * \file WRATHGLStateChange.cpp
 * \brief file WRATHGLStateChange.cpp
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
#include <algorithm>
#include "WRATHGLStateChange.hpp"

void
WRATHGLStateChange::
add_state_change(state_change::handle st)
{
  std::pair<std::set<state_change::handle>::iterator, bool> R;

  WRATHassert(st.valid());
  R=m_state_changes.insert(st);

}

void
WRATHGLStateChange::
remove_state_change(state_change::handle st)
{
  WRATHassert(st.valid());
  WRATHassert(m_state_changes.find(st)!=m_state_changes.end());
  m_state_changes.erase(st);
}
  
int
WRATHGLStateChange::
set_state(const const_handle &prev_value,
          WRATHGLProgram *program) const
{
  int return_value(0);
  
  if(prev_value.valid())
    {
      std::set<state_change::handle>::const_iterator i1, i2, e1, e2;

      /*
        call restore_state for those
        handle in prev_value that are
        not in this and call set_state
        for those handles that are in
        this but not in prev_value.
       */
      i1=prev_value->m_state_changes.begin();
      i2=m_state_changes.begin();

      e1=prev_value->m_state_changes.end();
      e2=m_state_changes.end();

      while(i1!=e1 and i2!=e2)
        {
          if(*i1<*i2)
            {
              (*i1)->restore_state(program);
              ++i1;
              ++return_value;
            }
          else if(*i2<*i1)
            {
              (*i2)->set_state(program);
              ++i2;
              ++return_value;
            }
          else
            {
              ++i2;
              ++i1;
            }
        }

      for(;i1!=e1; ++i1, ++return_value)
        {
          (*i1)->restore_state(program);
        }

      for(;i2!=e2; ++i2, ++return_value)
        {
          (*i2)->set_state(program);
        }

    }
  else
    {
      for(std::set<state_change::handle>::const_iterator 
            iter=m_state_changes.begin(),
            end=m_state_changes.end();
          iter!=end; ++iter, ++return_value)
        {
          (*iter)->set_state(program);
        }
    }

  return return_value;
}

bool
WRATHGLStateChange::
different(const WRATHGLStateChange::const_handle &v0,
          const WRATHGLStateChange::const_handle &v1)
{
  if(v0==v1)
    {
      return false;
    }

  if(v0.valid() and v1.valid())
    {
      return v0->m_state_changes!=v1->m_state_changes;
    }

  return true;
}




bool
WRATHGLStateChange::
compare(const WRATHGLStateChange::const_handle &lhs,
        const WRATHGLStateChange::const_handle &rhs)
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

  return lhs->m_state_changes < rhs->m_state_changes;
}
