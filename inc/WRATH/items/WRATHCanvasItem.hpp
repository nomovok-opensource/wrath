/*! 
 * \file WRATHCanvasItem.hpp
 * \brief file WRATHCanvasItem.hpp
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


#ifndef WRATH_HEADER_CANVAS_ITEM_HPP_
#define WRATH_HEADER_CANVAS_ITEM_HPP_

#include "WRATHCanvasHandle.hpp"
#include "WRATHBaseItem.hpp"


/*! \addtogroup Items
 * @{
 */

/*!\class WRATHCanvasItem
  A WRATHCanvasItem represents an item whose contents is a WRATHCanvas
  derived object. The canvas held is returned by contents().
  \tparam T WRATHCanvas derived type specifying the canvas type of contents().
            T must have a ctor of the form T(T *parent) where the created
            T will be a child of the T parent.
 */
template<typename T>
class WRATHCanvasItem:public WRATHBaseItem
{
public:
  /*!\fn WRATHCanvasItem
    Ctor. 
    \param c parent canvas to the canvas returned by contents()
   */ 
  explicit
  WRATHCanvasItem(T *c)
  {
    c=WRATHNew T(c);
    m_c.canvas(c);
  }

  virtual
  ~WRATHCanvasItem()
  {
    T *c;
    c=m_c.canvas();
    if(c!=NULL)
      {
        WRATHPhasedDelete(c);
      }
  }
 
  virtual
  WRATHCanvas*
  canvas_base(void) const
  {
    /*
      note: returns the parent of m_c.canvas()
      because we view the item as an item
      of that parent...
    */
    return m_c.canvas()->parent();
  }

  virtual
  void
  canvas_base(WRATHCanvas *p) 
  {
    WRATHassert(dynamic_cast<T*>(p)!=NULL);
    m_c.canvas()->parent(static_cast<T*>(p));
  }

  /*!\fn contents
    Returns the contents of the item, which
    is a WRAtHCanvas derived type.
   */
  T*
  contents(void)
  {
    return m_c.canvas();
  }

private:
  /*
    by using a WRATHCanvasHandleT, the canvas pointer
    inside is set to NULL if the canvas is deleted
   */
  WRATHCanvasHandleT<T> m_c;
};


/*! @} */

#endif
