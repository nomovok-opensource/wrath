/*! 
 * \file FURYSDLEvent.hpp
 * \brief file FURYSDLEvent.hpp
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


#ifndef FURY_SDL_EVENT_HPP_
#define FURY_SDL_EVENT_HPP_

#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include "FURYEvent.hpp"
#include "FURYKeyEvent.hpp"
#include "FURYMouseEvent.hpp"
#include "FURYTouchEvent.hpp"
#include "FURYResizeEvent.hpp"
#include <boost/signals2.hpp>

#include <SDL_events.h>

namespace FURYSDL
{
  /*!\class EventProducer
    An EventProducer consumes SDL events,
    and if an event corresponds to a
    FURYEvent, then it signal the FURYEvent(s)
    made from the SDL event(s).
   */
  class EventProducer:boost::noncopyable
  {
  public:
    /*!\typedef signal_t
      Signal type.
     */
    typedef boost::signals2::signal<void (FURYEvent::handle)> signal_t;

    /*!\typedef slot_type
      Slot type.
     */
    typedef signal_t::slot_type slot_type;

    /*!\typedef connect_t
      Convenience typedef for the connection type.
    */
    typedef boost::signals2::connection connect_t;

    /*!\fn EventProducer
      Ctor
      \param w initial SDL window width 
      \param h initial SDL window height
     */
    EventProducer(int w, int h);

    ~EventProducer();

    /*!\fn capture_all
      if true, then _ALL_ SDL events
      fire the signal including
      those events that do not
      have a FURY analogue. Those
      events that do not have a
      FURY analogue are sent as
      UnknownEvent objects. If false
      those SDL events that do not
      correspond to a FURYEvent are
      ignored. Default value is false.
     */
    void
    capture_all(bool v);

    /*!\fn connect
      Connects a subscribing slot to a signal.
      \param subscriber Subscriber slot
     */
    connect_t
    connect(const slot_type &subscriber);

    /*!\fn enable_key_repeat
      Enable that holding a key triggers
      muliple key events.
     */
    void
    enable_key_repeat(int delay, int interval);

    /*!\fn enable_key_repeat
      Enable/disable that holding a key triggers
      muliple key events.
     */
    void
    enable_key_repeat(bool v);

    /*!\fn enable_text_mode
      Enable or disable text mode,
      in text mode, key events are 
      interpreted as events for 
      inputing text. Default value
      is false.
     */
    void
    enable_text_mode(bool);

    /*!\fn feed_event
      Feed an SDL Event to the EventProducer
     */
    void
    feed_event(const SDL_Event *ev);

  private:
    void *m_state;    
  };

  //!\class UnknownEvent
  /*!
    An event from SDL that does not have a
    FURY analogue. 
   */
  class UnknownEvent:
    public FURYEventT<UnknownEvent>
  {
  public:
    explicit
    UnknownEvent(const SDL_Event &ev):
      FURYEventT<UnknownEvent>(enumeration_value()),
      m_event(ev)
    {}

    const SDL_Event&
    event(void) const
    {
      return m_event;
    }

    static
    enum event_type
    enumeration_value(void);

  private:
    SDL_Event m_event;
  };

};

enum
  {
    /*
      This is not a nice thing to do,
      but oh well. The basic idea 
      is that we will use the SDL's
      enumeration values for it's Keys
      directly.
    */
#include "FURYSDLKeyCode.values.tcc"
  };

#endif
