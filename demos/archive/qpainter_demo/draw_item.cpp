/*! 
 * \file draw_item.cpp
 * \brief file draw_item.cpp
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


#include "draw_item.hpp"

DrawItem::
DrawItem(DrawList *parent):
  QObject(parent)
{}


DrawItem::
~DrawItem()
{}

DrawList::
DrawList(QObject *parent):
  QObject(parent)
{}

DrawList::
~DrawList()
{}


void
DrawList::
draw(QPainter *p)
{
  for(QObjectList::const_iterator iter=children().begin(),
        end=children().end(); iter!=end; ++iter)
    {
      DrawItem *ptr;

      ptr=qobject_cast<DrawItem*>(*iter);
      if(ptr!=NULL)
        {
          ptr->drawItem(p);
        }
    }
}
