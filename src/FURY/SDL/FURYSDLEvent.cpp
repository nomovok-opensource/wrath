/*! 
 * \file FURYSDLEvent.cpp
 * \brief file FURYSDLEvent.cpp
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



#include "WRATHConfig.hpp"
#include "FURYSDLEvent.hpp"
#include "FURYJoystickEvent.hpp"
#include "FURYTextEvent.hpp"

namespace
{
  class sdl_fury_event_state:boost::noncopyable
  {
  public:
    sdl_fury_event_state(void):
      m_capture_all(false),
      m_last_size(0, 0),
      m_text_mode(false)
    {
      SDL_EnableUNICODE(0);
    }

    FURYSDL::EventProducer::signal_t m_sig;
    bool m_capture_all;
    ivec2 m_last_size;
    bool m_text_mode;
  };

  sdl_fury_event_state&
  st(void *q)
  {
    sdl_fury_event_state *r;

    r=static_cast<sdl_fury_event_state*>(q);
    return *r;
  }

  enum FURYJoystickHatMotion::hat_position_t
  create_hat_code(Uint8 v)
  {
    uint32_t r(0);

    if(v&SDL_HAT_UP)
      {
        r|=FURYJoystickHatMotion::hat_up;
      }

    if(v&SDL_HAT_RIGHT)
      {
        r|=FURYJoystickHatMotion::hat_right;
      }

    if(v&SDL_HAT_DOWN)
      {
        r|=FURYJoystickHatMotion::hat_down;
      }

    if(v&SDL_HAT_LEFT)
      {
        r|=FURYJoystickHatMotion::hat_left;
      }

    return static_cast<enum FURYJoystickHatMotion::hat_position_t>(r);
  }

  FURYKeyModifier
  compute_modifier(SDLMod flags)
  {
    uint32_t a(0);

    if(flags&KMOD_LSHIFT)
      {
        a|=FURYKeyModifier::left_shift_down;
      }

    if(flags&KMOD_RSHIFT)
      {
        a|=FURYKeyModifier::right_shift_down;
      }

    if(flags&KMOD_LCTRL)
      {
        a|=FURYKeyModifier::left_control_down;
      }

    if(flags&KMOD_RCTRL)
      {
        a|=FURYKeyModifier::right_control_down;
      }

    if(flags&KMOD_LALT)
      {
        a|=FURYKeyModifier::left_alt_down;
      }

    if(flags&KMOD_RALT)
      {
        a|=FURYKeyModifier::right_alt_down;
      }

    /*if(flags&KMOD_MODE)
      {
        // AltGr
      }*/

    if(flags&KMOD_LMETA)
      {
        a|=FURYKeyModifier::left_meta_down;
      }

    if(flags&KMOD_RMETA)
      {
        a|=FURYKeyModifier::right_meta_down;
      }

    if(flags&KMOD_NUM)
      {
        a|=FURYKeyModifier::keypad;
      }

    if(flags&KMOD_CAPS)
      {
        a|=FURYKeyModifier::caps;
      }

    return FURYKeyModifier(a);
  }
}

///////////////////////////////////
// FURYSDL::UnknownEvent methods
enum FURYEvent::event_type
FURYSDL::UnknownEvent::
enumeration_value(void)
{
  static FURYEvent::event_type R(FURYEvent::register_event());
  return R;
}


//////////////////////////////////
// FURYSDL::EventProducer methods
FURYSDL::EventProducer::
EventProducer(int w, int h):
  m_state(NULL)
{
  sdl_fury_event_state *q;

  q=WRATHNew sdl_fury_event_state();
  q->m_last_size=ivec2(w,h);
  m_state=q;
}

FURYSDL::EventProducer::
~EventProducer()
{
  sdl_fury_event_state *q;

  q=&st(m_state);

  WRATHDelete(q);
  m_state=NULL;
}


