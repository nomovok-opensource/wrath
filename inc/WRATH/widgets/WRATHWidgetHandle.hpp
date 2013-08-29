/*! 
 * \file WRATHWidgetHandle.hpp
 * \brief file WRATHWidgetHandle.hpp
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


#ifndef __WRATH_WIDGET_HANDLE_HPP__
#define __WRATH_WIDGET_HANDLE_HPP__

#include "WRATHConfig.hpp"
#include "WRATHWidget.hpp"
  
/*! \addtogroup Widgets
 * @{
 */


/*!\class WRATHWidgetHandleAutoDelete
  In contrast to \ref WRATHWidgetHandle, (from which
  WRATHWidgetHandleAutoDelete inherits), a 
  WRATHWidgetHandleAutoDelete will delete the 
  widget it handles when it goes out of scope.
*/
template<typename pWidget>
class WRATHWidgetHandleAutoDelete;

/*!\class WRATHWidgetHandle
  A WidgetHandle is a container for a
  pointer to a widget.
  In addition, the WidgetHandle catches
  a signal for when the underlying
  widget is deleted and updates it's
  widget pointer as NULL on such a 
  signal
*/
template<typename pWidget>
class WRATHWidgetHandle:boost::noncopyable
{
public:
  
  /*!\typedef Widget
    Typedef to get the "widget" type.
  */
  typedef pWidget Widget;
  
  /*!\typedef WidgetBase
    Widget base typedef
  */
  typedef typename Widget::WidgetBase WidgetBase;

  /*!\typedef Node
    Conveniance typedef import from WidgetBase
   */
  typedef typename WidgetBase::Node Node;

  /*!\typedef Canvas
    Conveniance typedef import from WidgetBase
   */
  typedef typename WidgetBase::Canvas Canvas;

  /*!\typedef SubKey
    Conveniance typedef import from WidgetBase
   */
  typedef typename WidgetBase::SubKey SubKey;

  /*!\typedef DrawerFactory
    Conveniance typedef import from WidgetBase
   */
  typedef typename WidgetBase::DrawerFactory DrawerFactory;
  
  /*!\typedef AutoDelete
    Converniance typedef for the AutoDelete analogue
    of the WidgetHandle type.
  */
  typedef WRATHWidgetHandleAutoDelete<pWidget> AutoDelete;
  
  /*!\fn WRATHWidgetHandle
    Ctor, intializes as having no widet to handle.
   */ 
  WRATHWidgetHandle(void):
    m_widget(NULL)
  {}

  ~WRATHWidgetHandle()
  {
    m_dtor_connect.disconnect();
  }
  
  /*!\fn Widget* widget(void) const
    Returns the widget that this handle handles.
    If the return value if NULL, then the handle
    is not handling a widget.
   */
  Widget*
  widget(void) const
  {
    return m_widget;
  }
  
  /*!\fn void widget(Widget*)
    Sets the widget that this handle handles. 
    The WRATHWidgetHandle will auto-magically
    set this value to NULL when the widget it
    handles goes out of scope. Changing the
    widget to a different value does NOT delete
    the old widget.
    \param p new widget to handle.
   */
  void
  widget(Widget *p)
  {
    if(p==m_widget)
      {
        return;
      }
    m_dtor_connect.disconnect();
    m_widget=p;
    if(m_widget!=NULL)
      {
        m_widget->properties()->connect_dtor(boost::bind(&WRATHWidgetHandle::null_widget, this));
      }
  }
  
  /*!\fn void null_widget(void)
    Provided as a conveniance, equivalent to
    \code
    widget(NULL)
    \endcode

    Note that calling null_widget() does NOT
    delete the underlying widget.
   */
  void
  null_widget(void)
  {
    widget(NULL);
  }

  /*!\fn void delete_widget(void) 
    (Phase) deletes the widget handled by this
    handle. 
   */
  void
  delete_widget(void)
  {
    if(m_widget!=NULL)
      {
        pWidget *q(m_widget);
        WRATHPhasedDelete(q);
      }
    WRATHassert(m_widget==NULL);
  }

private:
  Widget *m_widget;
  WRATHBaseItem::connect_t m_dtor_connect;
};


/*!\class WRATHWidgetHandleAutoDelete
  In contrast to \ref WRATHWidgetHandle, (from which
  WRATHWidgetHandleAutoDelete inherits), a 
  WRATHWidgetHandleAutoDelete will delete the 
  widget it handles when it goes out of scope.
*/
template<typename pWidget>
class WRATHWidgetHandleAutoDelete:public WRATHWidgetHandle<pWidget>
{
public:
  
  ~WRATHWidgetHandleAutoDelete()
  {
    this->delete_widget();
  }
  
  /*!\fn pWidget* release_widget(void)
    Returns and releases the widget from
    this \ref WRATHWidgetHandleAutoDelete.
  */
  pWidget*
  release_widget(void)
  {
    pWidget *q;
    q=this->widget();
    this->null_widget();
    return q;
  }
};


/*! @} */


#endif
