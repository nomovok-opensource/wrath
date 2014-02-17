/*! 
 * \file WRATHWidget.hpp
 * \brief file WRATHWidget.hpp
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


#ifndef WRATH_HEADER_WIDGET_HPP_
#define WRATH_HEADER_WIDGET_HPP_

#include "WRATHConfig.hpp"
#include "WRATHTextItem.hpp"
#include "WRATHShapeItem.hpp"
#include "WRATHRectItem.hpp"
#include "WRATHEmptyItem.hpp"
#include "WRATHCanvasItem.hpp"


/*!\addtogroup Widgets
 * @{
 */

/*!\class WRATHEmptyWidget
  A WRATHEmptyWidget represents a transformation
  and/or clipping information to be applied
  to child widgets.
  \tparam pWidgetBase type defined by \ref WRATHWidgetBase providing 
                      node type, canvas type, etc for the widget 
 */
template<typename pWidgetBase>
class WRATHEmptyWidget:
  public pWidgetBase,
  public WRATHEmptyItem
{
public:
  /*!\typedef item_type
    Typedef for the item type of the widget class
   */
  typedef WRATHEmptyItem item_type;

  /*!\typedef WidgetBase
    Widget base typedef
   */
  typedef pWidgetBase WidgetBase;

  /*!\typedef Node
    Conveniance typedef for the Node type
   */
  typedef typename WidgetBase::Node Node;

  /*!\typedef Canvas
    Conveniance typedef to define the Canvas type.
   */
  typedef typename WidgetBase::Canvas Canvas;

  /*!\fn WRATHEmptyWidget(Canvas*)
    Ctor.
    \param pcanvas Canvas of the widget, canvas 
                   takes ownership of the widget
   */
  explicit
  WRATHEmptyWidget(Canvas *pcanvas):
    WidgetBase(pcanvas),
    WRATHEmptyItem(pcanvas)
  {}

  /*!\fn WRATHEmptyWidget(WidgetType*)
    Ctor.
    \param pparent_widget parent of the widget, widget
                          will use the same canvas as
                          pparent_widget. Additionally,
                          pparent_widget takes owner ship
                          of the widget.
   */
  template<typename WidgetType>
  explicit
  WRATHEmptyWidget(WidgetType *pparent_widget):
    WidgetBase(pparent_widget->node()),
    WRATHEmptyItem(pparent_widget->canvas())
  {}

  /*!\fn WRATHEmptyWidget(ParentType*, Canvas*)
    Ctor.
    \param pparent_widget parent of the widget. Additionally,
                          pparent_widget takes owner ship
                          of the widget.
    \param pcanvas Canvas of the widget
   */
  template<typename ParentType>
  WRATHEmptyWidget(ParentType *pparent_widget,
                   Canvas *pcanvas):
    WidgetBase(pparent_widget),
    WRATHEmptyItem(pcanvas)
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

/*!\class WRATHTextWidget
  A WRATHTextWidget is widget for
  drawing text, the underlying
  item is a \ref WRATHTextItem

  \tparam pWidgetBase type defined by \ref WRATHWidgetBase providing 
                      node type, canvas type, etc for the widget 
 */
template<typename pWidgetBase>
class WRATHTextWidget:
  public pWidgetBase,
  public WRATHTextItem
{
public:
  /*!\typedef item_type
    Typedef for the item type of the widget class
   */
  typedef WRATHTextItem item_type;

  /*!\typedef WidgetBase
    Convenience typedef to define the WidgetBase 
   */
  typedef pWidgetBase WidgetBase;

  /*!\typedef Node
    Conveniance typedef for the Node type
   */
  typedef typename WidgetBase::Node Node;

  /*!\typedef Canvas
    Convenience typedef to define the Canvas type.
   */
  typedef typename WidgetBase::Canvas Canvas;

  /*!\typedef Drawer
    Convenience typedef
   */
  typedef WRATHTextItem::Drawer Drawer;

  /*!\typedef draw_order
    Convenience typedef
   */
  typedef WRATHTextItem::draw_order draw_order;

  /*!\typedef ExtraDrawState
    Convenience typedef
   */
  typedef WRATHTextItem::ExtraDrawState ExtraDrawState;

  /*!\fn WRATHTextWidget(Canvas*, enum WRATHTextItemTypes::text_opacity_t,
                         const Drawer&,
                         const draw_order&,
                         const ExtraDrawState&)
    Ctor.
    \param pcanvas Canvas of the widget, canvas 
                   takes ownership of the widget
    \param item_opacity opacity of the text of the text widget
    \param pdrawer drawer for the text of the text widget
    \param pdraw_order draw order object of the text widget
    \param extra_state additional GL state vector data for drawing the text widget
   */
  WRATHTextWidget(Canvas *pcanvas,
                  enum WRATHTextItemTypes::text_opacity_t item_opacity,
                  const Drawer &pdrawer=Drawer(),
                  const draw_order &pdraw_order=draw_order(),
                  const ExtraDrawState &extra_state=ExtraDrawState()):
    WidgetBase(pcanvas),
    WRATHTextItem(typename WidgetBase::DrawerFactory(), 
                  WidgetBase::subdrawer_id(),
                  pcanvas, WidgetBase::subkey(),
                  item_opacity, 
                  pdrawer, pdraw_order,
                  extra_state)
  {}

  /*!\fn WRATHTextWidget(WidgetType*, 
                         enum WRATHTextItemTypes::text_opacity_t,
                         const Drawer&,
                         const draw_order&,
                         const ExtraDrawState&)
    Ctor.
    \param parent_widget parent of the text widget, 
                         text widget will use the same 
                         canvas as parent_widget. Additionally,
                         parent_widget takes owner ship
                         of the widget.
    \param item_opacity opacity of the text of the text widget
    \param pdrawer drawer for the text of the text widget
    \param pdraw_order draw order object of the text widget
    \param extra_state additional GL state vector data for drawing the text widget
   */
  template<typename WidgetType>
  WRATHTextWidget(WidgetType *parent_widget,
                  enum WRATHTextItemTypes::text_opacity_t item_opacity,
                  const Drawer &pdrawer=Drawer(),
                  const draw_order &pdraw_order=draw_order(),
                  const ExtraDrawState &extra_state=ExtraDrawState()):
    WidgetBase(parent_widget->node()),
    WRATHTextItem(typename WidgetBase::DrawerFactory(), 
                  WidgetBase::subdrawer_id(),
                  parent_widget->canvas(), WidgetBase::subkey(),
                  item_opacity,
                  pdrawer, pdraw_order,
                  extra_state) 
  {}
                  
  /*!\fn WRATHTextWidget(ParentType*, Canvas*,
                         enum WRATHTextItemTypes::text_opacity_t,
                         const Drawer&,
                         const draw_order&,
                         const ExtraDrawState&)
    Ctor.
    \param parent parent of the text widget.
                  Additionally, parent takes owner ship of the widget.
    \param pcanvas Canvas of the widget
    \param item_opacity opacity of the text of the text widget
    \param pdrawer drawer for the text of the text widget
    \param pdraw_order draw order object of the text widget
    \param extra_state additional GL state vector data for drawing the text widget
    \tparam ParentType must be consumable as the ctor for WidgetBase,
                       most common use case is that it is a node type
   */
  template<typename ParentType>
  WRATHTextWidget(ParentType *parent,
                  Canvas *pcanvas,
                  enum WRATHTextItemTypes::text_opacity_t item_opacity,
                  const Drawer &pdrawer=Drawer(),
                  const draw_order &pdraw_order=draw_order(),
                  const ExtraDrawState &extra_state=ExtraDrawState()):
    WidgetBase(parent),
    WRATHTextItem(typename WidgetBase::DrawerFactory(), 
                  WidgetBase::subdrawer_id(),
                  pcanvas, WidgetBase::subkey(),
                  item_opacity, 
                  pdrawer, pdraw_order,
                  extra_state)
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

/*!\class WRATHRectWidget
  A WRATHRectWidget is widget for
  drawing an image, the underlying
  item is a \ref WRATHRectItem.
  A WRATHRectWidget may use as 
  the source image a \ref WRATHImage 
  or raw texture handles.
  \tparam pWidgetBase type defined by \ref WRATHWidgetBase providing 
                       node type, canvas type, etc for the widget 
 */
template<typename pWidgetBase>
class WRATHRectWidget:
  public pWidgetBase,
  public WRATHRectItem
{
public:
  /*!\typedef item_type
    Typedef for the item type of the widget class
   */
  typedef WRATHRectItem item_type;

  /*!\typedef WidgetBase
    Widget base typedef
   */
  typedef pWidgetBase WidgetBase;

  /*!\typedef Node
    Conveniance typedef for the Node type
   */
  typedef typename WidgetBase::Node Node;

  /*!\typedef Canvas
    Convenience typedef to define the Canvas type.
   */
  typedef typename WidgetBase::Canvas Canvas;

  /*!\typedef Drawer
    Convenience typedef
   */
  typedef WRATHRectItem::Drawer Drawer;

  /*!\fn WRATHRectWidget(Canvas*, const Drawer&)
    Ctor.
    \param pcanvas Canvas of the widget, canvas 
                   takes ownership of the widget
    \param pdrawer drawer used to draw the rect
   */
  WRATHRectWidget(Canvas *pcanvas, const Drawer &pdrawer):
    WidgetBase(pcanvas),
    WRATHRectItem(typename WidgetBase::DrawerFactory(), 
                  WidgetBase::subdrawer_id(),
                  pcanvas, WidgetBase::subkey(),
                  pdrawer)
  {}

  /*!\fn WRATHRectWidget(WidgetType*, const Drawer&)
    Ctor.
    \param parent_widget parent of the text widget, 
                         text widget will use the same 
                         canvas as parent_widget. Additionally,
                         parent_widget takes owner ship
                         of the widget.    
    \param pdrawer drawer used to draw the rect
   */
  template<typename WidgetType>
  WRATHRectWidget(WidgetType *parent_widget, const Drawer &pdrawer):
    WidgetBase(parent_widget->node()),
    WRATHRectItem(typename WidgetBase::DrawerFactory(), 
                  WidgetBase::subdrawer_id(),
                  parent_widget->canvas(), WidgetBase::subkey(),
                  pdrawer)
  {}
       
  /*!\fn WRATHRectWidget(ParentType*, Canvas*, const Drawer&)
    Ctor. 
    \param parent parent of the image widget. Additionally, 
                  parent takes owner ship of the widget.
    \param pcanvas Canvas of the widget
    \param pdrawer drawer used to draw the rect
    \tparam ParentType must be consumable as the ctor for WidgetBase,
                       most common use case is that it is a node type
   */
  template<typename ParentType>
  WRATHRectWidget(ParentType *parent, Canvas *pcanvas, const Drawer &pdrawer):
    WidgetBase(parent),
    WRATHRectItem(typename WidgetBase::DrawerFactory(), 
                  WidgetBase::subdrawer_id(),
                  pcanvas, WidgetBase::subkey(),
                  pdrawer)
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


/*!\class WRATHShapeWidget
  A WRATHShapeWidget is widget for
  drawing a shape, the underlying
  item is a \ref WRATHShapeItem.

  \tparam pWidgetBase type defined by \ref WRATHWidgetBase providing 
                       node type, canvas type, etc for the widget 
 */
template<typename pWidgetBase>
class WRATHShapeWidget:
  public pWidgetBase,
  public WRATHShapeItem
{
public:
  /*!\typedef item_type
    Typedef for the item type of the widget class
   */
  typedef WRATHShapeItem item_type;

  /*!\typedef WidgetBase
    Widget base typedef
   */
  typedef pWidgetBase WidgetBase;

  /*!\typedef Node
    Conveniance typedef for the Node type
   */
  typedef typename WidgetBase::Node Node;

  /*!\typedef Canvas
    Convenience typedef to define the Canvas type.
   */
  typedef typename WidgetBase::Canvas Canvas;

  /*!\fn WRATHShapeWidget(Canvas*, const WRATHShapeItemTypes::shape_valueT<T>&,
                          const WRATHShapeItemTypes::ShapeDrawer<T>&,
                          const WRATHShapeAttributePackerBase::PackingParametersBase&)
    Ctor.
    \param pcanvas Canvas of the widget, canvas 
                   takes ownership of the widget
    \param pShape shape for the widget to draw
    \param pdrawer drawer used to draw the shape
    \param additional_packing_params additional attribute packing parameters
                                     that WRATHShapeItem::Drawer<T>::m_packer
                                     may use when creating the attribute data
                                     for the underlying WRATHShapeItem.
   */  
  template<typename T>
  WRATHShapeWidget(Canvas *pcanvas,
                   const WRATHShapeItemTypes::shape_valueT<T> &pShape,
                   const WRATHShapeItemTypes::ShapeDrawer<T> &pdrawer,
                   const WRATHShapeAttributePackerBase::PackingParametersBase &additional_packing_params=
                   WRATHShapeAttributePackerBase::PackingParametersBase()):
    WidgetBase(pcanvas),
    WRATHShapeItem(typename WidgetBase::DrawerFactory(), 
                   WidgetBase::subdrawer_id(),
                   pcanvas, WidgetBase::subkey(),
                   pShape, pdrawer, 
                   additional_packing_params)
  {}
  
  /*!\fn WRATHShapeWidget(WidgetType*,
                          const WRATHShapeItemTypes::shape_valueT<T>&,
                          const WRATHShapeItemTypes::ShapeDrawer<T>&,
                          const WRATHShapeAttributePackerBase::PackingParametersBase&)
    Ctor.
    \param parent_widget parent of the text widget, 
                         text widget will use the same 
                         canvas as parent_widget. Additionally,
                         parent_widget takes owner ship
                         of the widget.
    \param pShape shape for the widget to draw
    \param pdrawer drawer used to draw the shape
    \param additional_packing_params additional attribute packing parameters
                                     that \ref WRATHShapeItemTypes::ShapeDrawer<T>::m_packer
                                     may use when creating the attribute data
                                     for the underlying \ref WRATHShapeItem.
   */  
  template<typename WidgetType, typename T>
  WRATHShapeWidget(WidgetType *parent_widget,
                   const WRATHShapeItemTypes::shape_valueT<T> &pShape,
                   const WRATHShapeItemTypes::ShapeDrawer<T> &pdrawer,
                   const WRATHShapeAttributePackerBase::PackingParametersBase &additional_packing_params=
                   WRATHShapeAttributePackerBase::PackingParametersBase()):
    WidgetBase(parent_widget->node()),
    WRATHShapeItem(typename WidgetBase::DrawerFactory(), 
                   WidgetBase::subdrawer_id(),
                   parent_widget->canvas(), WidgetBase::subkey(),
                   pShape, pdrawer, 
                   additional_packing_params)
  {}
  
  /*!\fn WRATHShapeWidget(ParentType*, Canvas*,
                          const WRATHShapeItemTypes::shape_valueT<T>&,
                          const WRATHShapeItemTypes::ShapeDrawer<T>&,
                          const WRATHShapeAttributePackerBase::PackingParametersBase&)
    Ctor.
    \param parent parent of the text widget. 
                  Additionally, parent takes 
                  owner ship of the widget.
    \param pcanvas Canvas of the widget
    \param pShape shape for the widget to draw
    \param pdrawer drawer used to draw the shape
    \param additional_packing_params additional attribute packing parameters
                                     that \ref WRATHShapeItemTypes::ShapeDrawer<T>::m_packer
                                     may use when creating the attribute data
                                     for the underlying \ref WRATHShapeItem.
    \tparam ParentType must be consumable as the ctor for WidgetBase,
                       most common use case is that it is a node type
   */  
  template<typename ParentType, typename T>
  WRATHShapeWidget(ParentType *parent,
                   Canvas *pcanvas,
                   const WRATHShapeItemTypes::shape_valueT<T> &pShape,
                   const WRATHShapeItemTypes::ShapeDrawer<T> &pdrawer,
                   const WRATHShapeAttributePackerBase::PackingParametersBase &additional_packing_params=
                   WRATHShapeAttributePackerBase::PackingParametersBase()):
    WidgetBase(parent),
    WRATHShapeItem(typename WidgetBase::DrawerFactory(), 
                   WidgetBase::subdrawer_id(),
                   pcanvas, WidgetBase::subkey(),
                   pShape, pdrawer, 
                   additional_packing_params)
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


/*!\class WRATHCanvasWidget
  Represents a widget whose contents
  is a child canvas object.
  \tparam pWidgetBase type defined by \ref WRATHWidgetBase providing 
                      node type, canvas type, etc for the widget.
                      The underlying node type (see \ref WRATHWidgetBase::Node)
                      must implement the non-static member function 
                      \code void canvas_as_child_of_node(Canvas*) \endcode
                      that makes the passed Canvas draw as a child
                      of the node; for example if the node type encodes
                      a position, the the Canvas should be drawed at
                      the position.
 */
template<typename pWidgetBase>
class WRATHCanvasWidget:
  public pWidgetBase,
  public WRATHCanvasItem<typename pWidgetBase::Canvas>
{
public:
  /*!\typedef WidgetBase
    Widget base typedef
   */
  typedef pWidgetBase WidgetBase;

  /*!\typedef Node
    Conveniance typedef for the Node type
   */
  typedef typename WidgetBase::Node Node;

  /*!\typedef Canvas
    Conveniance typedef to define the Canvas type.
   */
  typedef typename WidgetBase::Canvas Canvas;

  /*!\typedef item_type
    Typedef for the item type of the widget class
   */
  typedef WRATHCanvasItem<Canvas> item_type;

  /*!\fn WRATHCanvasWidget(Canvas*)
    Ctor.
    \param pcanvas Canvas of the widget, canvas takes ownership of the widget
   */
  explicit
  WRATHCanvasWidget(Canvas *pcanvas):
    WidgetBase(pcanvas),
    WRATHCanvasItem<Canvas>(pcanvas)
  {
    finish_ctor();
  }
  
  /*!\fn WRATHCanvasWidget(WidgetType*)
    Ctor.
    \param pparent_widget parent of the widget, widget
                          will use the same canvas as
                          pparent_widget. Additionally,
                          pparent_widget takes owner ship
                          of the widget.
   */
  template<typename WidgetType>
  explicit
  WRATHCanvasWidget(WidgetType *pparent_widget):
    WidgetBase(pparent_widget->node()),
    WRATHCanvasItem<Canvas>(static_cast<Canvas*>(pparent_widget->canvas()))
  {
    finish_ctor();
  }

  /*!\fn WRATHCanvasWidget(ParentType*, Canvas*)
    Ctor.
    \param parent parent of the widget. Additionally,
                         parent takes owner ship
                         of the widget.
    \param pcanvas Canvas of the widget
   */
  template<typename ParentType>
  WRATHCanvasWidget(ParentType *parent, Canvas *pcanvas):
    WidgetBase(parent),
    WRATHCanvasItem<Canvas>(pcanvas)
  {
    finish_ctor();
  }

  ~WRATHCanvasWidget();

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

  /*!\fn WRATHEmptyWidget<WidgetBase>* empty_widget
    Returns an empty widget object whose
    Canvas is 
    \code 
    properties()->contents() 
    \endcode
    i.e. is a good root node to
    use for widgets to be placed
    within this WRATHCanvasWidget.
   */
  WRATHEmptyWidget<WidgetBase>*
  empty_widget(void)
  {
    return m_empty_widget;
  }

  /*!\fn const std::list<typename WidgetBase::Node*>& clip_out_items
    A WRATHCanvasWidget maintains a list of nodes
    of the clip out items applied to the canvas
    of the WRATHCanvasWidget. This function 
    returns those nodes as a linked list. 
   */
  const std::list<typename WidgetBase::Node*>&
  clip_out_items(void) const
  {
    return m_clipout_items_list;
  }

  /*!\fn void add_clip_out_item
    Add a clipout item to be tracked by the
    WRATHCanvasWidget. The item will be automagically
    removed from the list when the item goes
    out of scope. See also \ref clip_out_items().
    Adding a widget does not make it a clip out item,
    it is required that the item associated to the
    widget is added to the Canvas as a clip out item.
    \tparam W widget to add, must implement the function node()
              which is down castable to \ref Node. For example,
              anything derived from \ref WidgetBase will do.
    \param w Widget to add as a clip out item
   */
  template<typename W>
  void
  add_clip_out_item(W *w);

  /*!\fn void remove_clip_out_item
    Remove an clipout item to be tracked by the
    WRATHCanvasWidget, see \ref add_clip_out_item()
    \tparam W widget to add, must implement the function node()
              which is down castable to \ref Node. For example,
              anything derived from \ref WidgetBase will do.
    \param w Widget previously added by \ref add_clip_out_item()
   */
  template<typename W>
  void
  remove_clip_out_item(W *w)
  {
    typename WidgetBase::Node *n;
    WRATHassert(w!=NULL);
    n=w->node();
    remove_clip_out_item(n);
  }


private:
  void
  remove_clip_out_node(typename WidgetBase::Node *n);

  void
  finish_ctor(void)
  {  
    WidgetBase::canvas_as_child_of_node(this->contents());
    /*
      we make the parent of m_empty_widget this, which means
      its transformation and z-order jazz get composited with
      this, which are compoisted with this->parent(). We do 
      NOT want the transformation composited, thus we
      set compose_transformation_with_parent() as false,
      but we do want the z-order composited for heirarchy
      z-ordering nodes.
     */
    m_empty_widget=WRATHNew WRATHEmptyWidget<WidgetBase>(this, this->contents());
    m_empty_widget->compose_transformation_with_parent(false);
  }

  typedef std::list<typename WidgetBase::Node*> list_type;
  typedef std::pair<WRATHBaseItem::connect_t, typename list_type::iterator> map_value;
  typedef std::map<typename WidgetBase::Node*, map_value> map_type;

  WRATHEmptyWidget<WidgetBase> *m_empty_widget;
  map_type m_clipout_items;
  list_type m_clipout_items_list;
};

/*!\class WRATHWidgetBase
  WRATHWidgetBase is a template machine to localize a node type,
  canvas type, drawer factory type and a sub drawer ID value
  for the purpose of feeding those to consructors of items
  so that the item has a node applied to its contents.
  \tparam NodeType the node type for the items
  \tparam CanvasType the canvas type for the items, derived from WRATHCanvas,
          the type CanvasType::SubKey must be defined, derived from WRATHCanvas::SubKeyBase
          and constructable from a pointer to a NodeType. In addition, CanvasType
          must define a template function root_node<N> where N is a node type
          that will return a root node of that type to the Canvas
  \tparam DrawerFactoryType the factory type for the items, derived from WRATHItemDrawerFactory,
          DrawerFactoryType must be stateless and provide a constructor taking no arguments. 
  \tparam SubDrawerID used by the items passed to the FactoryType when creating drawers
 */
template<typename NodeType,
         typename CanvasType,
         typename DrawerFactoryType,
         int SubDrawerID=0>
class WRATHWidgetBase:public NodeType
{
public:
  /*!\typedef Node
    Conveniance typedef for the Node type
   */
  typedef NodeType Node;

  /*!\typedef Canvas
    Conveniance typedef for the Canvas type
   */
  typedef CanvasType Canvas;

  /*!\typedef SubKey
    Conveniance typedef for the SubKey type
   */
  typedef typename CanvasType::SubKey SubKey;

  /*!\typedef DrawerFactory
    Conveniance typedef for the DrawerFactory type
   */
  typedef DrawerFactoryType DrawerFactory;

  /*!\typedef WidgetBase
    Conveniance self-typedef
   */  
  typedef WRATHWidgetBase WidgetBase;
    
  /*!\fn WRATHWidgetBase(Canvas*)
    Ctor. Widget is a child of a \ref Canvas directly,
    i.e. canvas takes ownership of the widget.
    \param dr \ref Canvas to be the parent of this 
   */
  WRATHWidgetBase(Canvas *dr):
    Node(dr->template root_node<Node>())
  {}
  
  /*!\fn WRATHWidgetBase(Node*)
    Ctor. Widget is a child of a \ref Node, node
    takes ownerhsip of the widget
    \param pparent \ref Node to be the parent of this 
   */
  WRATHWidgetBase(Node *pparent):
    Node(pparent)
  {}
  
  /*!\fn WRATHWidgetBase(S *)
    Ctor. Widget is a child of a widget.
    \tparam S node must be constructable from a pointer to S
    \param pparent S to be the parent of this 
  */
  template<typename S>
  WRATHWidgetBase(S *pparent):
    Node(pparent)
  {}

  ~WRATHWidgetBase()
  {}
  
  /*!\fn SubKey subkey
    Returns the \ref SubKey to be used by
    item constructors. 
   */
  SubKey
  subkey(void) 
  {
    return SubKey(this);
  }
  
  /*!\fn int subdrawer_id
    Returns the sub-drawer id used by items
    to pass to the \ref DrawerFactory for
    creating the \ref WRATHItemDrawer objects
    to draw the items
   */
  int 
  subdrawer_id(void)
  {
    return SubDrawerID;
  }
  
  /*!\fn Node* node
    Provided as a template conveniance to
    return this downcasted to Node.
   */
  Node*
  node(void)
  {
    return this;
  }
  
  /*!\fn enum return_code parent_node(Node *)
    Sets the parent of this as a Node.
    Returns routine_success on succes, 
    routine_fail on failure.
    \param q node to set as the parent
   */
  enum return_code
  parent_node(Node *q)
  {
    return Node::parent(q);
  }

  /*!\fn ParentNodeType* parent_node(void)
    Returns the parent node of this upcasted
    to a specified type
    \tparam ParentNodeType node type to which to upcast
   */
  template<typename ParentNodeType>
  ParentNodeType*
  parent_node(void)
  {
    WRATHassert(dynamic_cast<ParentNodeType*>(Node::parent())!=NULL);
    return static_cast<ParentNodeType*>(Node::parent());
  }
  
  /*!\fn enum return_code parent_widget(Widget *)
    Sets the parent of this from the node
    of a widget. Returns routine_success 
    on succes, routine_fail on failure.
    \tparam Widget widget type of the parent 
    \param q Widget to which to set the parent, q must not be NULL
   */
  template<typename Widget>
  enum return_code
  parent_widget(Widget *q)
  {
    WRATHassert(q!=NULL);
    return Node::parent(q->node());
  }
  
 
};

/*! @} */


template<typename pWidgetBase>
void
WRATHCanvasWidget<pWidgetBase>::
remove_clip_out_node(typename WidgetBase::Node *n)
{
  typename map_type::iterator iter;
  
  iter=m_clipout_items.find(n);
  if(iter==m_clipout_items.end())
    {
      iter->second.first.disconnect();
      m_clipout_items_list.erase(iter->second.second);
      m_clipout_items.erase(iter);
    }
}

template<typename pWidgetBase>
template<typename W>
void
WRATHCanvasWidget<pWidgetBase>::
add_clip_out_item(W *w)
{
  typename map_type::iterator iter;
  typename WidgetBase::Node *n;
  
  WRATHassert(w!=NULL);
  n=w->node();
  
  iter=m_clipout_items.find(n);
  if(iter==m_clipout_items.end())
    {
      map_value v;

      v.second=m_clipout_items_list.insert(m_clipout_items_list.end(), n);
      v.first=w->properties()->connect_dtor(boost::bind(&WRATHCanvasWidget::remove_clip_out_node,
                                                        this, n) );

      m_clipout_items[n]=v;
    }
}

template<typename pWidgetBase>
WRATHCanvasWidget<pWidgetBase>::
~WRATHCanvasWidget()
{
  for(typename map_type::iterator iter=m_clipout_items.begin(), end=m_clipout_items.end();
      iter!=end; ++iter)
    {
      iter->second.first.disconnect();
    }
}


#endif

