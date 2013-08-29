/*! 
 * \file image_item.hpp
 * \brief file image_item.hpp
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


#ifndef __IMAGE_ITEM_HPP__
#define __IMAGE_ITEM_HPP__

#include <QObject>
#include <QPainter>
#include "draw_item.hpp"
#include "transformation_node.hpp"


class TransformationNode;

class ImageItem:public DrawItem
{
public:

  ImageItem(QImage img, const QRectF &location_to_draw,
            DrawList *drawer, 
            TransformationNode *transformation);

  ~ImageItem();

  virtual
  void
  drawItem(QPainter *painter);

  const QRectF&
  rect(void) const 
  {
    return m_rect;
  }

  void
  setRect(const QRectF &R)
  {
    m_rect=R;
  }

private:
  QImage m_image;
  QRectF m_rect;
  TransformationNode *m_transformation;
};


#endif