void
FURYSDL::EventProducer::
feed_event(const SDL_Event *ev)
{
  if(ev==NULL)
    {
      return;
    }

  FURYEvent::handle h;
  switch(ev->type)
    {
    default:
      if(st(m_state).m_capture_all)
        {
          h=WRATHNew UnknownEvent(*ev);
        }
      break;

    case SDL_KEYUP:
      if(st(m_state).m_text_mode)
        {
          break;
        }
      //fall through
    case SDL_KEYDOWN:
      {
        if(st(m_state).m_text_mode)
          {
            std::vector<uint32_t> pvalues(1, ev->key.keysym.unicode);
            h=WRATHNew FURYTextEvent(pvalues);
          }
        else
          {
            h=WRATHNew FURYKeyEvent(FURYKey(ev->key.keysym.sym), 
                                    ev->key.type==SDL_KEYDOWN,
                                    ev->key.keysym.scancode,
                                    ev->key.keysym.scancode,
                                    compute_modifier(ev->key.keysym.mod));
          }
      }
      break;

    case SDL_MOUSEMOTION:
      {
        h=WRATHNew FURYMouseMotionEvent(ivec2(ev->motion.x, ev->motion.y),
                                        ivec2(ev->motion.xrel, ev->motion.yrel),
                                        FURYMouse(ev->motion.which));
      }
      break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      {
        h=WRATHNew FURYMouseButtonEvent(ev->button.button,
                                        ivec2(ev->button.x, ev->button.y),
                                        ev->button.state==SDL_PRESSED,
                                        FURYMouse(ev->button.which));
      }
      break;

    case SDL_JOYAXISMOTION:
      {
        h=WRATHNew FURYJoystickAxisEvent(ev->jaxis.value,
                                         FURYJoystickAxis(ev->jaxis.axis),
                                         FURYJoystick(ev->jaxis.which));
      }
      break;

    case SDL_JOYHATMOTION:
      {
        h=WRATHNew FURYJoystickHatMotion(create_hat_code(ev->jhat.value),
                                         FURYJoyHat(ev->jhat.hat),
                                         FURYJoystick(ev->jhat.which));
      } 
      break;

    case SDL_JOYBALLMOTION:
      {
        h=WRATHNew FURYJoystickBallMotionEvent(ivec2(ev->jball.xrel, ev->jball.yrel),
                                               FURYJoyBall(ev->jball.ball),
                                               FURYJoystick(ev->jball.which));
      } 
      break; 
      

    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
      {
        h=WRATHNew FURYJoystickButtonEvent(ev->jbutton.type==SDL_JOYBUTTONDOWN,
                                           FURYJoystickButton(ev->jbutton.button),
                                           FURYJoystick(ev->jbutton.which));
      }
      break;

    case SDL_VIDEORESIZE:
      {
        ivec2 new_size(ev->resize.w, ev->resize.h);
        h=WRATHNew FURYResizeEvent(st(m_state).m_last_size, 
                                   new_size);
        st(m_state).m_last_size=new_size;
      }
      break;

    case SDL_QUIT:
      {
        h=WRATHNew FURYEvent(FURYEvent::Quit);
      }
      break;
    }

  if(h.valid())
    {
      st(m_state).m_sig(h);
    }
}


void
FURYSDL::EventProducer::
enable_key_repeat(bool v)
{
  if(v)
    {
      SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,
                          SDL_DEFAULT_REPEAT_INTERVAL);
    }
  else
    {
      SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
    }
}

void
FURYSDL::EventProducer::
enable_key_repeat(int delay, int interval)
{
  SDL_EnableKeyRepeat( std::max(delay, 0),
                       std::max(interval, 0) );
}


void
FURYSDL::EventProducer::
enable_text_mode(bool v)
{
  st(m_state).m_text_mode=v;
  SDL_EnableUNICODE(v?1:0);
}


FURYSDL::EventProducer::connect_t
FURYSDL::EventProducer::
connect(const slot_type &subscriber)
{
  return st(m_state).m_sig.connect(subscriber);
}
