/*! 
 * \file FURYQtEvent.cpp
 * \brief file FURYQtEvent.cpp
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
#include "FURYQtEvent.hpp"
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTouchEvent>

namespace
{
  class FilterState
  {
  public:
    explicit
    FilterState(void):
      m_last_mouse_position(0,0),
      m_handle_all(false),
      m_accept_auto_repeat(false),
      m_text_mode(false),
      m_delta_non_zero(false)
    {}

    void
    capture_all(bool v)
    {
      m_handle_all=v;
    }

    void
    enable_key_repeat(bool v)
    {
      m_accept_auto_repeat=v;
    }

    void
    enable_text_mode(bool v)
    {
      m_text_mode=v;
    }

    void
    process_event(QEvent *ev);

    FURYQT::EventProducer::connect_t
    connect(const FURYQT::EventProducer::slot_type &subscriber)
    {
      return m_sig.connect(subscriber);
    }


  private:

    ivec2
    compute_delta(int x, int y)
    {
      if(m_delta_non_zero)
        {
          ivec2 p(x,y);
          return p - m_last_mouse_position;
        }
      else
        {
          return ivec2(0,0);
        }
    }

    FURYEvent::handle
    make_touch_pt(const QTouchEvent::TouchPoint &in_pt);

    void
    handle_touch_event(QTouchEvent *ev);
    
    ivec2 m_last_mouse_position;
    FURYQT::EventProducer::signal_t m_sig;
    bool m_handle_all;
    bool m_accept_auto_repeat;
    bool m_text_mode;
    bool m_delta_non_zero;
  };

  vec2
  make_vec2(const QPointF &pt)
  {
    return vec2(pt.x(), pt.y());
  }

  FURYKeyModifier
  generate_modifier(Qt::KeyboardModifiers v)
  {
    uint32_t a(0);

    if(v.testFlag(Qt::ShiftModifier))
      {
        a|=FURYKeyModifier::shift_down;
      }

    if(v.testFlag(Qt::ControlModifier))
      {
        a|=FURYKeyModifier::control_down;
      }

    if(v.testFlag(Qt::AltModifier))
      {
        a|=FURYKeyModifier::alt_down;
      }

    if(v.testFlag(Qt::MetaModifier))
      {
        a|=FURYKeyModifier::meta_down;
      }

    if(v.testFlag(Qt::KeypadModifier))
      {
        a|=FURYKeyModifier::keypad;
      }
    return FURYKeyModifier(a);
  }
}

//////////////////////////////////////////////
// FilterState methods
void
FilterState::
handle_touch_event(QTouchEvent *ev)
{
  const QList<QTouchEvent::TouchPoint> &in_pts(ev->touchPoints());

  for(QList<QTouchEvent::TouchPoint>::const_iterator 
        iter=in_pts.begin(), end=in_pts.end(); iter!=end; ++iter)
    {
      FURYEvent::handle h;

      h=make_touch_pt(*iter);
      if(h.valid())
        {
          m_sig(h);
        }
    }
}


FURYEvent::handle
FilterState::
make_touch_pt(const QTouchEvent::TouchPoint &in_pt)
{
  FURYEvent::event_type tp;
  switch(in_pt.state())
    {
    default:
      return FURYEvent::handle();

    case Qt::TouchPointPressed:
      tp=FURYEvent::TouchDown;
      break;

    case Qt::TouchPointReleased:
      tp=FURYEvent::TouchUp;
      break;

    case Qt::TouchPointMoved:
      tp=FURYEvent::TouchMotion;
      break;
      
    }
  return WRATHNew FURYTouchEvent(tp,
                                 FURYTouchID(in_pt.id()),
                                 make_vec2(in_pt.pos()),
                                 make_vec2(in_pt.pos()) - make_vec2(in_pt.lastPos()),
                                 in_pt.pressure());
}

void
FilterState::
process_event(QEvent *ev)
{
 

  FURYEvent::handle h;

  switch(ev->type())
    {
      default:
        if(m_handle_all)
          {
            h=WRATHNew FURYQT::UnknownEvent(ev);
          }
      break;

    case QEvent::Close:
      h=WRATHNew FURYEvent(FURYEvent::Close);
      break;

    case QEvent::Quit:
      h=WRATHNew FURYEvent(FURYEvent::Quit);
      break;
      
    case QEvent::MouseMove:
      {
        QMouseEvent *qev(static_cast<QMouseEvent*>(ev));

        h=WRATHNew FURYMouseMotionEvent( ivec2(qev->x(), qev->y()),
                                         compute_delta(qev->x(), qev->y()) );
        m_delta_non_zero=true;
        m_last_mouse_position=ivec2(qev->x(), qev->y());
      }
      break;

    case QEvent::MouseButtonDblClick:
      {
        /*
          Qt... is a PITA. There is not a documented way to
          get the "raw" mouse button up and down event on
          a double click.. so we emulate it by sending
          two events:
         */
        QMouseEvent *qev(static_cast<QMouseEvent*>(ev));

        h=WRATHNew FURYMouseButtonEvent(qev->button(),
                                        ivec2(qev->x(), qev->y()),
                                        true);
        m_sig(h);

        h=WRATHNew FURYMouseButtonEvent(qev->button(),
                                        ivec2(qev->x(), qev->y()),
                                        false);
      }


    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
      {
        QMouseEvent *qev(static_cast<QMouseEvent*>(ev));
        h=WRATHNew FURYMouseButtonEvent(qev->button(),
                                        ivec2(qev->x(), qev->y()),
                                        ev->type()==QEvent::MouseButtonPress);
        m_delta_non_zero=true;
        m_last_mouse_position=ivec2(qev->x(), qev->y());
      }
      break;

    case QEvent::Wheel:
      {
        QWheelEvent *qev(static_cast<QWheelEvent*>(ev));
        ivec2 pscroll(0,0);
        int indx(qev->orientation()==Qt::Horizontal?0:1);
        
        pscroll[indx]=qev->delta();
        h=WRATHNew FURYMouseWheelEvent(ivec2(qev->x(), qev->y()),
                                       pscroll);
      }
      break;

    case QEvent::Resize:
      {
        QResizeEvent *qev(static_cast<QResizeEvent*>(ev));
        h=WRATHNew FURYResizeEvent(ivec2(qev->oldSize().width(),
                                         qev->oldSize().height()), 
                                   ivec2(qev->size().width(),
                                         qev->size().height()) );
      }
      break;

    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:
      {
        handle_touch_event(static_cast<QTouchEvent*>(ev));
      }
      break;

    case QEvent::KeyPress:
      if(m_text_mode)
        {
          break;
        }
      //fall through
    case QEvent::KeyRelease:
      {
        QKeyEvent *qev(static_cast<QKeyEvent*>(ev));
        if(m_text_mode)
          {
            QVector<uint> str(qev->text().toUcs4());
            if(str.count()>0)
              {
                std::vector<uint32_t> ptext(str.count());
                std::copy(str.begin(), str.end(), ptext.begin());

                h=WRATHNew FURYTextEvent(ptext);
              }
          }
        else if(!qev->isAutoRepeat() or m_accept_auto_repeat)
          {
            h=WRATHNew FURYKeyEvent(FURYKey(qev->key()), 
                                    qev->type()==QEvent::KeyPress,
                                    qev->nativeVirtualKey(),
                                    qev->nativeScanCode(),
                                    generate_modifier(qev->modifiers()) );
          }
      }
      break;

    
    }

  if(h.valid())
    {
      m_sig(h);
    }

}



