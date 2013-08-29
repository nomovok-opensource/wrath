/*! 
 * \file WRATHBaseItem.hpp
 * \brief file WRATHBaseItem.hpp
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




#ifndef __WRATH_BASE_ITEM_HPP__
#define __WRATH_BASE_ITEM_HPP__

#include "WRATHConfig.hpp"
#include <boost/signals2.hpp>
#include "WRATHCanvas.hpp"
#include "WRATHMultiGLProgram.hpp"


/*! \addtogroup Items
 * @{
 */

/*!\class WRATHBaseItem
  A WRATHBaseItem provides a common base class
  for item classes in WRATH. The common interface
  for all items are:
  - to return the WRATHCanvas that the item resides within
  - fire a signal when the item is deleted

  In addition, WRATHBaseItem defines a number
  of conventions for shaders used to draw
  items. These conventions also provide
  \ref WRATHMultiGLProgram::Selector
  values for use in drawing.
 */
class WRATHBaseItem:boost::noncopyable
{
public:
   
  /*!\typedef signal_t
    Conveniance typedef for the dtor signal type.
   */
  typedef boost::signals2::signal<void ()> signal_t;

  /*!\typedef connect_t
    Conveniance typedef for the dtor connection type.
   */
  typedef boost::signals2::connection connect_t;

  WRATHBaseItem(void)
  {}

  virtual
  ~WRATHBaseItem()
  {
    m_dtor_signal();
  }

  /*!\fn connect_dtor
    Connect to the signal fired when the dtor
    of the WRATHBaseItem is called.
    \param subscriber slot called on signal fire
    \param gp_order order of slot call. Lower values of gp_order
                    are guarnteed to be call those of higher values
                    of gp_order. Slots connected with the same
                    value of gp_order are called in a non-deterministic
                    order (i.e. order of calling connect_dtor does
                    not imply any order about the order of being called).
   */
  connect_t
  connect_dtor(const signal_t::slot_type &subscriber, int gp_order=0)
  {
    return m_dtor_signal.connect(gp_order, subscriber);
  }

  /*!\fn canvas_base(void) const
    To be implemented by a derived class to
    return the WRATHCanvas on which the item
    resides.
   */
  virtual
  WRATHCanvas*
  canvas_base(void) const=0;

  /*!\fn canvas_base(WRATHCanvas*)
    To be implemented by a derived class to
    change the WRATHCanvas on which the item
    resides.
    \param c WRATHCanvas to change item to
   */
  virtual
  void
  canvas_base(WRATHCanvas *c)=0;

  /*!\fn WRATHMultiGLProgram::Selector selector_color_depth_draw
    WRATHMultiGLProgram::Selector for "normal" item drawing:
    drawing to depth and color buffer. Does not define
    any additional macros.
   */
  static
  WRATHMultiGLProgram::Selector 
  selector_color_depth_draw(void);

  /*!\fn WRATHMultiGLProgram::Selector selector_depth_stenicl_only_draw
    WRATHMultiGLProgram::Selector for depth and/or
    stencil drawing. Color buffer writes are not performed.
    Defines the macro WRATH_DEPTH_STENCIL_ONLY_DRAW.
   */
  static
  WRATHMultiGLProgram::Selector
  selector_depth_stenicl_only_draw(void);

  /*!\fn WRATHMultiGLProgram::Selector selector_color_post_draw
    WRATHMultiGLProgram::Selector for color only
    writes after a depth only pass. This is used 
    for drawing where the z-value is layed down 
    first followed by drawing to the color buffer.
    In this drawing case, if a shader uses discard
    to avoid drawing it can skip that check in this
    mode since the depth buffer is already correct
    from a previous draw.
    Defines the macro WRATH_POST_DEPTH_COLOR_ONLY_DRAW.
   */
  static
  WRATHMultiGLProgram::Selector
  selector_color_post_draw(void);

private:
  signal_t m_dtor_signal;
};


/*! @} */

#endif
