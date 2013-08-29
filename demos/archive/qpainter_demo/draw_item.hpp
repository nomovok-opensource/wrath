/*! 
 * \file draw_item.hpp
 * \brief file draw_item.hpp
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


#ifndef __DRAW_ITEM_HPP__
#define __DRAW_ITEM_HPP__

#include <QObject>
#include <QPainter>

class DrawList;
class DrawItem:public QObject
{
  Q_OBJECT;

public:

  DrawItem(DrawList *parent);
  ~DrawItem();

  virtual
  void
  drawItem(QPainter*)=0;


};

class DrawList:public QObject
{
  Q_OBJECT;

public:
  DrawList(QObject *parent);
  ~DrawList();

  void
  draw(QPainter *p);
};


#endif
