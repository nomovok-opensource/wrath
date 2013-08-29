/*! 
 * \file WRATHStaticInit.cpp
 * \brief file WRATHStaticInit.cpp
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

void
WRATHInternalNewInit(void);

void
WRATHInternalMallocInit(void);

void
WRATHStaticInit(void)
{
  WRATHInternalNewInit();
  WRATHInternalMallocInit();
}
