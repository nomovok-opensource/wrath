/*! 
 * \file WRATHCanvasHandle.hpp
 * \brief file WRATHCanvasHandle.hpp
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




#ifndef __WRATH_CANVAS_HANDLE_HPP__
#define __WRATH_CANVAS_HANDLE_HPP__

#include "WRATHCanvas.hpp"


/*! \addtogroup Group
 * @{
 */

/*!\class WRATHCanvasHandle
  A WRATHCanvasHandle is a smart pointer to
  a WRATHCanvas. The internal pointer
  is set to NULL when the WRATHCanvas
  goes out of scope.
 */
class WRATHCanvasHandle:boost::noncopyable
{
public:
  /*!\fn WRATHCanvasHandle(void)
    Ctor. Initializes to not point
    to any WRATHCanvas
   */
  WRATHCanvasHandle(void):
    m_canvas(NULL)
  {}

  ~WRATHCanvasHandle()
  {
    m_dtor_connect.disconnect();
  }

  /*!\fn WRATHCanvas* canvas_base(void) const
    Returns the WRATHCanvas to which
    this WRATHCanvasHandle points.
   */
  WRATHCanvas*
  canvas_base(void) const
  {
    return m_canvas;
  }

  /*!\fn void canvas_base(WRATHCanvas*) 
    Sets the WRATHCanvas to which
    this WRATHCanvasHandle points.
    The value is auto-magically set
    as NULL when the WRATHCanvas goes
    out of scope
    \param p value which to use
   */
  void
  canvas_base(WRATHCanvas *p);

private:
  WRATHCanvas *m_canvas;
  WRATHCanvas::connect_t m_dtor_connect;
};

/*!\class WRATHCanvasHandleT
  A WRATHCanvasHandleT provides a type safe-way
  to guarantee that the object pointed is alteast
  a certain WRATHCanvas dervied type.
  \tparam C WRATHCanvas derived type
 */
template<typename C>
class WRATHCanvasHandleT:private WRATHCanvasHandle
{
public:
  /*!\typedef Canvas
    Local typedef for C
   */
  typedef C Canvas;

  /*!\fn void canvas(C*)
    Sets the Canvas to which
    this WRATHCanvasHandleT points.
    The value is auto-magically set
    as NULL when the Canvas goes
    out of scope
    \param p value which to use
   */
  void
  canvas(Canvas *p)
  {
    WRATHCanvasHandle::canvas_base(p);
  }

  /*!\fn Canvas* canvas(void) const 
    Returns the Canvas to which
    this WRATHCanvasHandleT points.
   */
  Canvas*
  canvas(void) const
  {
    WRATHCanvas *R;

    R=WRATHCanvasHandle::canvas_base();
    WRATHassert(R==NULL or dynamic_cast<Canvas*>(R)!=NULL);
    return static_cast<Canvas*>(R);
  }
};  

/*! @} */

#endif
