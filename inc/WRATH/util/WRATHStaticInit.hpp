/*! 
 * \file WRATHStaticInit.hpp
 * \brief file WRATHStaticInit.hpp
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




#ifndef __WRATH_STATIC_INIT_HPP__
#define __WRATH_STATIC_INIT_HPP__

#include "WRATHConfig.hpp"


/*! \addtogroup Utility
 * @{
 */

/*!\fn void WRATHStaticInit
  If a function has static local variables, call WRATHStaticInit()
  _before_ the declaration of those static local variables.
  This is needed so that various book keeping data structures
  will go out of scope _AFTER_ those static local variables.
 */
void
WRATHStaticInit(void);


/*! @} */

#endif