#define FILTER(o) reinterpret_cast<FilterState*>(o)

/////////////////////////////////////
// FURYQT::UnknownEvent methods
enum FURYEvent::event_type
FURYQT::UnknownEvent::
enumeration_value(void)
{
  static FURYEvent::event_type R(FURYEvent::register_event());
  return R;
}

//////////////////////////////////////
// FURYQT::EventProducer methods
FURYQT::EventProducer::
EventProducer(QWidget *p)
{
  //note that making the owner of this p, one does
  //not delete this.
  m_state=WRATHNew FilterState();
  p->setMouseTracking(true);
}

FURYQT::EventProducer::
~EventProducer(void)
{
  FilterState *d(FILTER(m_state));
  WRATHDelete(d);
}

void
FURYQT::EventProducer::
feed_event(QEvent *event)
{
  FilterState *d(FILTER(m_state));
  
  d->process_event(event);
}


FURYQT::EventProducer::connect_t
FURYQT::EventProducer::
connect(const signal_t::slot_type &subscriber)
{
  FilterState *d(FILTER(m_state));
  
  return d->connect(subscriber);
}

void
FURYQT::EventProducer::
enable_key_repeat(bool v)
{
  FilterState *d(FILTER(m_state));
  
  return d->enable_key_repeat(v);
}

void
FURYQT::EventProducer::
enable_text_mode(bool v)
{
  FilterState *d(FILTER(m_state));
  
  return d->enable_text_mode(v);
}


void
FURYQT::EventProducer::
capture_all(bool v)
{
  FilterState *d(FILTER(m_state));
  
  return d->capture_all(v);
}

