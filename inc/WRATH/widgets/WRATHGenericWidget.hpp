/*! 
 * \file WRATHGenericWidget.hpp
 * \brief file WRATHGenericWidget.hpp
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


#ifndef __WRATH_GENERIC_WIDGET_HPP__
#define __WRATH_GENERIC_WIDGET_HPP__

#include "WRATHConfig.hpp"
#include "WRATHBaseItem.hpp"


/*! \addtogroup Widgets
 * @{
 */

/*!\class WRATHGenericWidget

  WRATHGenericWidget create a widget type from an item type.
  \tparam pItemType the type for the underling UI item, must
                    be derived from WRATHBaseItem and provide
                    the member type parameters and the ctor 
                    of the item is of the form \code 
                    pItemType(const WRATHItemDrawerFactory &fact, int subdrawer_id,
                              WRATHCanvas *canvas,
                              const WRATHCanvas::SubKeyBase &subkey,
                              const pItemType::parameters &p)
                    \endcode
  \tparam pWidgetBase type defined by \ref WRATHWidgetBase providing 
                      node type, canvas type, etc for the widget  
 */
template<typename pItemType, typename pWidgetBase>
class WRATHGenericWidget:
  public pWidgetBase,
  public pItemType
{
public:

  /*!\typedef item_type
    Typedef for the item type of the widget class
   */
  typedef pItemType item_type;

  /*!\typedef parameters
    Typedef for the parameter type def to the ctor
    of the item_type.
   */
  typedef typename item_type::parameters parameters;

  /*!\typedef WidgetBase
    Widget base typedef
   */
  typedef pWidgetBase WidgetBase;

  /*!\typedef Canvas
    Convenience typedef to define the Canvas type.
   */
  typedef typename WidgetBase::Canvas Canvas;
  
  /*!\fn WRATHGenericWidget(Canvas*, const parameters&)
    Ctor.
    \param pcanvas Canvas of the widget, canvas 
                   takes ownership of the widget
    \param params parameters to specify the ctor for the underling item
   */  
  WRATHGenericWidget(Canvas *pcanvas,
                     const parameters &params):
    WidgetBase(pcanvas),
    item_type(typename WidgetBase::DrawerFactory(), 
              WidgetBase::subdrawer_id(),
              pcanvas, WidgetBase::subkey(),
              params)
  {}

  /*!\fn WRATHGenericWidget(WidgetType*, const parameters&)
    Ctor.
    \param parent_widget parent of the text widget, 
                         text widget will use the same 
                         canvas as parent_widget. Additionally,
                         parent_widget takes owner ship
                         of the widget. 
    \param params parameters to specify the ctor for the underling item
   */  
  template<typename WidgetType>
  WRATHGenericWidget(WidgetType *parent_widget,
                     const parameters &params):
    WidgetBase(parent_widget->node()),
    item_type(typename WidgetBase::DrawerFactory(), 
              WidgetBase::subdrawer_id(),
              parent_widget->canvas(), WidgetBase::subkey(),
              params)
  {}
                  
  /*!\fn WRATHGenericWidget(ParentType*, Canvas*,
                            const parameters&)
    Ctor.
    \param parent parent of the text widget. 
                  Additionally, parent takes 
                  owner ship of the widget.
    \param pcanvas Canvas of the widget
    \param params parameters to specify the ctor for the underling item
    \tparam ParentType must be consumable as the ctor for WidgetBase,
                       most common use case is that it is a node type
   */  
  template<typename ParentType>
  WRATHGenericWidget(ParentType *parent,
                     Canvas *pcanvas,
                     const parameters &params):
    WidgetBase(parent),
    item_type(typename WidgetBase::DrawerFactory(), 
              WidgetBase::subdrawer_id(),
              pcanvas, WidgetBase::subkey(),
              params)
  {}

  /*!\fn item_type* properties
    Returns this down casted to \ref item_type
   */
  item_type*
  properties(void)
  {
    return this;
  }

  /*!\fn Canvas* canvas(void) const
    Returns the Canvas of this object
    upcasted to Canvas. Debug builds
    check if the upcast succeeds
   */
  Canvas*
  canvas(void) const
  {
    WRATHassert(dynamic_cast<Canvas*>(this->canvas_base())!=NULL);
    return static_cast<Canvas*>(this->canvas_base());
  }

  /*!\fn void canvas(Canvas*)
    Sets the Canvas on which this item resides
    \param v Canvas to place item onto
   */
  void
  canvas(Canvas *v)
  {
    this->canvas_base(v);
  }

};


/*! @} */

#endif
