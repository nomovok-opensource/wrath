/*! 
 * \file image_item.cpp
 * \brief file image_item.cpp
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


#include "image_item.hpp"
#include "test_widget.hpp"

ImageItem::
ImageItem(QImage img,  
          const QRectF &location_to_draw,
          DrawList *drawer, 
          TransformationNode *transformation):
  DrawItem(drawer),
  m_image(img),
  m_rect(location_to_draw),
  m_transformation(transformation)
{}

ImageItem::
~ImageItem()
{}

void
ImageItem::
drawItem(QPainter *painter)
{
  //save painter transformation state:
  //painter->save();

  //draw image, QPainter API does not seem
  //to include the ability to modulate the color of an image.
  painter->setTransform(m_transformation->getGlobalValue().transformation());
  painter->drawImage(m_rect, m_image); 

  //restore transformation state:
  //painter->restore();
}
