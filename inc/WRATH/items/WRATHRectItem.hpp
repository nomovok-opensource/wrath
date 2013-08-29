/*! 
 * \file WRATHRectItem.hpp
 * \brief file WRATHRectItem.hpp
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




#ifndef __WRATH_IMAGE_ELEMENT_HPP__
#define __WRATH_IMAGE_ELEMENT_HPP__

#include "WRATHConfig.hpp"
#include <boost/utility.hpp>
#include <limits>
#include "WRATHRectAttributePacker.hpp"
#include "WRATHDefaultRectAttributePacker.hpp"
#include "WRATHBrush.hpp"
#include "WRATHBaseItem.hpp"
#include "WRATHItemTypes.hpp"

/*! \addtogroup Items
 * @{
 */

/*!\namespace WRATHRectItemTypes
  Namespace to encapsulate the types
  used to create/set parameters
  of WRATHRectItem objects
 */
namespace WRATHRectItemTypes
{
  /*!\typedef DrawerPass
    Import \ref WRATHItemTypes::DrawerPass
   */
  typedef WRATHItemTypes::DrawerPass DrawerPass;

  /*!\class Drawer
    Object to hold how to draw a WRATHRectItem:
    - WRATHRectAttributePacker 
    - Set of drawing passes
   */
  class Drawer:
    public WRATHItemTypes::Drawer<WRATHRectAttributePacker>
  {
  public:
    /*!\typedef base_class
      Typedef to base class, \ref WRATHItemTypes::Drawer
     */
    typedef WRATHItemTypes::Drawer<WRATHRectAttributePacker> base_class; 

    /*!\fn Drawer(void)
      Empty init ctor.
     */
    Drawer(void)
    {}

    /*!\fn Drawer(const WRATHShaderSpecifier*, 
                  const WRATHRectAttributePacker*, 
                  WRATHDrawType)   
      Ctor. 
      \param sh value to which to set \ref m_draw_passes[0].m_shader
      \param p attribute packer to generate attribute data
      \param ppass WRATHDrawType specifying at which pass to draw
     */
    Drawer(const WRATHShaderSpecifier *sh,
           const WRATHRectAttributePacker *p=WRATHDefaultRectAttributePacker::fetch(),
           WRATHDrawType ppass=WRATHDrawType::opaque_pass()):
      base_class(sh, p, ppass)
    {}

    /*!\fn Drawer(const WRATHBrush&, WRATHDrawType, enum WRATHBaseSource::precision_t)
      Set the drawer to use a built in shader made from the passed \ref WRATHBrush.
      \param brush \ref WRATHBrush specifying if/how to apply image, gradient and const-color
      \param ppass WRATHDrawType specifying at which pass to draw
      \param v precision qualifiers to use in computing the gradient
               interpolate.
     */
    Drawer(const WRATHBrush &brush,
           WRATHDrawType ppass=WRATHDrawType::opaque_pass(),
           enum WRATHBaseSource::precision_t v
           =WRATHBaseSource::mediump_precision);    
  };
};


/*!\class WRATHRectItem
  A WRATHRectItem represents drawing a rectangle.
 */
class WRATHRectItem:public WRATHBaseItem
{
public:
  /*!\typedef DrawerPass
    Bring into scope WRATHRectItemTypes::DrawerPass
   */
  typedef WRATHRectItemTypes::DrawerPass DrawerPass;

  /*!\typedef Drawer
    Bring into scope WRATHRectItemTypes::Drawer
   */
  typedef WRATHRectItemTypes::Drawer Drawer;

  /*!\fn WRATHRectItem
    Ctor for a WRATHRectItem
    \param fact WRATHItemDrawerFactory responsible for fetching/creating
                the WRATHItemDrawer used by the item
    \param subdrawer_id SubDrawer Id passed to \ref WRATHItemDrawerFactory::generate_drawer()
    \param canvas WRATHCanvas where created item is placed, Canvas does NOT own the item.
    \param subkey SubKey used by pcanvas, typically holds "what" transformation/clipping node to use
    \param drawer drawer used to draw the image
   */
  explicit
  WRATHRectItem(const WRATHItemDrawerFactory &fact, int subdrawer_id,
                WRATHCanvas *canvas,
                const WRATHCanvas::SubKeyBase &subkey,
                const Drawer &drawer);

  virtual
  ~WRATHRectItem();

  /*!\fn void set_parameters
    Set/change the attribute values for drawing.
    \param rect handle to data describing rectangle to draw
   */
  void
  set_parameters(const WRATHReferenceCountedObject::handle &rect);
  
  virtual
  WRATHCanvas*
  canvas_base(void) const
  {
    return m_item_group.parent();
  }

  virtual
  void 
  canvas_base(WRATHCanvas*);

private:
  const WRATHRectAttributePacker *m_packer;
  WRATHCanvas::DataHandle m_item_group;
  range_type<int> m_attribute_data_location;
  WRATHIndexGroupAllocator::index_group<GLushort> m_index_data_location;
  WRATHStateBasedPackingData::handle m_immutable_packing_data;
  
};
/*! @} */

#endif
