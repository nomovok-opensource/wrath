/*! 
 * \file FURYJoystickEvent.hpp
 * \brief file FURYJoystickEvent.hpp
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


#ifndef __FURY_JOYSTICK_EVENT_HPP__
#define __FURY_JOYSTICK_EVENT_HPP__

#include "WRATHConfig.hpp"
#include "FURYEvent.hpp"

class FURYJoystick
{
public:
  uint8_t m_joystick_index;

  explicit
  FURYJoystick(uint8_t v):
    m_joystick_index(v)
  {}

  friend
  std::ostream&
  operator<<(std::ostream &ostr, FURYJoystick v)
  {
    ostr << "Joystick#" << static_cast<int>(v.m_joystick_index);
    return ostr;
  }
};

class FURYJoystickAxis
{
public:
  uint8_t m_axis_index;

  explicit
  FURYJoystickAxis(uint8_t v):
    m_axis_index(v)
  {}

  friend
  std::ostream&
  operator<<(std::ostream &ostr, FURYJoystickAxis v)
  {
    ostr << "Axis#" << static_cast<int>(v.m_axis_index);
    return ostr;
  }
};

class FURYJoystickButton
{
public:
  uint8_t m_button_index;
  
  explicit
  FURYJoystickButton(uint8_t v):
    m_button_index(v)
  {}

  friend
  std::ostream&
  operator<<(std::ostream &ostr, FURYJoystickButton v)
  {
    ostr << "Button#" << static_cast<int>(v.m_button_index);
    return ostr;
  }
};

class FURYJoyHat
{
public:
  uint8_t m_hat_index;
  
  explicit
  FURYJoyHat(uint8_t v):
    m_hat_index(v)
  {}

  friend
  std::ostream&
  operator<<(std::ostream &ostr, FURYJoyHat v)
  {
    ostr << "Hat#" << static_cast<int>(v.m_hat_index);
    return ostr;
  }
};

class FURYJoyBall
{
public:
  uint8_t m_ball_index;
  
  explicit
  FURYJoyBall(uint8_t v):
    m_ball_index(v)
  {}

  friend
  std::ostream&
  operator<<(std::ostream &ostr, FURYJoyBall v)
  {
    ostr << "Ball#" << static_cast<int>(v.m_ball_index);
    return ostr;
  }
};

class FURYJoystickAxisEvent:
  public FURYEventT<FURYJoystickAxisEvent>
{
public:
  FURYJoystickAxisEvent(int32_t paxis_position,
                        FURYJoystickAxis paxis,
                        FURYJoystick pjoystick):
    FURYEventT<FURYJoystickAxisEvent>(JoystickAxisMotion),
    m_axis_position(paxis_position),
    m_axis(paxis),
    m_joystick(pjoystick)
  {}

  int32_t
  axis_position(void) const
  {
    return m_axis_position;
  }
 
  FURYJoystickAxis
  axis(void) const
  {
    return m_axis;
  }
  
  FURYJoystick
  joystick(void) const
  {
    return m_joystick;
  }

  virtual
  void
  log_event(std::ostream &ostr) const
  {
    ostr << "JoystickAxisEvent[axis_position=" 
         << m_axis_position << ", " 
         << m_axis << ", " 
         << m_joystick << "]";
  }

private:
  int32_t m_axis_position;       
  FURYJoystickAxis m_axis;  
  FURYJoystick m_joystick; 
};

class FURYJoystickButtonEvent:
  public FURYEventT<FURYJoystickButtonEvent>
{
public:
  FURYJoystickButtonEvent(bool ppressed,
                          FURYJoystickButton pbutton,
                          FURYJoystick pjoystick):
    FURYEventT<FURYJoystickButtonEvent>(ppressed?JoystickButtonDown:JoystickButtonUp),
    m_joystick(pjoystick),
    m_button(pbutton)
  {}

  bool
  pressed(void) const
  {
    return type()==JoystickButtonDown;
  }

  FURYJoystickButton
  button(void) const
  {
    return m_button;
  }
  
  FURYJoystick
  joystick(void) const
  {
    return m_joystick;
  }

  virtual
  void
  log_event(std::ostream &ostr) const
  {
    ostr << "JoystickButtonEvent[" 
         << m_button << ", pressed="
         << pressed() << ", "
         << m_joystick << "]";
  }
    

private:
  FURYJoystick m_joystick;
  FURYJoystickButton m_button;
};

class FURYJoystickHatMotion:
  public FURYEventT<FURYJoystickHatMotion>
{
public:

  enum hat_position_t
    {
      hat_centered=0x0,
      hat_left=0x1,
      hat_right=0x2,
      hat_up=0x4,
      hat_down=0x8,

      hat_left_up=hat_left|hat_up,
      hat_left_down=hat_left|hat_down,
      hat_right_up=hat_right|hat_up,
      hat_right_down=hat_right|hat_down,

    };

  FURYJoystickHatMotion(enum hat_position_t h,
                        FURYJoyHat phat,
                        FURYJoystick pjoystick):
    FURYEventT<FURYJoystickHatMotion>(JoystickHatMotion),
    m_value(h),
    m_hat(phat),
    m_joystick(pjoystick)
  {}

  
  enum hat_position_t
  hat_position(void) const
  {
    return m_value;
  }
  
  FURYJoyHat
  hat(void) const
  {
    return m_hat;
  }

  FURYJoystick
  joystick(void) const
  {
    return m_joystick;
  }

  virtual
  void
  log_event(std::ostream &ostr) const
  {
    ostr << "JoystickHatEvent[value=" 
         << m_value << ", "
         << m_hat << ", "
         << m_joystick << "]";
  }

private:
  enum hat_position_t m_value;
  FURYJoyHat m_hat;
  FURYJoystick m_joystick;  
};

class FURYJoystickBallMotionEvent:
  public FURYEventT<FURYJoystickBallMotionEvent>
{
public:
  FURYJoystickBallMotionEvent(const ivec2 &pdelta,
                              FURYJoyBall pball,
                              FURYJoystick pjoy):
    FURYEventT<FURYJoystickBallMotionEvent>(FURYEvent::JoystickBallMotion),
    m_delta(pdelta),
    m_ball(pball),
    m_joystick(pjoy)
  {}

  const ivec2&
  delta(void) const
  {
    return m_delta;
  }

  FURYJoyBall
  ball(void) const
  {
    return m_ball;
  }

  FURYJoystick
  joystick(void) const
  {
    return m_joystick;
  }

  virtual
  void
  log_event(std::ostream &ostr) const
  {
    ostr << "JoystickBallEvent[delta=" 
         << m_delta << ", "
         << m_ball << ", " 
         << m_joystick << "]";
  }

private:
  ivec2 m_delta;
  FURYJoyBall m_ball;
  FURYJoystick m_joystick;
};


#endif
