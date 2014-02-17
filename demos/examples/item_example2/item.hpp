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

//defines the attribute packer interface used by item
#include "item_packer.hpp"

class item:public WRATHBaseItem
{
public:
  /*
    typedef for drawer of our item
   */
  typedef WRATHItemTypes::Drawer<ItemAttributePacker> Drawer;

  /*
    the item ctor parameters type
    must be called parameters to work with \ref WRATHGenericWidget
   */
  class parameters 
  {
  public:
    /*
      how to pack and draw
     */
    Drawer m_drawer;

    /*
      fed to attribute packer.
     */
    ItemAttributePacker::packer_data m_polygon_spec;
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
  std::vector<range_type<int> > m_attribute_data_location;

};


#endif
