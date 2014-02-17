/*! 
 * \file WRATHFamily.hpp
 * \brief file WRATHFamily.hpp
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


#ifndef WRATH_HEADER_FAMILY_HPP_
#define WRATH_HEADER_FAMILY_HPP_

#include "WRATHConfig.hpp"
#include "WRATHWidgetEnums.hpp"
#include "WRATHWidgetHandle.hpp"


/*! \addtogroup Widgets
 * @{
 */

/*!\class WRATHFamily
  A typedef "machine" that generates
  widget and handle types.
  \param pWidgetBase class type defined by \ref WRATHWidgetBase providing 
                     node type, canvas type, etc for the widget  
 */
template<typename pWidgetBase>
class WRATHFamily
{
public:
  /*!\typedef WidgetBase
    typedef for pWidgetBase class
   */
  typedef pWidgetBase WidgetBase;

  /*!\typedef Node
    local typedef for WidgetBase::Node class,
    see \ref WRATHWidgetBase
   */
  typedef typename WidgetBase::Node Node;

  /*!\typedef Canvas
    local typedef for WidgetBase::Canvas class,
    see \ref WRATHWidgetBase
   */
  typedef typename WidgetBase::Canvas Canvas;

  /*!\typedef SubKey
    local typedef for WidgetBase::SubKey class,
    see \ref WRATHWidgetBase
   */
  typedef typename WidgetBase::SubKey SubKey;

  /*!\typedef DrawerFactory
    local typedef for WidgetBase::DrawerFactory class,
    see \ref WRATHWidgetBase
   */
  typedef typename WidgetBase::DrawerFactory DrawerFactory;
  
  /*!\typedef NodeWidget
    Conveniance typedef for WRATHEmptyWidget class.
    Widget type is used for transformation hierarchy
    of widgets.
   */ 
  typedef WRATHEmptyWidget<pWidgetBase> NodeWidget;
  
  /*!\typedef TextWidget
    Conveniance typedef for WRATHTextWidget class
   */ 
  typedef WRATHTextWidget<pWidgetBase> TextWidget;
  
  /*!\typedef RectWidget
    Conveniance typedef for WRATHRectWidget class
   */ 
  typedef WRATHRectWidget<pWidgetBase> RectWidget;
  
  /*!\typedef ShapeWidget
    Conveniance typedef for WRATHShapeWidget class
   */ 
  typedef WRATHShapeWidget<pWidgetBase> ShapeWidget;
  
  /*!\typedef CanvasWidget
    Conveniance typedef for WRATHCanvasWidget class
   */ 
  typedef WRATHCanvasWidget<pWidgetBase> CanvasWidget;
  
  /*!\typedef NodeHandle
    Conveniance typedef for handle class to WRATHWidgetHandle\<\ref NodeWidget\>
   */
  typedef WRATHWidgetHandle<NodeWidget> NodeHandle;
  
  /*!\typedef DrawnText
    Conveniance typedef for handle class to WRATHWidgetHandle\<\ref TextWidget\>
   */
  typedef WRATHWidgetHandle<TextWidget> DrawnText;
  
  /*!\typedef DrawnRect
    Conveniance typedef for handle class to WRATHWidgetHandle\<\ref RectWidget\>
   */
  typedef WRATHWidgetHandle<RectWidget> DrawnRect;
  
  /*!\typedef DrawnShape
    Conveniance typedef for handle class to WRATHWidgetHandle\<\ref ShapeWidget\>
   */
  typedef WRATHWidgetHandle<ShapeWidget> DrawnShape;
  
  /*!\typedef DrawnCanvas
    Conveniance typedef for handle class to WRATHWidgetHandle\<\ref CanvasWidget\>
   */
  typedef WRATHWidgetHandle<CanvasWidget> DrawnCanvas;
};

/*! @} */

#endif
