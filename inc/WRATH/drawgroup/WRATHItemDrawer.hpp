/*! 
 * \file WRATHItemDrawer.hpp
 * \brief file WRATHItemDrawer.hpp
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




#ifndef __WRATH_ITEM_DRAWER_HPP__
#define __WRATH_ITEM_DRAWER_HPP__

#include "WRATHConfig.hpp"
#include <map>
#include <boost/utility.hpp>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include "WRATHNew.hpp"
#include "WRATHMultiGLProgram.hpp"
#include "WRATHResourceManager.hpp"



/*! \addtogroup Group
 * @{
 */

/*!\class WRATHItemDrawer
  A WRATHItemDrawer draws the contents
  of a WRATHItemGroup, it is a conveniant base
  class for different WRATH drawing systems,
  internally it has only one data item:
  a pointer to a WRATHMultiGLProgram.
 */
class WRATHItemDrawer:boost::noncopyable
{
public:
  /// @cond
  WRATH_RESOURCE_MANAGER_DECLARE(WRATHItemDrawer, std::string);         
  /// @endcond

  /*!\fn WRATHItemDrawer(WRATHMultiGLProgram *pr)
    Ctor, passing the WRATHMultiGLProgram the WRATHItemDrawer is to use 
    for drawing. The resource name of the created WRATHItemDrawer
    will be the same as the resource name of the passed WRATHMultiGLProgram.
    \param pr WRATHMultiGLProgram to use for drawing.
   */
  explicit
  WRATHItemDrawer(WRATHMultiGLProgram *pr);       

  /*!\fn WRATHItemDrawer(WRATHMultiGLProgram *pr,
                         const std::string &presource_name)
    Ctor, passing the WRATHMultiGLProgram the WRATHItemDrawer is to use 
    for drawing.
    \param pr WRATHMultiGLProgram to use for drawing.
    \param presource_name resource name to give to created object  
   */
  WRATHItemDrawer(WRATHMultiGLProgram *pr, 
                  const std::string &presource_name);

  virtual
  ~WRATHItemDrawer();

  /*!\fn boost::signals2::connection connect_dtor 
    The dtor of a WRATHItemDrawer emit's a signal, use this function
    to connect to that signal. The signal is emitted just before
    the WRATHItemDrawer is removed from the resource manager.
   */
  boost::signals2::connection 
  connect_dtor(const boost::signals2::signal<void () >::slot_type &slot)
  {
    return m_dtor_signal.connect(slot);
  }

  /*!\fn WRATHMultiGLProgram* program
    Returns the WRATHMultiGLProgram used for drawing.
   */
  WRATHMultiGLProgram*
  program(void) const
  {
    return m_program;
  }

  /*!\fn const std::string& resource_name
    Returns the resource name of the WRATHItemDrawer.
   */
  const std::string&
  resource_name(void) const
  {
    return m_resource_name;
  }

private:
  WRATHMultiGLProgram *m_program;
  std::string m_resource_name;
  boost::signals2::connection m_on_program_dtor;
  boost::signals2::signal<void () > m_dtor_signal;
};

/*! @} */

#endif
