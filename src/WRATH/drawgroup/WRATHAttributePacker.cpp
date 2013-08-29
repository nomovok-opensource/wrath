/*! 
 * \file WRATHAttributePacker.cpp
 * \brief file WRATHAttributePacker.cpp
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
#include "WRATHAttributePacker.hpp"
#include "WRATHStaticInit.hpp"

WRATH_RESOURCE_MANAGER_IMPLEMENT(WRATHAttributePacker, 
                                 WRATHAttributePacker::ResourceKey);

void
WRATHAttributePacker::
register_resource(void)
{
  resource_manager().add_resource(m_resource_name, this);
}


WRATHAttributePacker::
~WRATHAttributePacker()
{
  resource_manager().remove_resource(this);
}

WRATHMutex&
WRATHAttributePacker::
fetch_make_mutex(void)
{
  WRATHStaticInit();
  static WRATHMutex R;
  return R;
}


void
WRATHAttributePacker::
bind_attributes(WRATHGLPreLinkActionArray &binder) const
{
  for(int i=0, N=number_attributes(); i<N; ++i)
    {
      binder.add_binding(attribute_name(i), i);
    }
}
