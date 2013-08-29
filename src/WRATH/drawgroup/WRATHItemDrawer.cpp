/*! 
 * \file WRATHItemDrawer.cpp
 * \brief file WRATHItemDrawer.cpp
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
#include "WRATHItemDrawer.hpp"

namespace
{
  void
  delete_item_drawer(WRATHItemDrawer *ptr)
  {
    WRATHDelete(ptr);
  }
}


//////////////////////////////////
//WRATHItemDrawer methods
WRATH_RESOURCE_MANAGER_IMPLEMENT(WRATHItemDrawer, std::string)

WRATHItemDrawer::
WRATHItemDrawer(WRATHMultiGLProgram *pr):
  m_program(pr)
{
  WRATHassert(m_program!=NULL);
  m_resource_name=m_program->resource_name();
  resource_manager().add_resource(m_resource_name, this);
  m_on_program_dtor=m_program->connect_dtor(boost::bind(delete_item_drawer, this));
}

WRATHItemDrawer::
WRATHItemDrawer(WRATHMultiGLProgram *pr, 
                const std::string &presource_name):
  m_program(pr),
  m_resource_name(presource_name)
{
  WRATHassert(m_program!=NULL);
  resource_manager().add_resource(m_resource_name, this);
  m_on_program_dtor=m_program->connect_dtor(boost::bind(delete_item_drawer, this));
}


WRATHItemDrawer::
~WRATHItemDrawer()
{
  m_dtor_signal();
  m_on_program_dtor.disconnect();
  resource_manager().remove_resource(this);
}

