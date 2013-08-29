/*! 
 * \file FURYQtEvent.hpp
 * \brief file FURYQtEvent.hpp
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


#ifndef __FURY_QT_EVENT_HPP__
#define __FURY_QT_EVENT_HPP__

#include "WRATHConfig.hpp"
#include "FURYEvent.hpp"
#include "FURYKeyEvent.hpp"
#include "FURYMouseEvent.hpp"
#include "FURYTouchEvent.hpp"
#include "FURYResizeEvent.hpp"
#include "FURYTextEvent.hpp"
#include <boost/signals2.hpp>
#include <QEvent>
#include <QKeyEvent>
#include <QWidget>

namespace FURYQT
{
  //!\class EventProducer
  /*!
    This is.. unfortunate.
    In order to correctly get several event
    types (and values) we need to see the
    events before Qt does *something* to
    them. Additionally, some Qt events 
    correspond to multiple FURYEvents.
    An EventProducer needs to be fed 
    events. Some Qt events may generate 
    multiple FURYEvent's, and others may 
    produce none.
   */
  class EventProducer:boost::noncopyable
  {
  public:

    //!\typedef signal_t
    /*!
      Signal type.
     */
    typedef boost::signals2::signal<void (FURYEvent::handle)> signal_t;

    //!\typedef slot_type
    /*!
      Slot type.
     */
    typedef signal_t::slot_type slot_type;

    //!\typedef connect_t
    /*!
      Conveniance typedef for the connection type.
    */
    typedef boost::signals2::connection connect_t;

    //!\fn
    /*!
      \param p pointer to QWidget which will use
               the feed the EventProducer to produce
               FURY events. 
     */
    explicit
    EventProducer(QWidget *p);

    ~EventProducer(void);

    //!\fn
    /*!
      if true, then _ALL_ Qt events
      fire the signal including
      those events that do not
      have a FURY analogue. Those
      events that do not have a
      FURY analogue are sent as
      UnknownEvent objects. If false
      those Qt events that do not
      correspond to a FURYEvent are
      ignored. Default value is false.
     */
    void
    capture_all(bool v);

    //!\fn
    /*!
     */
    connect_t
    connect(const slot_type &subscriber);
    
    //!\fn
    /*
      Enable key repeat, i.e. user holding down
      a key generates many key release and
      press events.
      \param delay time in ms between trigger key event
     */
    void
    enable_key_repeat(bool);

    //!\fn
    /*!
      Enable or disable text mode,
      in text mode, key events are 
      interpreted as events for 
      inputing text. Default value
      is false.
     */
    void
    enable_text_mode(bool);

    /*
      what to call to feed events
      
     */
    void
    feed_event(QEvent *event);

  private:
    void *m_state;
  };


  //!\class UnknownEvent
  /*!
    An event from Qt that does not have a
    FURY analogue. Beware! Qt deletes
    the event so don't save the QEvent
   */
  class UnknownEvent:
    public FURYEventT<UnknownEvent>
  {
  public:
    explicit
    UnknownEvent(QEvent *ev):
      FURYEventT<UnknownEvent>(enumeration_value()),
      m_event(ev)
    {}

    QEvent*
    event(void) const
    {
      return m_event;
    }

    static
    enum event_type
    enumeration_value(void);

  private:
    QEvent *m_event;
  };

};



enum
  {
    /*
      This is not a nice thing to do,
      but oh well. The basic idea 
      is that we will use the Qt's
      enumeration values for it's Keys
      directly.
    */
#include "FURYQTKeyCode.values.tcc"
  };

#endif
