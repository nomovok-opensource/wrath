/*! 
 * \file WRATHResourceManager.cpp
 * \brief file WRATHResourceManager.cpp
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
#include "WRATHResourceManager.hpp"
#include "WRATHStaticInit.hpp"
#include "WRATHMutex.hpp"

namespace
{
  class tracker:boost::noncopyable
  {
  public:
    std::set<WRATHResourceManagerBase*> m_elements;
    WRATHMutex m_mutex;
  };

  tracker&
  all_resources(void)
  {
    WRATHStaticInit();
    static tracker R;
    return R;
  }
}


WRATHResourceManagerBase::
WRATHResourceManagerBase(void)
{
  WRATHAutoLockMutex(all_resources().m_mutex);
  all_resources().m_elements.insert(this);
}

WRATHResourceManagerBase::
~WRATHResourceManagerBase(void)
{
  WRATHAutoLockMutex(all_resources().m_mutex);
  all_resources().m_elements.erase(this);
}


void
WRATHResourceManagerBase::
clear_all_resource_managers(void)
{
  WRATHAutoLockMutex(all_resources().m_mutex);
  for(std::set<WRATHResourceManagerBase*>::iterator
        iter=all_resources().m_elements.begin(), 
        end=all_resources().m_elements.end();
      iter!=end; ++iter)
    {
      (*iter)->clear();
    }
}
