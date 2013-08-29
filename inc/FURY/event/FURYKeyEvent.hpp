/*! 
 * \file FURYKeyEvent.hpp
 * \brief file FURYKeyEvent.hpp
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


#ifndef __FURY_KEY_EVENT_HPP__
#define __FURY_KEY_EVENT_HPP__


#include "WRATHConfig.hpp"
#include "FURYEvent.hpp"

class FURYKey
{
public:
  uint32_t m_value;

  explicit
  FURYKey(uint32_t v):
    m_value(v)
  {}

  friend
  std::ostream&
  operator<<(std::ostream &ostr, FURYKey v)
  {
    ostr << "KeyCode:" << v.m_value;
    return ostr;
  }
};

class FURYKeyModifier
{
public:
  uint32_t m_v;

  enum
    {
      
      left_shift_down= 0x0001,
      right_shift_down=0x0002,
      shift_down=left_shift_down|right_shift_down,

      left_control_down= 0x0004,
      right_control_down=0x0008,
      control_down=left_control_down|right_control_down,

      left_alt_down= 0x0010,
      right_alt_down=0x0020,
      alt_down=left_alt_down|right_alt_down,
      
      left_meta_down= 0x0040,
      right_meta_down=0x0080,
      meta_down=left_meta_down|right_meta_down,

      keypad=0x0100,
      caps  =0x0200,
     
    };

  explicit
  FURYKeyModifier(uint32_t v):
    m_v(v)
  {}


  friend
  std::ostream&
  operator<<(std::ostream &ostr, FURYKeyModifier v)
  {
    ostr << "FURYKeyModifier:" 
         << std::hex << v.m_v << std::dec;
    return ostr;
  }
};


class FURYKeyEvent:
  public FURYEventT<FURYKeyEvent>
{
public:
  /*
    TODO: 
    - add a "mod" field, i.e. a
      bitfield listing what modifier keys
      are up or down.
    - add "something" to allow for NOT
      getting raw keys, akin to SDL's
      SDL_EnableUNICODE
   */

  FURYKeyEvent(FURYKey v, bool ppressed,
               uint32_t pnative_virtual_key,
               uint32_t pnative_scancode,
               FURYKeyModifier pmodifier):
    FURYEventT<FURYKeyEvent>(ppressed?KeyDown:KeyUp),
    m_key(v),
    m_native_virtual_key(pnative_virtual_key),
    m_native_scancode(pnative_scancode),
    m_modifier(pmodifier)
  {}

  FURYKey 
  key(void) const
  {
    return m_key;
  }

  bool
  pressed(void) const
  {
    return type()==KeyDown;
  }

  uint32_t
  native_virtual_key(void) const
  {
    return m_native_virtual_key;
  }

  uint32_t
  native_scancode(void) const
  {
    return m_native_scancode;
  }

  FURYKeyModifier
  modifier(void) const
  {
    return m_modifier;
  }

  virtual
  void
  log_event(std::ostream &ostr) const
  {
    ostr << "KeyEvent[pressed="
         << pressed() << ", "
         << m_key << ", "
         << m_modifier
         << " scancode="
         << m_native_scancode
         << " native_virtual="
         << m_native_virtual_key << "]";
  }

private:
  FURYKey m_key;
  uint32_t m_native_virtual_key, m_native_scancode;
  FURYKeyModifier m_modifier;
};

#endif
