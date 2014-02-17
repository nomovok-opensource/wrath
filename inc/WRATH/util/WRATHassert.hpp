/*! 
 * \file WRATHassert.hpp
 * \brief file WRATHassert.hpp
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


#ifndef WRATH_HEADER_ASSERT_HPP_
#define WRATH_HEADER_ASSERT_HPP_


#include "WRATHConfig.hpp"
#include <iostream>

/*! \addtogroup Utility
 * @{
 */


/*!\def WRATHassert
  If WRATH_ASSERT_ACTIVE is defined,
  WRATHassert maps to assert, otherwise
  it maps to nothing. Note that assert
  maps to nothing if NDEBUG is defined.
  \param X condition to test
 */
#include <assert.h>
#ifdef WRATH_ASSERT_ACTIVE
#define WRATHassert(X) assert(X)
#else
#define WRATHassert(X)
#endif

/*!\def WRATHwarning
  Macro to print a warning message to std::cerr, prefixed
  with Warning[file, line]. 
  \param X that which to stream to std::cerr, can be anything that is allowed
           on the right hand side of the operator << where the left hand side
           is an std::ostream&
 */
#define WRATHwarning(X) do { std::cerr << "Warning [" << __FILE__ << ", " << __LINE__ << "]:" << X << "\n"; } while(0)

/*!\def WRATHunused
  Macro to stop the compiler from reporting
  an argument as unused. Typically used on
  those arguments used in \ref WRATHassert 
  invocation but otherwise unused.
  \param expr expression of which to ignore the value
 */
#define WRATHunused(expr) do { (void)(expr); } while (0)


/*! @} */



#endif
