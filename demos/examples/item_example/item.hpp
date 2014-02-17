/*! 
 * \file item.hpp
 * \brief file item.hpp
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


#ifndef ITEM_HPP
#define ITEM_HPP


#include "WRATHConfig.hpp"
#include "WRATHCanvas.hpp"
#include "WRATHBaseItem.hpp"
#include "WRATHGenericWidget.hpp"
#include "WRATHItemTypes.hpp"

class item:public WRATHBaseItem
{
public:
  /*
    our item ctor parameters; the item ctor parameters
    must be called parameters to work with \ref WRATHGenericWidget
   */
  class parameters
  {
  public:
    /*
      our item supports only one pass of drawing
      AND the attribute packing is fixed, so
      how to draw is specified exactly by
      a single WRATHItemTypes::DrawerPass
    */
    WRATHItemTypes::DrawerPass m_drawer;

    //item is a polygon, gives how many sides to it.
    int m_number_sides;

    //center of polygon
    vec2 m_center;

    //radius of polygon
    float m_radius;
  };

  item(const WRATHItemDrawerFactory &fact, int subdrawer_id,
       WRATHCanvas *pcanvas,
       const WRATHCanvas::SubKeyBase &subkey,
       const parameters &params);

  ~item();

  virtual
  WRATHCanvas*
  canvas_base(void) const;

  virtual
  void
  canvas_base(WRATHCanvas *c);

private:
  WRATHCanvas::DataHandle m_data_handle;
  WRATHIndexGroupAllocator::index_group<GLushort> m_indices;
  range_type<int> m_attribute_data_location;

};


#endif
