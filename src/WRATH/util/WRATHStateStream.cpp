/*! 
 * \file WRATHStateStream.cpp
 * \brief file WRATHStateStream.cpp
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
#include "WRATHStateStream.hpp"

WRATHStateStream::
~WRATHStateStream()
{
  for(std::map<key_type, array_holder_base*>::iterator 
        iter=m_runtime_arrays.begin(), end=m_runtime_arrays.end();
      iter!=end; ++iter)
    {
      WRATHDelete(iter->second);
    }
}


void
WRATHStateStream::
reset(void)
{
  m_time_location=0;
  for(std::map<key_type, array_holder_base*>::iterator 
        iter=m_runtime_arrays.begin(), end=m_runtime_arrays.end();
      iter!=end; ++iter)
    {
      WRATHDelete(iter->second);
    }
  m_runtime_arrays.clear();
}


void
WRATHStateStream::
set_state(const WRATHStateStream &obj, bool copy_stacks)
{
  if(&obj==this)
    {
      return;
    }
  std::map<key_type, array_holder_base*>::iterator this_iter, this_end;
  std::map<key_type, array_holder_base*>::const_iterator obj_iter, obj_end;

  this_iter=m_runtime_arrays.begin();
  this_end=m_runtime_arrays.end();

  obj_iter=obj.m_runtime_arrays.begin();
  obj_end=obj.m_runtime_arrays.end();

  while(this_iter!=this_end and obj_iter!=obj_end)
    {
      if(this_iter->first < obj_iter->first)
        {
          ++this_iter;
        }
      else if(obj_iter->first < this_iter->first)
        {
          ++obj_iter;
          obj_iter->second->create_copy(m_time_location, 
                                        this, obj_iter->first.id(),
                                        copy_stacks);
        }
      else
        {
          this_iter->second->copy_state(m_time_location, obj_iter->second,
                                        copy_stacks);
          ++this_iter;
          ++obj_iter;
        }
    }

  for(;obj_iter!=obj_end; ++obj_iter)
    {
      obj_iter->second->create_copy(m_time_location, this, 
                                    obj_iter->first.id(),
                                    copy_stacks);
    }
 
}
