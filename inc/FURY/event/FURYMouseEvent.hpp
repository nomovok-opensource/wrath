/*! 
 * \file FURYMouseEvent.hpp
 * \brief file FURYMouseEvent.hpp
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


#ifndef __FURY_MOUSE_EVENT_HPP__
#define __FURY_MOUSE_EVENT_HPP__

#include "WRATHConfig.hpp"
#include "FURYEvent.hpp"

class FURYMouse
{
public:
  uint32_t m_mouse_index;

  explicit
  FURYMouse(uint32_t v):
    m_mouse_index(v)
  {}
};

//!\class FURYMouseMotionEvent
/*!
 */
class FURYMouseMotionEvent:
  public FURYEventT<FURYMouseMotionEvent>
{
public:
  FURYMouseMotionEvent(const ivec2 &ppt,
                       const ivec2 &pdelta,
                       FURYMouse pmouse=FURYMouse(0)):
    FURYEventT<FURYMouseMotionEvent>(MouseMotion),
    m_pt(ppt),
    m_delta(pdelta),
    m_mouse(pmouse)
  {}

  const ivec2&
  pt(void) const
  {
    return m_pt;
  }

  const ivec2&
  delta(void) const
  {
    return m_delta;
  }
  
  FURYMouse
  mouse(void) const
  {
    return m_mouse;
  }

  virtual
  void
  log_event(std::ostream &ostr) const
  {
    ostr << "MouseMotion[pt=" << m_pt << ", delta="
         << m_delta << ", mouse=" << m_mouse.m_mouse_index << "]";
  }

private:
  ivec2 m_pt, m_delta;
  FURYMouse m_mouse;
};

class FURYMouseWheelEvent:
  public FURYEventT<FURYMouseWheelEvent>
{
public:

  FURYMouseWheelEvent(const ivec2 &ppt,
                      const ivec2 &pscroll,
                      FURYMouse pmouse=FURYMouse(0)):
    FURYEventT<FURYMouseWheelEvent>(MouseWheel),
    m_pt(ppt),
    m_scroll(pscroll),
    m_mouse(pmouse)
  {}

  const ivec2&
  pt(void) const
  {
    return m_pt;
  }

  const ivec2&
  scroll(void) const
  {
    return m_scroll;
  }
  
  FURYMouse
  mouse(void) const
  {
    return m_mouse;
  }

  virtual
  void
  log_event(std::ostream &ostr) const
  {
    ostr << "MouseWheel[pt=" << m_pt << ", scroll="
         << m_scroll << ", mouse=" << m_mouse.m_mouse_index << "]";
  }

private:
  ivec2 m_pt;
  ivec2 m_scroll;
  FURYMouse m_mouse;  
};

class FURYMouseButtonEvent:
  public FURYEventT<FURYMouseButtonEvent>
{
public:
  FURYMouseButtonEvent(int pbutton, 
                       const ivec2 &ppt, 
                       bool ppressed,
                       FURYMouse pmouse=FURYMouse(0)):
    FURYEventT<FURYMouseButtonEvent>(ppressed?MouseButtonDown:MouseButtonUp),
    m_button(pbutton),
    m_mouse(pmouse),
    m_pt(ppt)
  {}

  bool
  pressed(void) const
  {
    return type()==MouseButtonDown;
  }

  int
  button(void) const
  {
    return m_button;
  }

  const ivec2&
  pt(void) const
  {
    return m_pt;
  }
  
  FURYMouse
  mouse(void) const
  {
    return m_mouse;
  }

  virtual
  void
  log_event(std::ostream &ostr) const
  {
    ostr << "MouseButton[pt=" << m_pt << ", button="
         << m_button << ", mouse=" << m_mouse.m_mouse_index << "]";
  }

private:
  int m_button;
  FURYMouse m_mouse;
  ivec2 m_pt;
};


#endif
