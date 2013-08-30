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

  /*!\fn WRATHMultiGLProgram::Selector selector_draw
    WRATHMultiGLProgram::Selector for "normal" item drawing:
    drawing to depth and color buffer. Does not define
    any additional macros.
   */
  static
  WRATHMultiGLProgram::Selector 
  selector_draw(void);

  /*!\fn WRATHMultiGLProgram::Selector selector_non_color_draw
    WRATHMultiGLProgram::Selector for drawing with color
    buffer masked out. Defines the macro WRATH_NON_COLOR_DRAW.
   */
  static
  WRATHMultiGLProgram::Selector
  selector_non_color_draw(void);

  /*!\fn WRATHMultiGLProgram::Selector selector_color_draw_cover
    WRATHMultiGLProgram::Selector for color only drawing
    where a previous draw pass set the depth values
    already (by drawing with \ref selector_non_color_draw()).
    In particular, if a fragment shader issues discard
    for normal drawing it should not for this drawing
    phase and should only compute a color value.
    Defines the macro WRATH_COVER_DRAW.
   */
  static
  WRATHMultiGLProgram::Selector
  selector_color_draw_cover(void);

  /*!\fn WRATHMultiGLProgram::Selector selector_non_color_draw_cover
    WRATHMultiGLProgram::Selector for when color buffer
    is masked out AND a previous draw pass already set
    the depth and/or stencil values to the buffer and
    relies on the depth and/or stencil test for coverage.
    Thus the fragment shader should do essentially nothing
    and the vertex shader just needs to make sure it emits
    vertices for primitives that cover. Defines
    the macros WRATH_NON_COLOR_DRAW and WRATH_COVER_DRAW
   */
  static
  WRATHMultiGLProgram::Selector
  selector_non_color_draw_cover(void);

private:
  signal_t m_dtor_signal;
};


/*! @} */

#endif
