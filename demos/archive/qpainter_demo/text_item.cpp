/*! 
 * \file text_item.cpp
 * \brief file text_item.cpp
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


#include "text_item.hpp"
#include "test_widget.hpp"
#include <iostream>

TextItem::
TextItem(QFont pfont, const QColor &pcolor, QString ptext,
         DrawList *drawer, 
         TransformationNode *transformation):
  DrawItem(drawer),
  m_font(pfont),
  m_text(ptext),
  m_color(pcolor),
  m_transformation(transformation)
{}

TextItem::
~TextItem()
{}


void
TextItem::
setColor(const QColor &c)
{
  m_color=c;
}

const QColor&
TextItem::
color(void) const
{
  return m_color;
}

void
TextItem::
setText(const QString &ptext)
{
  m_text=ptext;
}

const QString&
TextItem::
text(void) const
{
  return m_text;
}


void
TextItem::
drawItem(QPainter *painter)
{
  //save painter transformation state:
  //painter->save();

  painter->setTransform(m_transformation->getGlobalValue().transformation());
  painter->setFont(m_font);
  painter->setPen(m_color);
  painter->drawText(QPointF(10.0f, 10.0f), m_text);

  //restore transformation state:
  //painter->restore();
}
