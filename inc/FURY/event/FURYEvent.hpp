/*! 
 * \file FURYEvent.hpp
 * \brief file FURYEvent.hpp
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


#ifndef __FURY_EVENT_HPP__
#define __FURY_EVENT_HPP__

#include "WRATHConfig.hpp"
#include <stdint.h>
#include <ostream>
#include <stdint.h>
#include "WRATHReferenceCountedObject.hpp"
#include "vectorGL.hpp"

//!\class FURYEvent
/*!
 */
class FURYEvent:public WRATHReferenceCountedObjectT<FURYEvent>
{
public:
  
  enum event_type
    {
      Quit,
      Close,
      Resize,

      /*
        TODO, add:
        - move window
        - hide/show window
        - gain/lose focus [keyboard, mouse, etc]
        - tablet events
       */
      

      KeyUp,
      KeyDown,

      Text,

      MouseMotion,
      MouseButtonUp,
      MouseButtonDown,
      MouseWheel,

      TouchDown,
      TouchUp,
      TouchMotion,

      JoystickAxisMotion,
      JoystickBallMotion,
      JoystickHatMotion,
      JoystickButtonDown,
      JoystickButtonUp,

      LastEvent=0x8000
    };

  explicit
  FURYEvent(enum event_type ptype):
    m_type(ptype),
    m_accepted(false)
  {}

  enum event_type
  type(void) const
  {
    return m_type;
  }

  void
  accept(void)
  {
    m_accepted=true;
  }
  
  void
  ignore(void)
  {
    m_accepted=false;
  }

  bool
  accepted(void)
  {
    return m_accepted;
  }

  virtual
  void
  log_event(std::ostream &ostr) const
  {
    ostr << "GenericEvent[" << m_type << "]";
  }

  static
  enum event_type
  register_event(void);

 


  
private:
  enum event_type m_type;
  bool m_accepted;

};





template<typename T>
class FURYEventT:public FURYEvent
{
public:
  typedef handle_t<T> handle;
  typedef const_handle_t<T> const_handle;

  explicit
  FURYEventT(enum event_type ptype):
    FURYEvent(ptype)
  {}

};


#endif
