/*! 
 * \file FURYTouchEvent.hpp
 * \brief file FURYTouchEvent.hpp
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


#ifndef __FURY_TOUCH_EVENT_HPP__
#define __FURY_TOUCH_EVENT_HPP__


#include "WRATHConfig.hpp"
#include "FURYEvent.hpp"
#include "WRATHBBox.hpp"

//!\class FURYTouchID
/*!
  Denotes the touch Id of a 
  touch event (for handling multi-
  touch devices)
 */
class FURYTouchID
{
public:
  int m_value;

  explicit
  FURYTouchID(int v):
    m_value(v)
  {}
};

class FURYTouchEvent:
  public FURYEventT<FURYTouchEvent>
{
public:

  /*
    Follow SDL1.3 conventions, one touch
    device event spawns one FURYTouchEvent
   */    
  explicit
  FURYTouchEvent(enum event_type tp,
                 FURYTouchID pid,
                 const vec2 &pposition,
                 const vec2 &pdelta,
                 float ppressure):
    FURYEventT<FURYTouchEvent>(tp),
    m_position(pposition),
    m_delta(pdelta),
    m_pressure(ppressure),
    m_id(pid)
  {}

  const vec2&
  position(void) const
  {
    return m_position;
  }

  const vec2&
  delta(void) const
  {
    return m_delta;
  }

  float
  pressure(void) const
  {
    return m_pressure;
  }

  FURYTouchID
  id(void) const
  {
    return m_id;
  }

  virtual
  void
  log_event(std::ostream &ostr) const
  {
    ostr << "TouchEvent[pt=" << m_position << ", delta="
         << m_delta << ", pressure=" << m_pressure
         << ", ID=" << m_id.m_value << "]";
  }

private:
  vec2 m_position;
  vec2 m_delta;
  float m_pressure;
  FURYTouchID m_id;
};


#endif
