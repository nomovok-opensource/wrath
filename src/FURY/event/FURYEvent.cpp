/*! 
 * \file FURYEvent.cpp
 * \brief file FURYEvent.cpp
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
#include "FURYEvent.hpp"
#include "WRATHMutex.hpp"

enum FURYEvent::event_type
FURYEvent::
register_event(void)
{
  static WRATHMutex m;
  static unsigned int current_v(LastEvent);
  
  WRATHAutoLockMutex(m);
  ++current_v;

  return static_cast<enum event_type>(current_v);
}
