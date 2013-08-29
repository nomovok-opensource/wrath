/*! 
 * \file WRATHTime.hpp
 * \brief file WRATHTime.hpp
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



#ifndef __WRATH_TIME_HPP__
#define __WRATH_TIME_HPP__

#include "WRATHConfig.hpp"
#include "WRATHUtil.hpp"

/*! \addtogroup Utility
 * @{
 */

/*!\class WRATHTime
  A WRATHTime object is a conveniance class 
  using unix gettimeofday() to report
  how much time has elapsed.
 */
class WRATHTime
{
public:
  /*!\fn WRATHTime
    Ctor. 
   */
  WRATHTime(void):
    m_start_tick(0)
  {
    gettimeofday(&m_start_time, NULL);
  }
  
  /*!\fn int32_t elapsed
    Returns the number of milliseconds since
    the last call to restart(), if restart()
    has not yet been called on this \ref
    WRATHTime object then returns the number
    of milliseconds since its ctor has been
    called.
   */
  int32_t
  elapsed(void)
  {
    struct timeval current_time;
    
    gettimeofday(&current_time, NULL);
    return WRATHUtil::time_difference(current_time, m_start_time) - m_start_tick;
  }

  /*!\fn int32_t restart
    Restart the WRATHTime to the current time.
   */
  int32_t
  restart(void)
  {
    int32_t r;

    r=elapsed();
    m_start_tick+=r;
    return r;
  }

private:
  struct timeval m_start_time;
  int32_t m_start_tick;
};

/*! @} */

#endif
