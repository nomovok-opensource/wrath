/*! 
 * \file WRATHatomic.hpp
 * \brief file WRATHatomic.hpp
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


#ifndef __WRATH_ATOMIC_HPP__
#define __WRATH_ATOMIC_HPP__

#include "WRATHConfig.hpp"


/*! \addtogroup Utility
 * @{
 */

/*!\def WRATHAtomicAddAndFetch
  Atomic add. Atmic version of
  \code
  X+=Y;
  return X;
  \endcode
  \param X value to affect
  \param Y how much to add to X
 */

/*!\def WRATHAtomicSubtractAndFetch
  Atomic subtract. Atmic version of
  \code
  X-=Y;
  return X;
  \endcode
  \param X value to affect
  \param Y how much to add to X
 */

#if __GNUC__>4 || (__GNUC__>=4 && __GNUC_MINOR__>=7)
  #define WRATHAtomicAddAndFetch(X, Y) __atomic_add_fetch((X),  (Y), __ATOMIC_SEQ_CST)
  #define WRATHAtomicSubtractAndFetch(X, Y) __atomic_sub_fetch((X),  (Y), __ATOMIC_SEQ_CST)
#else  
  #define WRATHAtomicAddAndFetch(X, Y) __sync_add_and_fetch((X),  (Y))
  #define WRATHAtomicSubtractAndFetch(X, Y) __sync_sub_and_fetch((X),  (Y))
#endif


/*! @} */

#endif
