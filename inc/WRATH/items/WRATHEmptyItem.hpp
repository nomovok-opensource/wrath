/*! 
 * \file WRATHEmptyItem.hpp
 * \brief file WRATHEmptyItem.hpp
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




#ifndef WRATH_HEADER_EMPTY_ITEM_HPP_
#define WRATH_HEADER_EMPTY_ITEM_HPP_

#include "WRATHConfig.hpp"
#include "WRATHCanvas.hpp"


/*! \addtogroup Items
 * @{
 */

/*!\class WRATHEmptyItem
  A WRATHEmptyItem represents an empty item;
  used as an item type for those widgets
  that only represent transformation and/or
  clipping information.
 */
class WRATHEmptyItem:public WRATHBaseItem
{
public:
  /*!\fn WRATHEmptyItem
    Ctor.
    \param p WRATHCanvas that the item is viewed as a part
   */
  explicit 
  WRATHEmptyItem(WRATHCanvas *p):
    m_canvas(p)
  {}

  virtual
  ~WRATHEmptyItem()
  {}

  virtual
  WRATHCanvas*
  canvas_base(void) const
  {
    return m_canvas;
  }

  virtual
  void
  canvas_base(WRATHCanvas *p)
  {
    m_canvas=p;
  }

private:
  WRATHCanvas *m_canvas;
};


/*! @} */

#endif
